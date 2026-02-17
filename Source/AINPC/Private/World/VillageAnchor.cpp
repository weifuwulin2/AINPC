#include "World/VillageAnchor.h"

#include "AINPC.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Subsystems/NPCVillageSubsystem.h"

AVillageAnchor::AVillageAnchor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	TerritorySphere = CreateDefaultSubobject<USphereComponent>(TEXT("TerritorySphere"));
	TerritorySphere->SetupAttachment(RootComponent);
	TerritorySphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TerritorySphere->SetCollisionObjectType(ECC_WorldDynamic);
	TerritorySphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	TerritorySphere->SetGenerateOverlapEvents(false);
	TerritorySphere->bHiddenInGame = false;
	TerritorySphere->SetSphereRadius(BaseClaimRadius);

	CurrentClaimRadius = BaseClaimRadius;
	TargetClaimRadius = BaseClaimRadius;
	CurrentScaleMultiplier = 1.0f;

	if (BuildingTags.Num() == 0)
	{
		BuildingTags.Add(FName(TEXT("Building")));
		BuildingTags.Add(FName(TEXT("House")));
	}
}

void AVillageAnchor::BeginPlay()
{
	Super::BeginPlay();

	AINPC_VILLAGE_LOG(Log,
		"[VillageAnchor] BeginPlay: Anchor=%s VillageID=%s BaseRadius=%.0f MaxRadius=%.0f EventDriven=%s AutoClaim=%s",
		*GetName(),
		*VillageID.ToString(),
		BaseClaimRadius,
		MaxClaimRadius,
		bUseEventDrivenBuildingUpdates ? TEXT("true") : TEXT("false"),
		bAutoClaimOnBeginPlay ? TEXT("true") : TEXT("false"));

	CurrentClaimRadius = FMath::Clamp(BaseClaimRadius, 100.0f, FMath::Max(BaseClaimRadius, MaxClaimRadius));
	TargetClaimRadius = CurrentClaimRadius;
	CurrentScaleMultiplier = 1.0f;
	SetActorScale3D(BaseVisualScale * CurrentScaleMultiplier);
	if (TerritorySphere)
	{
		TerritorySphere->SetSphereRadius(CurrentClaimRadius);
	}

	if (UWorld* World = GetWorld())
	{
		CachedVillageSubsystem = World->GetSubsystem<UNPCVillageSubsystem>();
	}
	else
	{
		AINPC_VILLAGE_LOG_WARNING("[VillageAnchor] BeginPlay world is null for %s", *GetName());
	}

	if (!CachedVillageSubsystem.IsValid() && bAutoClaimOnBeginPlay)
	{
		AINPC_VILLAGE_LOG(Warning, "[VillageAnchor] NPCVillageSubsystem unavailable on BeginPlay: Anchor=%s", *GetName());
	}

	if (bScaleWithTerritoryBuildings && bInitialScanOnBeginPlay)
	{
		if (bUseEventDrivenBuildingUpdates)
		{
			RebuildTrackedBuildingsByScan();
		}
		else
		{
			float MaxDistance = 0.0f;
			CachedBuildingCount = CountBuildingsInTerritory(MaxDistance);
			const float TargetByCount = BaseClaimRadius + CachedBuildingCount * RadiusPerBuilding;
			const float TargetBySpread = CachedBuildingCount > 0 ? FMath::Max(BaseClaimRadius, MaxDistance + BuildingRadiusPadding) : BaseClaimRadius;
			TargetClaimRadius = FMath::Clamp(FMath::Max(TargetByCount, TargetBySpread), BaseClaimRadius, FMath::Max(BaseClaimRadius, MaxClaimRadius));
		}

		CurrentClaimRadius = TargetClaimRadius;
		CurrentScaleMultiplier = CurrentClaimRadius / FMath::Max(100.0f, BaseClaimRadius);
		CurrentScaleMultiplier = FMath::Clamp(CurrentScaleMultiplier, 0.1f, MaxScaleMultiplier);
		SetActorScale3D(BaseVisualScale * CurrentScaleMultiplier);
		if (TerritorySphere)
		{
			TerritorySphere->SetSphereRadius(CurrentClaimRadius);
		}

		AINPC_VILLAGE_LOG(Log,
			"[VillageAnchor] Initial territory scan complete: Anchor=%s Buildings=%d Radius=%.0f",
			*GetName(),
			CachedBuildingCount,
			CurrentClaimRadius);
	}

	if (bScaleWithTerritoryBuildings && bUseEventDrivenBuildingUpdates && bListenWorldSpawnFallback)
	{
		if (UWorld* World = GetWorld())
		{
			WorldActorSpawnedHandle = World->AddOnActorSpawnedHandler(
				FOnActorSpawned::FDelegate::CreateUObject(this, &AVillageAnchor::HandleWorldActorSpawned));
			AINPC_VILLAGE_LOG_VERBOSE("[VillageAnchor] World spawn fallback listener enabled: Anchor=%s", *GetName());
		}
	}

	if (bAutoClaimOnBeginPlay)
	{
		PushClaimToSubsystem(true);
	}
}

void AVillageAnchor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bScaleWithTerritoryBuildings)
	{
		return;
	}

	UpdateBuildingDrivenScale(DeltaSeconds);
}

void AVillageAnchor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (WorldActorSpawnedHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->RemoveOnActorSpawnedHandler(WorldActorSpawnedHandle);
		}
		WorldActorSpawnedHandle.Reset();
	}

	for (const TWeakObjectPtr<AActor>& BuildingPtr : TrackedBuildings)
	{
		if (AActor* Building = BuildingPtr.Get())
		{
			Building->OnDestroyed.RemoveDynamic(this, &AVillageAnchor::OnTrackedBuildingDestroyed);
		}
	}
	TrackedBuildings.Reset();

	if (UNPCVillageSubsystem* VillageSubsystem = CachedVillageSubsystem.Get())
	{
		VillageSubsystem->RevokeTerritoryBySource(this);
		AINPC_VILLAGE_LOG(Log, "[VillageAnchor] EndPlay revoked territory claim: Anchor=%s VillageID=%s",
			*GetName(),
			*VillageID.ToString());
	}

	Super::EndPlay(EndPlayReason);
}

void AVillageAnchor::HandleWorldActorSpawned(AActor* SpawnedActor)
{
	if (!bScaleWithTerritoryBuildings || !bUseEventDrivenBuildingUpdates)
	{
		return;
	}

	NotifyBuildingConstructed(SpawnedActor);
}

void AVillageAnchor::ForceRefreshTerritory()
{
	const float OldRadius = CurrentClaimRadius;
	if (bUseEventDrivenBuildingUpdates)
	{
		RebuildTrackedBuildingsByScan();
	}
	else
	{
		float MaxDistance = 0.0f;
		CachedBuildingCount = CountBuildingsInTerritory(MaxDistance);
		float TargetByCount = BaseClaimRadius + CachedBuildingCount * RadiusPerBuilding;
		float TargetBySpread = CachedBuildingCount > 0 ? FMath::Max(BaseClaimRadius, MaxDistance + BuildingRadiusPadding) : BaseClaimRadius;
		TargetClaimRadius = FMath::Clamp(FMath::Max(TargetByCount, TargetBySpread), BaseClaimRadius, FMath::Max(BaseClaimRadius, MaxClaimRadius));
	}

	CurrentClaimRadius = TargetClaimRadius;
	CurrentScaleMultiplier = CurrentClaimRadius / FMath::Max(100.0f, BaseClaimRadius);
	CurrentScaleMultiplier = FMath::Clamp(CurrentScaleMultiplier, 0.1f, MaxScaleMultiplier);
	SetActorScale3D(BaseVisualScale * CurrentScaleMultiplier);
	if (TerritorySphere)
	{
		TerritorySphere->SetSphereRadius(CurrentClaimRadius);
	}

	AINPC_VILLAGE_LOG(Log,
		"[VillageAnchor] ForceRefreshTerritory: Anchor=%s Buildings=%d Radius %.0f -> %.0f",
		*GetName(),
		CachedBuildingCount,
		OldRadius,
		CurrentClaimRadius);

	PushClaimToSubsystem(true);
}

void AVillageAnchor::NotifyBuildingConstructed(AActor* BuildingActor)
{
	if (!bScaleWithTerritoryBuildings || !bUseEventDrivenBuildingUpdates)
	{
		return;
	}

	if (!IsBuildingCandidate(BuildingActor) || !IsBuildingInsideCurrentTerritory(BuildingActor))
	{
		return;
	}

	const bool bAlreadyTracked = TrackedBuildings.ContainsByPredicate(
		[BuildingActor](const TWeakObjectPtr<AActor>& Entry)
		{
			return Entry.Get() == BuildingActor;
		});

	if (!bAlreadyTracked)
	{
		TrackedBuildings.Add(BuildingActor);
		if (!BuildingActor->OnDestroyed.IsAlreadyBound(this, &AVillageAnchor::OnTrackedBuildingDestroyed))
		{
			BuildingActor->OnDestroyed.AddDynamic(this, &AVillageAnchor::OnTrackedBuildingDestroyed);
		}

		AINPC_VILLAGE_LOG_VERBOSE("[VillageAnchor] Tracked building added: Anchor=%s Building=%s",
			*GetName(),
			*BuildingActor->GetName());
	}

	RecalculateTargetsFromTrackedBuildings();
}

void AVillageAnchor::NotifyBuildingRemoved(AActor* BuildingActor)
{
	bool bAnyRemoved = false;
	for (int32 Index = TrackedBuildings.Num() - 1; Index >= 0; --Index)
	{
		AActor* Tracked = TrackedBuildings[Index].Get();
		if (Tracked != nullptr && Tracked != BuildingActor)
		{
			continue;
		}

		if (IsValid(Tracked))
		{
			Tracked->OnDestroyed.RemoveDynamic(this, &AVillageAnchor::OnTrackedBuildingDestroyed);
		}
		TrackedBuildings.RemoveAtSwap(Index);
		bAnyRemoved = true;
	}

	if (bAnyRemoved || !IsValid(BuildingActor))
	{
		RecalculateTargetsFromTrackedBuildings();
		AINPC_VILLAGE_LOG_VERBOSE("[VillageAnchor] Tracked building removed/rebuilt: Anchor=%s Remaining=%d",
			*GetName(),
			TrackedBuildings.Num());
	}
}

void AVillageAnchor::RebuildTrackedBuildingsByScan()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& BuildingPtr : TrackedBuildings)
	{
		if (AActor* Building = BuildingPtr.Get())
		{
			Building->OnDestroyed.RemoveDynamic(this, &AVillageAnchor::OnTrackedBuildingDestroyed);
		}
	}
	TrackedBuildings.Reset();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsBuildingCandidate(Candidate) || !IsBuildingInsideCurrentTerritory(Candidate))
		{
			continue;
		}

		TrackedBuildings.Add(Candidate);
		if (!Candidate->OnDestroyed.IsAlreadyBound(this, &AVillageAnchor::OnTrackedBuildingDestroyed))
		{
			Candidate->OnDestroyed.AddDynamic(this, &AVillageAnchor::OnTrackedBuildingDestroyed);
		}
	}

	RecalculateTargetsFromTrackedBuildings();
	AINPC_VILLAGE_LOG(Log, "[VillageAnchor] RebuildTrackedBuildingsByScan: Anchor=%s Tracked=%d",
		*GetName(),
		TrackedBuildings.Num());
}

void AVillageAnchor::OnTrackedBuildingDestroyed(AActor* DestroyedActor)
{
	NotifyBuildingRemoved(DestroyedActor);
}

void AVillageAnchor::UpdateBuildingDrivenScale(float DeltaSeconds)
{
	if (!bScaleWithTerritoryBuildings)
	{
		return;
	}

	if (bUseEventDrivenBuildingUpdates)
	{
		if (bEnablePeriodicScanFallback)
		{
			TimeSinceLastScan += DeltaSeconds;
			if (TimeSinceLastScan >= BuildingScanInterval)
			{
				TimeSinceLastScan = 0.0f;
				RebuildTrackedBuildingsByScan();
			}
		}
	}
	else
	{
		TimeSinceLastScan += DeltaSeconds;
		if (TimeSinceLastScan >= BuildingScanInterval)
		{
			TimeSinceLastScan = 0.0f;

			float MaxDistance = 0.0f;
			CachedBuildingCount = CountBuildingsInTerritory(MaxDistance);

			const float TargetByCount = BaseClaimRadius + CachedBuildingCount * RadiusPerBuilding;
			const float TargetBySpread = CachedBuildingCount > 0 ? FMath::Max(BaseClaimRadius, MaxDistance + BuildingRadiusPadding) : BaseClaimRadius;
			TargetClaimRadius = FMath::Clamp(FMath::Max(TargetByCount, TargetBySpread), BaseClaimRadius, FMath::Max(BaseClaimRadius, MaxClaimRadius));
		}
	}

	CurrentClaimRadius = FMath::FInterpTo(CurrentClaimRadius, TargetClaimRadius, DeltaSeconds, ScaleInterpSpeed);
	CurrentScaleMultiplier = CurrentClaimRadius / FMath::Max(100.0f, BaseClaimRadius);
	CurrentScaleMultiplier = FMath::Clamp(CurrentScaleMultiplier, 0.1f, MaxScaleMultiplier);

	SetActorScale3D(BaseVisualScale * CurrentScaleMultiplier);
	if (TerritorySphere)
	{
		TerritorySphere->SetSphereRadius(CurrentClaimRadius);
	}

	PushClaimToSubsystem(false);
}

void AVillageAnchor::RecalculateTargetsFromTrackedBuildings()
{
	float MaxDistance = 0.0f;
	const FVector Center = GetActorLocation();
	int32 ValidCount = 0;

	for (int32 Index = TrackedBuildings.Num() - 1; Index >= 0; --Index)
	{
		AActor* Building = TrackedBuildings[Index].Get();
		if (!IsBuildingCandidate(Building) || !IsBuildingInsideCurrentTerritory(Building))
		{
			if (IsValid(Building))
			{
				Building->OnDestroyed.RemoveDynamic(this, &AVillageAnchor::OnTrackedBuildingDestroyed);
			}
			TrackedBuildings.RemoveAtSwap(Index);
			continue;
		}

		const float Distance = FVector::Dist(Center, Building->GetActorLocation());
		MaxDistance = FMath::Max(MaxDistance, Distance);
		++ValidCount;
	}

	CachedBuildingCount = ValidCount;
	const float TargetByCount = BaseClaimRadius + CachedBuildingCount * RadiusPerBuilding;
	const float TargetBySpread = CachedBuildingCount > 0 ? FMath::Max(BaseClaimRadius, MaxDistance + BuildingRadiusPadding) : BaseClaimRadius;
	TargetClaimRadius = FMath::Clamp(FMath::Max(TargetByCount, TargetBySpread), BaseClaimRadius, FMath::Max(BaseClaimRadius, MaxClaimRadius));
}

void AVillageAnchor::PushClaimToSubsystem(bool bForce)
{
	UNPCVillageSubsystem* VillageSubsystem = CachedVillageSubsystem.Get();
	if (!VillageSubsystem)
	{
		if (UWorld* World = GetWorld())
		{
			VillageSubsystem = World->GetSubsystem<UNPCVillageSubsystem>();
			CachedVillageSubsystem = VillageSubsystem;
		}
	}

	if (!VillageSubsystem)
	{
		if (bForce)
		{
			AINPC_VILLAGE_LOG(Warning, "[VillageAnchor] Claim push skipped: NPCVillageSubsystem missing. Anchor=%s", *GetName());
		}
		return;
	}

	if (VillageID.IsNone() || VillageID == FName(TEXT("None")))
	{
		if (bForce)
		{
			AINPC_VILLAGE_LOG(Warning, "[VillageAnchor] Claim push skipped: VillageID is empty. Anchor=%s", *GetName());
		}
		return;
	}

	const float RadiusDelta = FMath::Abs(CurrentClaimRadius - LastClaimedRadius);
	if (!bForce && RadiusDelta < ClaimRadiusUpdateThreshold)
	{
		return;
	}

	VillageSubsystem->ClaimTerritoryFromActor(VillageID, this, CurrentClaimRadius, ClaimPriority);
	LastClaimedRadius = CurrentClaimRadius;
	if (bForce)
	{
		AINPC_VILLAGE_LOG(Log, "[VillageAnchor] Territory claim pushed (force): Anchor=%s Village=%s Radius=%.0f Priority=%d",
			*GetName(),
			*VillageID.ToString(),
			CurrentClaimRadius,
			ClaimPriority);
	}
	else
	{
		AINPC_VILLAGE_LOG_VERBOSE("[VillageAnchor] Territory claim pushed: Anchor=%s Radius=%.0f Delta=%.1f",
			*GetName(),
			CurrentClaimRadius,
			RadiusDelta);
	}
}

int32 AVillageAnchor::CountBuildingsInTerritory(float& OutMaxDistance) const
{
	OutMaxDistance = 0.0f;

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	const FVector Center = GetActorLocation();
	const float RadiusSq = CurrentClaimRadius * CurrentClaimRadius;
	int32 Count = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsBuildingCandidate(Candidate))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Center, Candidate->GetActorLocation());
		if (DistSq > RadiusSq)
		{
			continue;
		}

		Count++;
		OutMaxDistance = FMath::Max(OutMaxDistance, FMath::Sqrt(DistSq));
	}

	return Count;
}

bool AVillageAnchor::IsBuildingCandidate(AActor* Candidate) const
{
	if (!IsValid(Candidate) || Candidate == this)
	{
		return false;
	}

	if (bExcludePawnsFromBuildingScan && Candidate->IsA<APawn>())
	{
		return false;
	}

	if (BuildingClassFilter && !Candidate->IsA(BuildingClassFilter))
	{
		return false;
	}

	if (BuildingTags.Num() == 0)
	{
		return true;
	}

	for (const FName Tag : BuildingTags)
	{
		if (!Tag.IsNone() && Candidate->ActorHasTag(Tag))
		{
			return true;
		}
	}

	return false;
}

bool AVillageAnchor::IsBuildingInsideCurrentTerritory(AActor* Candidate) const
{
	if (!IsValid(Candidate))
	{
		return false;
	}

	const float Radius = FMath::Max(CurrentClaimRadius, BaseClaimRadius);
	const float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
	return DistSq <= Radius * Radius;
}
