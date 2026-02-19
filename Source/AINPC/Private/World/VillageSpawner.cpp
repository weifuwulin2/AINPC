#include "World/VillageSpawner.h"

#include "AINPC.h"
#include "Components/FactionReputationComponent.h"
#include "Components/GoalComponent.h"
#include "Components/NPCDefinitionComponent.h"
#include "Components/UtilityAIComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/NPCVillageSubsystem.h"
#include "TimerManager.h"

AVillageSpawner::AVillageSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AVillageSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnOnBeginPlay)
	{
		SpawnScenario(ScenarioRowID);
	}
}

bool AVillageSpawner::ResolveScenarioRow(FName InScenarioRowID, const FVillageScenarioRow*& OutRow) const
{
	OutRow = nullptr;
	if (!VillageScenarioTable)
	{
		AINPC_LOG(Error, "[VillageSpawner] VillageScenarioTable is null.");
		return false;
	}

	const FName EffectiveRowID = InScenarioRowID.IsNone() ? ScenarioRowID : InScenarioRowID;
	if (EffectiveRowID.IsNone())
	{
		AINPC_LOG(Error, "[VillageSpawner] ScenarioRowID is not set.");
		return false;
	}

	static const FString ContextString(TEXT("VillageSpawner_ResolveScenarioRow"));
	OutRow = VillageScenarioTable->FindRow<FVillageScenarioRow>(EffectiveRowID, ContextString);
	if (!OutRow)
	{
		AINPC_LOG(Error, "[VillageSpawner] Scenario row '%s' not found.", *EffectiveRowID.ToString());
		return false;
	}

	return true;
}

bool AVillageSpawner::ValidateScenarioTemplate(FName InScenarioRowID, bool bLogDetails) const
{
	const FVillageScenarioRow* ScenarioRow = nullptr;
	if (!ResolveScenarioRow(InScenarioRowID, ScenarioRow))
	{
		return false;
	}

	bool bValid = true;
	const FName EffectiveRowID = InScenarioRowID.IsNone() ? ScenarioRowID : InScenarioRowID;
	TSet<FName> KnownSlotIDs;

	auto ValidateTemplateRef = [this, &bValid, bLogDetails](FName TemplateID, const TCHAR* ContextLabel)
	{
		if (!NPCDefinitionTable)
		{
			bValid = false;
			if (bLogDetails)
			{
				AINPC_LOG(Error, "[VillageSpawner] NPCDefinitionTable is null while validating %s.", ContextLabel);
			}
			return;
		}

		static const FString ContextString(TEXT("VillageSpawner_ValidateTemplateRef"));
		const FNPCDefinitionRow* TemplateRow = NPCDefinitionTable->FindRow<FNPCDefinitionRow>(TemplateID, ContextString);
		if (!TemplateRow)
		{
			bValid = false;
			if (bLogDetails)
			{
				AINPC_LOG(Error, "[VillageSpawner] Missing NPC template '%s' (%s).", *TemplateID.ToString(), ContextLabel);
			}
			return;
		}

		if (!TemplateRow->PawnClass)
		{
			bValid = false;
			if (bLogDetails)
			{
				AINPC_LOG(Error, "[VillageSpawner] Template '%s' has no PawnClass (%s).", *TemplateID.ToString(), ContextLabel);
			}
		}
	};

	for (const FVillageRequiredRoleEntry& RequiredRole : ScenarioRow->RequiredRoles)
	{
		if (RequiredRole.SlotID.IsNone() || RequiredRole.DefinitionTemplateID.IsNone() || RequiredRole.Count <= 0)
		{
			bValid = false;
			if (bLogDetails)
			{
				AINPC_LOG(Error, "[VillageSpawner] Invalid required role entry in scenario '%s'.", *EffectiveRowID.ToString());
			}
			continue;
		}

		KnownSlotIDs.Add(RequiredRole.SlotID);
		ValidateTemplateRef(RequiredRole.DefinitionTemplateID, TEXT("RequiredRole"));
	}

	for (const FVillageOptionalRoleEntry& OptionalRole : ScenarioRow->OptionalRoles)
	{
		if (OptionalRole.SlotID.IsNone())
		{
			bValid = false;
			if (bLogDetails)
			{
				AINPC_LOG(Error, "[VillageSpawner] Optional role has empty SlotID in scenario '%s'.", *EffectiveRowID.ToString());
			}
			continue;
		}

		KnownSlotIDs.Add(OptionalRole.SlotID);
		if (OptionalRole.MaxCount < OptionalRole.MinCount)
		{
			bValid = false;
			if (bLogDetails)
			{
				AINPC_LOG(Error, "[VillageSpawner] Optional role '%s' has MaxCount < MinCount.",
					*OptionalRole.SlotID.ToString());
			}
		}

		if (OptionalRole.MaxCount > 0 && OptionalRole.CandidateTemplateIDs.Num() == 0)
		{
			bValid = false;
			if (bLogDetails)
			{
				AINPC_LOG(Error, "[VillageSpawner] Optional role '%s' has no candidate templates.",
					*OptionalRole.SlotID.ToString());
			}
		}

		for (const FName CandidateTemplate : OptionalRole.CandidateTemplateIDs)
		{
			if (!CandidateTemplate.IsNone())
			{
				ValidateTemplateRef(CandidateTemplate, TEXT("OptionalRole"));
			}
		}
	}

	auto IsKnownSlot = [&KnownSlotIDs](FName SlotKey) -> bool
	{
		if (KnownSlotIDs.Contains(SlotKey))
		{
			return true;
		}

		const FString SlotString = SlotKey.ToString();
		int32 LastUnderscoreIndex = INDEX_NONE;
		if (SlotString.FindLastChar(TEXT('_'), LastUnderscoreIndex) && LastUnderscoreIndex > 0)
		{
			const FName BaseSlot(*SlotString.Left(LastUnderscoreIndex));
			return KnownSlotIDs.Contains(BaseSlot);
		}

		return false;
	};

	for (const FVillageScenarioRelationshipSeed& Seed : ScenarioRow->RelationshipSeeds)
	{
		if (Seed.SourceSlotID.IsNone() || Seed.TargetSlotID.IsNone())
		{
			bValid = false;
			if (bLogDetails)
			{
				AINPC_LOG(Error, "[VillageSpawner] Relationship seed has empty SourceSlotID/TargetSlotID.");
			}
			continue;
		}

		if (!IsKnownSlot(Seed.SourceSlotID))
		{
			bValid = false;
			if (bLogDetails)
			{
				AINPC_LOG(Error, "[VillageSpawner] Relationship seed source slot '%s' is unknown.",
					*Seed.SourceSlotID.ToString());
			}
		}

		if (!IsKnownSlot(Seed.TargetSlotID))
		{
			bValid = false;
			if (bLogDetails)
			{
				AINPC_LOG(Error, "[VillageSpawner] Relationship seed target slot '%s' is unknown.",
					*Seed.TargetSlotID.ToString());
			}
		}
	}

	if (bLogDetails)
	{
		AINPC_LOG(Log, "[VillageSpawner] Scenario '%s' validation: %s",
			*EffectiveRowID.ToString(),
			bValid ? TEXT("PASS") : TEXT("FAIL"));
	}
	return bValid;
}

bool AVillageSpawner::SpawnScenario(FName InScenarioRowID)
{
	const FVillageScenarioRow* ScenarioRow = nullptr;
	if (!ResolveScenarioRow(InScenarioRowID, ScenarioRow))
	{
		return false;
	}

	const FName EffectiveRowID = InScenarioRowID.IsNone() ? ScenarioRowID : InScenarioRowID;
	if (!ValidateScenarioTemplate(EffectiveRowID, true))
	{
		return false;
	}

	if (bDestroyPreviousSpawnedNPCsOnRespawn)
	{
		ClearSpawnedNPCs();
	}
	else
	{
		SpawnedNPCs.Reset();
		SpawnedSlotMap.Reset();
	}

	const int32 EffectiveSeed = RandomSeed != 0 ? RandomSeed : static_cast<int32>(FDateTime::UtcNow().GetTicks() & 0x7fffffff);
	FRandomStream Stream(EffectiveSeed);
	TArray<FVector> OccupiedLocations;
	const float SpawnRadius = FMath::Max(100.0f, ScenarioRow->SpawnRadius);
	const float MinSpawnDistance = FMath::Max(0.0f, ScenarioRow->MinSpawnDistance);

	bool bSpawnedAny = false;

	for (const FVillageRequiredRoleEntry& RequiredRole : ScenarioRow->RequiredRoles)
	{
		const int32 SpawnCount = FMath::Max(1, RequiredRole.Count);
		for (int32 Index = 0; Index < SpawnCount; ++Index)
		{
			const FName InstanceSlotID = BuildInstanceSlotID(RequiredRole.SlotID, Index, SpawnCount);
			const FName FixedNameID = RequiredRole.FixedNameIDs.IsValidIndex(Index) ? RequiredRole.FixedNameIDs[Index] : NAME_None;
			const FVector SpawnLocation = BuildSpawnLocation(Stream, SpawnRadius, MinSpawnDistance, OccupiedLocations);

			if (APawn* SpawnedPawn = SpawnNPCFromTemplate(
				RequiredRole.DefinitionTemplateID,
				InstanceSlotID,
				ScenarioRow->VillageID,
				FixedNameID,
				SpawnLocation))
			{
				RegisterSpawnedNPC(InstanceSlotID, SpawnedPawn);
				OccupiedLocations.Add(SpawnLocation);
				bSpawnedAny = true;
			}
		}
	}

	for (const FVillageOptionalRoleEntry& OptionalRole : ScenarioRow->OptionalRoles)
	{
		if (OptionalRole.CandidateTemplateIDs.Num() == 0 || OptionalRole.MaxCount <= 0)
		{
			continue;
		}

		const float SpawnRoll = Stream.FRandRange(0.0f, 1.0f);
		if (SpawnRoll > FMath::Clamp(OptionalRole.SpawnChance, 0.0f, 1.0f))
		{
			continue;
		}

		const int32 MinCount = FMath::Max(0, OptionalRole.MinCount);
		const int32 MaxCount = FMath::Max(MinCount, OptionalRole.MaxCount);
		const int32 SpawnCount = Stream.RandRange(MinCount, MaxCount);
		for (int32 Index = 0; Index < SpawnCount; ++Index)
		{
			const int32 CandidateIndex = Stream.RandRange(0, OptionalRole.CandidateTemplateIDs.Num() - 1);
			const FName TemplateID = OptionalRole.CandidateTemplateIDs[CandidateIndex];
			const FName InstanceSlotID = BuildInstanceSlotID(OptionalRole.SlotID, Index, SpawnCount);
			const FName FixedNameID = OptionalRole.FixedNameIDs.IsValidIndex(Index) ? OptionalRole.FixedNameIDs[Index] : NAME_None;
			const FVector SpawnLocation = BuildSpawnLocation(Stream, SpawnRadius, MinSpawnDistance, OccupiedLocations);

			if (APawn* SpawnedPawn = SpawnNPCFromTemplate(
				TemplateID,
				InstanceSlotID,
				ScenarioRow->VillageID,
				FixedNameID,
				SpawnLocation))
			{
				RegisterSpawnedNPC(InstanceSlotID, SpawnedPawn);
				OccupiedLocations.Add(SpawnLocation);
				bSpawnedAny = true;
			}
		}
	}

	if (!bSpawnedAny)
	{
		AINPC_LOG(Warning, "[VillageSpawner] Scenario '%s' spawned 0 NPCs.", *EffectiveRowID.ToString());
		return false;
	}

	if (UNPCVillageSubsystem* VillageSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UNPCVillageSubsystem>() : nullptr)
	{
		if (bRebuildVillageRelationshipsAfterSpawn && !ScenarioRow->VillageID.IsNone())
		{
			VillageSubsystem->RebuildVillageRelationships(ScenarioRow->VillageID);
		}
	}

	if (bApplyScenarioRelationshipSeeds)
	{
		ApplyScenarioRelationshipSeeds(*ScenarioRow);
	}

	if (bRunProfessionSanityCheckOnSpawn)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ProfessionDiagnosticsTimerHandle);
			World->GetTimerManager().SetTimer(
				ProfessionDiagnosticsTimerHandle,
				this,
				&AVillageSpawner::RunProfessionSanityCheck,
				FMath::Max(0.0f, ProfessionSanityCheckDelay),
				false);
		}
	}

	AINPC_LOG(Log, "[VillageSpawner] Scenario '%s' spawned %d NPC(s).",
		*EffectiveRowID.ToString(),
		SpawnedNPCs.Num());
	return true;
}

void AVillageSpawner::ClearSpawnedNPCs()
{
	for (TObjectPtr<APawn> SpawnedPawn : SpawnedNPCs)
	{
		if (IsValid(SpawnedPawn))
		{
			SpawnedPawn->Destroy();
		}
	}

	SpawnedNPCs.Reset();
	SpawnedSlotMap.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProfessionDiagnosticsTimerHandle);
	}
}

APawn* AVillageSpawner::SpawnNPCFromTemplate(
	const FName TemplateID,
	const FName InstanceSlotID,
	const FName ScenarioVillageID,
	const FName FixedNameID,
	const FVector& SpawnLocation)
{
	if (!NPCDefinitionTable)
	{
		AINPC_LOG(Error, "[VillageSpawner] NPCDefinitionTable is null.");
		return nullptr;
	}

	static const FString ContextString(TEXT("VillageSpawner_SpawnNPCFromTemplate"));
	const FNPCDefinitionRow* TemplateRow = NPCDefinitionTable->FindRow<FNPCDefinitionRow>(TemplateID, ContextString);
	if (!TemplateRow)
	{
		AINPC_LOG(Error, "[VillageSpawner] Missing NPC template row '%s'.", *TemplateID.ToString());
		return nullptr;
	}

	if (!TemplateRow->PawnClass)
	{
		AINPC_LOG(Error, "[VillageSpawner] Template '%s' has no PawnClass.", *TemplateID.ToString());
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FTransform SpawnTransform(GetActorRotation(), SpawnLocation);
	APawn* SpawnedPawn = World->SpawnActorDeferred<APawn>(
		TemplateRow->PawnClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!SpawnedPawn)
	{
		AINPC_LOG(Error, "[VillageSpawner] Failed to spawn pawn for template '%s'.", *TemplateID.ToString());
		return nullptr;
	}

	if (UNPCDefinitionComponent* DefComp = SpawnedPawn->FindComponentByClass<UNPCDefinitionComponent>())
	{
		DefComp->DefinitionTable = NPCDefinitionTable;
		DefComp->DefinitionTemplateID = TemplateID;
		DefComp->bSkipTemplateLoadOnBeginPlay = true;
		DefComp->PersonalityID = TemplateRow->PersonalityID;
		DefComp->ProfessionID = TemplateRow->ProfessionID;
		DefComp->FactionID = TemplateRow->FactionID;
		DefComp->VillageID = (bForceVillageIDOverride && !ScenarioVillageID.IsNone()) ? ScenarioVillageID : TemplateRow->VillageID;

		if (!FixedNameID.IsNone())
		{
			DefComp->SetNameIDOverride(FixedNameID);
		}
	}
	else
	{
		AINPC_LOG(Warning, "[VillageSpawner] Spawned pawn '%s' has no NPCDefinitionComponent.", *SpawnedPawn->GetName());
	}

	SpawnedPawn = Cast<APawn>(UGameplayStatics::FinishSpawningActor(SpawnedPawn, SpawnTransform));
	if (IsValid(SpawnedPawn))
	{
		SpawnedPawn->Tags.AddUnique(FName(TEXT("VillageResident")));
		SpawnedPawn->Tags.AddUnique(FName(*FString::Printf(TEXT("VillageSlot.%s"), *InstanceSlotID.ToString())));
	}

	return SpawnedPawn;
}

void AVillageSpawner::RegisterSpawnedNPC(FName InstanceSlotID, APawn* SpawnedPawn)
{
	if (!IsValid(SpawnedPawn))
	{
		return;
	}

	SpawnedNPCs.Add(SpawnedPawn);
	SpawnedSlotMap.FindOrAdd(InstanceSlotID).Pawns.Add(SpawnedPawn);
}

void AVillageSpawner::CollectActorsBySlotKey(FName SlotKey, TArray<APawn*>& OutActors) const
{
	OutActors.Reset();
	if (SlotKey.IsNone())
	{
		return;
	}

	if (const FSpawnedPawnList* ExactActors = SpawnedSlotMap.Find(SlotKey))
	{
		for (const TObjectPtr<APawn> Pawn : ExactActors->Pawns)
		{
			if (IsValid(Pawn))
			{
				OutActors.AddUnique(Pawn);
			}
		}
	}

	const FString Prefix = SlotKey.ToString() + TEXT("_");
	for (const TPair<FName, FSpawnedPawnList>& Pair : SpawnedSlotMap)
	{
		if (!Pair.Key.ToString().StartsWith(Prefix))
		{
			continue;
		}

		for (const TObjectPtr<APawn> Pawn : Pair.Value.Pawns)
		{
			if (IsValid(Pawn))
			{
				OutActors.AddUnique(Pawn);
			}
		}
	}
}

void AVillageSpawner::ApplyScenarioRelationshipSeeds(const FVillageScenarioRow& ScenarioRow)
{
	for (const FVillageScenarioRelationshipSeed& Seed : ScenarioRow.RelationshipSeeds)
	{
		TArray<APawn*> SourceActors;
		TArray<APawn*> TargetActors;
		CollectActorsBySlotKey(Seed.SourceSlotID, SourceActors);
		CollectActorsBySlotKey(Seed.TargetSlotID, TargetActors);

		if (SourceActors.Num() == 0 || TargetActors.Num() == 0)
		{
			AINPC_LOG(Warning,
				"[VillageSpawner] Relationship seed skipped: SourceSlot=%s (%d), TargetSlot=%s (%d).",
				*Seed.SourceSlotID.ToString(),
				SourceActors.Num(),
				*Seed.TargetSlotID.ToString(),
				TargetActors.Num());
			continue;
		}

		for (APawn* SourcePawn : SourceActors)
		{
			for (APawn* TargetPawn : TargetActors)
			{
				if (!IsValid(SourcePawn) || !IsValid(TargetPawn) || SourcePawn == TargetPawn)
				{
					continue;
				}

				ApplySeedDirection(SourcePawn, TargetPawn, Seed);
				if (Seed.bBidirectional)
				{
					ApplySeedDirection(TargetPawn, SourcePawn, Seed);
				}
			}
		}
	}
}

void AVillageSpawner::ApplySeedDirection(APawn* SourcePawn, APawn* TargetPawn, const FVillageScenarioRelationshipSeed& Seed)
{
	if (!IsValid(SourcePawn) || !IsValid(TargetPawn))
	{
		return;
	}

	UFactionReputationComponent* SourceRep = SourcePawn->FindComponentByClass<UFactionReputationComponent>();
	if (!SourceRep)
	{
		return;
	}

	const float ClampedAttitude = FMath::Clamp(Seed.InitialAttitude, 0.0f, 100.0f);
	const FString SummaryText = Seed.InitialSummary.IsEmpty()
		? FString::Printf(TEXT("%s has a predefined relationship with %s."), *SourcePawn->GetName(), *TargetPawn->GetName())
		: Seed.InitialSummary;

	SourceRep->EnsureInitialRelationshipWith(TargetPawn, ClampedAttitude, SummaryText);
	SourceRep->PersonalReputations.Add(TargetPawn, ClampedAttitude);

	const FName TargetID = SourceRep->GetStableSocialID(TargetPawn);
	if (TargetID.IsNone())
	{
		return;
	}

	FSocialBond* Bond = SourceRep->SocialBonds.Find(TargetID);
	if (!Bond)
	{
		return;
	}

	if (Seed.InitialBondType != ESocialBondType::None)
	{
		Bond->Type = Seed.InitialBondType;
	}
	Bond->BondSalience = FMath::Clamp(Seed.InitialSalience, 0, 10);
	Bond->RelationshipSummary = SummaryText;
	Bond->LastInteractionTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

FName AVillageSpawner::BuildInstanceSlotID(FName BaseSlotID, int32 Index, int32 Count) const
{
	if (BaseSlotID.IsNone())
	{
		return FName(TEXT("Slot"));
	}

	if (Count <= 1)
	{
		return BaseSlotID;
	}

	return FName(*FString::Printf(TEXT("%s_%02d"), *BaseSlotID.ToString(), Index + 1));
}

FVector AVillageSpawner::BuildSpawnLocation(
	FRandomStream& Stream,
	float SpawnRadius,
	float MinSpawnDistance,
	TArray<FVector>& OccupiedLocations) const
{
	const FVector Origin = GetActorLocation();
	const float MinDistSq = MinSpawnDistance * MinSpawnDistance;

	for (int32 Attempt = 0; Attempt < 24; ++Attempt)
	{
		const float Angle = Stream.FRandRange(0.0f, 2.0f * PI);
		const float Dist = FMath::Sqrt(Stream.FRand()) * SpawnRadius;
		FVector Candidate = Origin + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);
		Candidate.Z = Origin.Z;

		bool bTooClose = false;
		if (MinSpawnDistance > 0.0f)
		{
			for (const FVector& Existing : OccupiedLocations)
			{
				if (FVector::DistSquared2D(Candidate, Existing) < MinDistSq)
				{
					bTooClose = true;
					break;
				}
			}
		}

		if (!bTooClose)
		{
			return Candidate;
		}
	}

	const float FallbackOffset = MinSpawnDistance * (OccupiedLocations.Num() + 1);
	return Origin + FVector(FallbackOffset, 0.0f, 0.0f);
}

void AVillageSpawner::RunProfessionSanityCheck()
{
	AINPC_LOG(Log, "[VillageSpawner][Diag] Profession sanity check begin. Spawned=%d", SpawnedNPCs.Num());

	for (TObjectPtr<APawn> SpawnedPawn : SpawnedNPCs)
	{
		if (!IsValid(SpawnedPawn))
		{
			continue;
		}

		const UNPCDefinitionComponent* DefComp = SpawnedPawn->FindComponentByClass<UNPCDefinitionComponent>();
		UGoalComponent* GoalComp = SpawnedPawn->FindComponentByClass<UGoalComponent>();
		UUtilityAIComponent* UtilityComp = SpawnedPawn->FindComponentByClass<UUtilityAIComponent>();

		if (AController* Controller = SpawnedPawn->GetController())
		{
			if (!GoalComp)
			{
				GoalComp = Controller->FindComponentByClass<UGoalComponent>();
			}
			if (!UtilityComp)
			{
				UtilityComp = Controller->FindComponentByClass<UUtilityAIComponent>();
			}
		}

		const FString TemplateID = DefComp ? DefComp->DefinitionTemplateID.ToString() : TEXT("None");
		const FString ProfessionID = DefComp ? DefComp->ProfessionID.ToString() : TEXT("None");
		const FString GoalDirective = (GoalComp && GoalComp->GetCurrentDirective().IsValid())
			? GoalComp->GetCurrentDirective().ToString()
			: TEXT("None");
		const FString GoalActivity = (GoalComp && GoalComp->GetScheduledActivity().IsValid())
			? GoalComp->GetScheduledActivity().ToString()
			: TEXT("None");
		const int32 UtilityActionCount = UtilityComp ? UtilityComp->AvailableActions.Num() : -1;
		const FString CurrentActionName = (UtilityComp && UtilityComp->CurrentAction)
			? UtilityComp->CurrentAction->ActionName
			: TEXT("None");

		AINPC_LOG(Log,
			"[VillageSpawner][Diag] NPC=%s Template=%s Profession=%s GoalDirective=%s GoalActivity=%s UtilityActions=%d CurrentAction=%s",
			*SpawnedPawn->GetName(),
			*TemplateID,
			*ProfessionID,
			*GoalDirective,
			*GoalActivity,
			UtilityActionCount,
			*CurrentActionName);
	}
}
