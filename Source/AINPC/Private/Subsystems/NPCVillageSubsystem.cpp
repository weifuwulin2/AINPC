#include "Subsystems/NPCVillageSubsystem.h"

#include "AINPC.h"
#include "Components/FactionReputationComponent.h"
#include "Components/NPCDefinitionComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Social/FactionSubsystem.h"
#include "Utilities/FactionHelpers.h"

void UNPCVillageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	VillageMembers.Empty();
	ActorVillageMap.Empty();
	TerritoryClaims.Empty();

	AINPC_VILLAGE_LOG(Log, "[NPCVillage] Initialized");
}

void UNPCVillageSubsystem::Deinitialize()
{
	const int32 MemberCountBeforeClear = ActorVillageMap.Num();
	const int32 ClaimCountBeforeClear = TerritoryClaims.Num();

	for (const auto& Pair : ActorVillageMap)
	{
		if (AActor* NPC = Pair.Key)
		{
			if (IsValid(NPC))
			{
				NPC->OnDestroyed.RemoveDynamic(this, &UNPCVillageSubsystem::OnVillageNPCDestroyed);
			}
		}
	}

	for (const FVillageTerritoryClaim& Claim : TerritoryClaims)
	{
		if (AActor* SourceActor = Claim.SourceActor)
		{
			if (IsValid(SourceActor))
			{
				SourceActor->OnDestroyed.RemoveDynamic(this, &UNPCVillageSubsystem::OnTerritorySourceDestroyed);
			}
		}
	}

	VillageMembers.Empty();
	ActorVillageMap.Empty();
	TerritoryClaims.Empty();
	AINPC_VILLAGE_LOG(Log, "[NPCVillage] Deinitialized: RemovedMembers=%d RemovedClaims=%d",
		MemberCountBeforeClear,
		ClaimCountBeforeClear);
	Super::Deinitialize();
}

void UNPCVillageSubsystem::RegisterNPC(AActor* NPC, FName VillageID)
{
	if (!IsValid(NPC))
	{
		AINPC_VILLAGE_LOG_WARNING("[NPCVillage] RegisterNPC skipped: NPC is invalid");
		return;
	}

	CleanupInvalidTerritoryClaims();

	FName FinalVillageID = ResolveVillageID(VillageID);
	if (FinalVillageID.IsNone() && bResolveVillageFromTerritoryWhenMissing)
	{
		FinalVillageID = ResolveVillageByLocation(NPC->GetActorLocation());
	}

	if (const FName* ExistingVillage = ActorVillageMap.Find(NPC))
	{
		if (FinalVillageID.IsNone())
		{
			AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] RegisterNPC unresolved village, unregistering existing: NPC=%s ExistingVillage=%s",
				*NPC->GetName(),
				*ExistingVillage->ToString());
			UnregisterNPC(NPC);
			return;
		}

		if (*ExistingVillage == FinalVillageID)
		{
			if (bAutoBootstrapRelationshipsOnRegister)
			{
				BootstrapRelationshipsFor(NPC, FinalVillageID);
			}
			AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] RegisterNPC no-op (already in village): NPC=%s Village=%s",
				*NPC->GetName(),
				*FinalVillageID.ToString());
			return;
		}

		AINPC_VILLAGE_LOG(Log, "[NPCVillage] RegisterNPC village migration: NPC=%s %s -> %s",
			*NPC->GetName(),
			*ExistingVillage->ToString(),
			*FinalVillageID.ToString());
		UnregisterNPC(NPC);
	}

	if (FinalVillageID.IsNone())
	{
		// Explicitly support "no village group" for anchor-script-first scenarios.
		AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] RegisterNPC skipped: no resolved village for NPC=%s InputVillage=%s",
			*NPC->GetName(),
			*VillageID.ToString());
		return;
	}

	FVillageMemberList& Members = VillageMembers.FindOrAdd(FinalVillageID);
	Members.Members.AddUnique(NPC);
	ActorVillageMap.Add(NPC, FinalVillageID);
	NPC->OnDestroyed.RemoveDynamic(this, &UNPCVillageSubsystem::OnVillageNPCDestroyed);
	NPC->OnDestroyed.AddDynamic(this, &UNPCVillageSubsystem::OnVillageNPCDestroyed);

	AINPC_VILLAGE_LOG(Log, "[NPCVillage] Registered %s to village %s", *NPC->GetName(), *FinalVillageID.ToString());

	if (bAutoBootstrapRelationshipsOnRegister)
	{
		BootstrapRelationshipsFor(NPC, FinalVillageID);
	}
}

void UNPCVillageSubsystem::UnregisterNPC(AActor* NPC)
{
	if (!NPC)
	{
		return;
	}

	const FName* ExistingVillage = ActorVillageMap.Find(NPC);
	if (!ExistingVillage)
	{
		AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] UnregisterNPC skipped: NPC=%s not found in ActorVillageMap", *NPC->GetName());
		return;
	}

	if (FVillageMemberList* Members = VillageMembers.Find(*ExistingVillage))
	{
		Members->Members.Remove(NPC);
		if (Members->Members.Num() == 0)
		{
			VillageMembers.Remove(*ExistingVillage);
		}
	}

	ActorVillageMap.Remove(NPC);
	NPC->OnDestroyed.RemoveDynamic(this, &UNPCVillageSubsystem::OnVillageNPCDestroyed);
	AINPC_VILLAGE_LOG(Log, "[NPCVillage] Unregistered %s from village %s",
		*NPC->GetName(),
		*ExistingVillage->ToString());
}

void UNPCVillageSubsystem::RebuildVillageRelationships(FName VillageID)
{
	const FName FinalVillageID = ResolveVillageID(VillageID);
	if (FinalVillageID.IsNone())
	{
		AINPC_VILLAGE_LOG_WARNING("[NPCVillage] RebuildVillageRelationships skipped: invalid VillageID=%s", *VillageID.ToString());
		return;
	}

	CleanupInvalidMembers(FinalVillageID);

	TArray<AActor*> Snapshot;
	GetVillageMembers(FinalVillageID, Snapshot);
	AINPC_VILLAGE_LOG(Log, "[NPCVillage] RebuildVillageRelationships: Village=%s Members=%d",
		*FinalVillageID.ToString(),
		Snapshot.Num());
	for (AActor* NPC : Snapshot)
	{
		BootstrapRelationshipsFor(NPC, FinalVillageID);
	}
}

void UNPCVillageSubsystem::GetVillageMembers(FName VillageID, TArray<AActor*>& OutMembers) const
{
	OutMembers.Reset();
	const FName FinalVillageID = ResolveVillageID(VillageID);
	if (FinalVillageID.IsNone())
	{
		return;
	}

	const FVillageMemberList* Members = VillageMembers.Find(FinalVillageID);
	if (!Members)
	{
		return;
	}

	for (AActor* Member : Members->Members)
	{
		if (IsValid(Member))
		{
			OutMembers.Add(Member);
		}
	}
}

FName UNPCVillageSubsystem::GetVillageIDForNPC(AActor* NPC) const
{
	if (!NPC)
	{
		return NAME_None;
	}

	if (const FName* VillageID = ActorVillageMap.Find(NPC))
	{
		return *VillageID;
	}

	return NAME_None;
}

void UNPCVillageSubsystem::ClaimTerritory(
	FName VillageID,
	FVector Center,
	float Radius,
	int32 Priority,
	AActor* SourceActor)
{
	if (VillageID.IsNone() || VillageID == FName(TEXT("None")))
	{
		AINPC_VILLAGE_LOG_WARNING("[NPCVillage] ClaimTerritory skipped: invalid VillageID");
		return;
	}

	CleanupInvalidTerritoryClaims();

	if (SourceActor)
	{
		RevokeTerritoryBySource(SourceActor);
		SourceActor->OnDestroyed.RemoveDynamic(this, &UNPCVillageSubsystem::OnTerritorySourceDestroyed);
		SourceActor->OnDestroyed.AddDynamic(this, &UNPCVillageSubsystem::OnTerritorySourceDestroyed);
		Center = SourceActor->GetActorLocation();
	}

	FVillageTerritoryClaim Claim;
	Claim.VillageID = VillageID;
	Claim.Center = Center;
	Claim.Radius = FMath::Max(100.0f, Radius);
	Claim.Priority = Priority;
	Claim.SourceActor = SourceActor;
	TerritoryClaims.Add(Claim);

	AINPC_VILLAGE_LOG(Log, "[NPCVillage] Territory claim added: Village=%s Radius=%.0f Priority=%d Source=%s",
		*VillageID.ToString(),
		Claim.Radius,
		Priority,
		SourceActor ? *SourceActor->GetName() : TEXT("None"));

	if (bAutoRefreshNPCOnTerritoryClaim)
	{
		RefreshAllNPCVillageAssignments();
	}
}

void UNPCVillageSubsystem::ClaimTerritoryFromActor(FName VillageID, AActor* SourceActor, float Radius, int32 Priority)
{
	if (!SourceActor)
	{
		AINPC_VILLAGE_LOG_WARNING("[NPCVillage] ClaimTerritoryFromActor skipped: SourceActor is null");
		return;
	}

	ClaimTerritory(VillageID, SourceActor->GetActorLocation(), Radius, Priority, SourceActor);
}

void UNPCVillageSubsystem::RevokeTerritoryBySource(AActor* SourceActor)
{
	if (!SourceActor)
	{
		AINPC_VILLAGE_LOG_WARNING("[NPCVillage] RevokeTerritoryBySource skipped: SourceActor is null");
		return;
	}

	int32 RemovedCount = 0;
	bool bRemovedAny = false;
	for (int32 Index = TerritoryClaims.Num() - 1; Index >= 0; --Index)
	{
		const FVillageTerritoryClaim& Claim = TerritoryClaims[Index];
		if (Claim.SourceActor == SourceActor)
		{
			TerritoryClaims.RemoveAtSwap(Index);
			bRemovedAny = true;
			++RemovedCount;
		}
	}

	SourceActor->OnDestroyed.RemoveDynamic(this, &UNPCVillageSubsystem::OnTerritorySourceDestroyed);
	if (bRemovedAny)
	{
		AINPC_VILLAGE_LOG(Log, "[NPCVillage] Territory revoked: Source=%s RemovedClaims=%d",
			*SourceActor->GetName(),
			RemovedCount);
	}

	if (bRemovedAny && bAutoRefreshNPCOnTerritoryClaim)
	{
		RefreshAllNPCVillageAssignments();
	}
}

FName UNPCVillageSubsystem::ResolveVillageByLocation(FVector Location) const
{
	float BestScore = -FLT_MAX;
	FName BestVillageID = NAME_None;

	for (const FVillageTerritoryClaim& Claim : TerritoryClaims)
	{
		if (Claim.VillageID.IsNone() || Claim.Radius <= 0.0f)
		{
			continue;
		}

		const FVector ClaimCenter = Claim.SourceActor ? Claim.SourceActor->GetActorLocation() : Claim.Center;
		const float DistanceSq = FVector::DistSquared(Location, ClaimCenter);
		const float RadiusSq = Claim.Radius * Claim.Radius;
		if (DistanceSq > RadiusSq)
		{
			continue;
		}

		const float Score = static_cast<float>(Claim.Priority) * 100000000.0f - DistanceSq;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestVillageID = Claim.VillageID;
		}
	}

	return BestVillageID;
}

void UNPCVillageSubsystem::RefreshNPCVillageByLocation(AActor* NPC)
{
	if (!IsValid(NPC))
	{
		AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] RefreshNPCVillageByLocation skipped: invalid NPC");
		return;
	}

	const FName PreviousVillageID = GetVillageIDForNPC(NPC);
	const FName ResolvedVillageID = ResolveVillageByLocation(NPC->GetActorLocation());

	if (ResolvedVillageID.IsNone())
	{
		if (!PreviousVillageID.IsNone())
		{
			AINPC_VILLAGE_LOG(Log, "[NPCVillage] Refresh by location removed village assignment: NPC=%s OldVillage=%s",
				*NPC->GetName(),
				*PreviousVillageID.ToString());
			UnregisterNPC(NPC);
		}
		return;
	}

	if (ResolvedVillageID != PreviousVillageID)
	{
		AINPC_VILLAGE_LOG(Log, "[NPCVillage] Refresh by location village migration: NPC=%s %s -> %s",
			*NPC->GetName(),
			*PreviousVillageID.ToString(),
			*ResolvedVillageID.ToString());
		RegisterNPC(NPC, ResolvedVillageID);
		return;
	}

	if (bAutoBootstrapRelationshipsOnRegister && !ResolvedVillageID.IsNone())
	{
		BootstrapRelationshipsFor(NPC, ResolvedVillageID);
	}
}

void UNPCVillageSubsystem::RefreshAllNPCVillageAssignments()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		AINPC_VILLAGE_LOG_WARNING("[NPCVillage] RefreshAllNPCVillageAssignments skipped: World is null");
		return;
	}

	int32 RefreshedCount = 0;
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!IsValid(Pawn))
		{
			continue;
		}

		if (Pawn->FindComponentByClass<UNPCDefinitionComponent>())
		{
			RefreshNPCVillageByLocation(Pawn);
			++RefreshedCount;
		}
	}

	AINPC_VILLAGE_LOG(Log, "[NPCVillage] RefreshAllNPCVillageAssignments complete: Refreshed=%d", RefreshedCount);
}

void UNPCVillageSubsystem::OnVillageNPCDestroyed(AActor* DestroyedActor)
{
	AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] OnVillageNPCDestroyed: %s", DestroyedActor ? *DestroyedActor->GetName() : TEXT("None"));
	UnregisterNPC(DestroyedActor);
}

void UNPCVillageSubsystem::OnTerritorySourceDestroyed(AActor* DestroyedActor)
{
	AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] OnTerritorySourceDestroyed: %s", DestroyedActor ? *DestroyedActor->GetName() : TEXT("None"));
	RevokeTerritoryBySource(DestroyedActor);
}

FName UNPCVillageSubsystem::ResolveVillageID(FName InVillageID) const
{
	if (!InVillageID.IsNone() && InVillageID != FName(TEXT("None")))
	{
		return InVillageID;
	}

	if (bUseDefaultVillageIDWhenMissing)
	{
		return DefaultVillageID.IsNone() ? FName(TEXT("Village.Default")) : DefaultVillageID;
	}

	return NAME_None;
}

FName UNPCVillageSubsystem::ResolveStableID(const AActor* NPC) const
{
	if (!NPC)
	{
		return NAME_None;
	}

	if (const UFactionReputationComponent* FactionComp = NPC->FindComponentByClass<UFactionReputationComponent>())
	{
		FName StableID = FactionComp->GetStableSocialID(const_cast<AActor*>(NPC));
		if (!StableID.IsNone() && StableID != FName(TEXT("None")))
		{
			return StableID;
		}
	}

	if (const UNPCDefinitionComponent* DefComp = NPC->FindComponentByClass<UNPCDefinitionComponent>())
	{
		const FName NameID = DefComp->GetNameID();
		if (!NameID.IsNone() && NameID != FName(TEXT("None")))
		{
			return NameID;
		}
	}

	return NPC->GetFName();
}

ENPCVillageSocialTier UNPCVillageSubsystem::ResolveTier(const AActor* NPC) const
{
	if (!NPC)
	{
		return ENPCVillageSocialTier::Commoner;
	}

	const UNPCDefinitionComponent* DefComp = NPC->FindComponentByClass<UNPCDefinitionComponent>();
	if (!DefComp || DefComp->ProfessionID.IsNone())
	{
		return ENPCVillageSocialTier::Commoner;
	}

	for (const FName LeaderID : LeaderProfessionIDs)
	{
		if (!LeaderID.IsNone() && DefComp->ProfessionID == LeaderID)
		{
			return ENPCVillageSocialTier::Leader;
		}
	}

	for (const FName NobleID : NobleProfessionIDs)
	{
		if (!NobleID.IsNone() && DefComp->ProfessionID == NobleID)
		{
			return ENPCVillageSocialTier::Noble;
		}
	}

	return ENPCVillageSocialTier::Commoner;
}

void UNPCVillageSubsystem::CleanupInvalidTerritoryClaims()
{
	int32 Removed = 0;
	for (int32 Index = TerritoryClaims.Num() - 1; Index >= 0; --Index)
	{
		const FVillageTerritoryClaim& Claim = TerritoryClaims[Index];
		if (Claim.SourceActor && !IsValid(Claim.SourceActor))
		{
			TerritoryClaims.RemoveAtSwap(Index);
			++Removed;
		}
	}

	if (Removed > 0)
	{
		AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] CleanupInvalidTerritoryClaims removed %d stale claim(s)", Removed);
	}
}

void UNPCVillageSubsystem::CleanupInvalidMembers(FName VillageID)
{
	FVillageMemberList* Members = VillageMembers.Find(VillageID);
	if (!Members)
	{
		return;
	}

	int32 Removed = 0;
	for (int32 Index = Members->Members.Num() - 1; Index >= 0; --Index)
	{
		AActor* Member = Members->Members[Index];
		if (!IsValid(Member))
		{
			Members->Members.RemoveAtSwap(Index);
			ActorVillageMap.Remove(Member);
			++Removed;
		}
	}

	if (Members->Members.Num() == 0)
	{
		VillageMembers.Remove(VillageID);
	}

	if (Removed > 0)
	{
		AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] CleanupInvalidMembers: Village=%s Removed=%d",
			*VillageID.ToString(),
			Removed);
	}
}

void UNPCVillageSubsystem::BootstrapRelationshipsFor(AActor* NPC, FName VillageID)
{
	if (!IsValid(NPC))
	{
		AINPC_VILLAGE_LOG_WARNING("[NPCVillage] BootstrapRelationshipsFor skipped: NPC invalid");
		return;
	}

	CleanupInvalidMembers(VillageID);

	UFactionReputationComponent* SourceRep = NPC->FindComponentByClass<UFactionReputationComponent>();
	if (!SourceRep)
	{
		AINPC_VILLAGE_LOG(Warning, "[NPCVillage] Bootstrap skipped: Source NPC has no UFactionReputationComponent. NPC=%s",
			*NPC->GetName());
		return;
	}

	FVillageMemberList* Members = VillageMembers.Find(VillageID);
	if (!Members)
	{
		AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] Bootstrap skipped: Village has no members array. Village=%s",
			*VillageID.ToString());
		return;
	}

	int32 AppliedPairCount = 0;
	for (AActor* Other : Members->Members)
	{
		if (!IsValid(Other) || Other == NPC)
		{
			continue;
		}

		UFactionReputationComponent* OtherRep = Other->FindComponentByClass<UFactionReputationComponent>();
		if (!OtherRep)
		{
			continue;
		}

		const float SourceToTarget = ComputeInitialAttitude(NPC, Other, VillageID);
		const float TargetToSource = ComputeInitialAttitude(Other, NPC, VillageID);

		SourceRep->EnsureInitialRelationshipWith(Other, SourceToTarget, BuildVillageSummary(NPC, Other, VillageID));
		OtherRep->EnsureInitialRelationshipWith(NPC, TargetToSource, BuildVillageSummary(Other, NPC, VillageID));
		++AppliedPairCount;
	}

	AINPC_VILLAGE_LOG_VERBOSE("[NPCVillage] BootstrapRelationshipsFor complete: NPC=%s Village=%s Pairs=%d",
		*NPC->GetName(),
		*VillageID.ToString(),
		AppliedPairCount);
}

float UNPCVillageSubsystem::ComputeInitialAttitude(AActor* Source, AActor* Target, FName VillageID) const
{
	if (!Source || !Target)
	{
		return 50.0f;
	}

	float Attitude = DefaultVillageAttitude;
	if (UWorld* World = GetWorld())
	{
		if (UFactionSubsystem* FactionSubsystem = World->GetSubsystem<UFactionSubsystem>())
		{
			const FName SourceFaction = FactionHelpers::GetFactionID(Source);
			const FName TargetFaction = FactionHelpers::GetFactionID(Target);
			Attitude = FactionSubsystem->GetBaseAttitude(SourceFaction, TargetFaction);
		}
	}

	const ENPCVillageSocialTier SourceTier = ResolveTier(Source);
	const ENPCVillageSocialTier TargetTier = ResolveTier(Target);

	if (SourceTier == TargetTier)
	{
		Attitude += 4.0f;
	}
	else if (SourceTier == ENPCVillageSocialTier::Leader && TargetTier == ENPCVillageSocialTier::Commoner)
	{
		Attitude += 8.0f;
	}
	else if (SourceTier == ENPCVillageSocialTier::Commoner && TargetTier == ENPCVillageSocialTier::Leader)
	{
		Attitude += 12.0f;
	}
	else if (SourceTier == ENPCVillageSocialTier::Leader && TargetTier == ENPCVillageSocialTier::Noble)
	{
		Attitude += 5.0f;
	}
	else if (SourceTier == ENPCVillageSocialTier::Noble && TargetTier == ENPCVillageSocialTier::Leader)
	{
		Attitude += 7.0f;
	}
	else if (SourceTier == ENPCVillageSocialTier::Noble && TargetTier == ENPCVillageSocialTier::Commoner)
	{
		Attitude += 2.0f;
	}

	const FName SourceID = ResolveStableID(Source);
	const FName TargetID = ResolveStableID(Target);

	uint32 Seed = GetTypeHash(VillageID);
	Seed = HashCombine(Seed, GetTypeHash(SourceID));
	Seed = HashCombine(Seed, GetTypeHash(TargetID));
	FRandomStream Stream(static_cast<int32>(Seed & 0x7fffffff));

	const float Noise = Stream.FRandRange(-VillageAttitudeNoiseRange, VillageAttitudeNoiseRange);
	Attitude += Noise;

	return FMath::Clamp(Attitude, 0.0f, 100.0f);
}

FString UNPCVillageSubsystem::BuildVillageSummary(const AActor* Source, const AActor* Target, FName VillageID) const
{
	const FName SourceID = ResolveStableID(Source);
	const FName TargetID = ResolveStableID(Target);

	return FString::Printf(
		TEXT("%s and %s are both residents of %s."),
		*SourceID.ToString(),
		*TargetID.ToString(),
		*VillageID.ToString());
}
