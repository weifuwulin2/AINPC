#include "World/BuildingBase.h"

#include "AINPC.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "World/VillageAnchor.h"

ABuildingBase::ABuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.AddUnique(FName(TEXT("Building")));
}

void ABuildingBase::BeginPlay()
{
	Super::BeginPlay();

	const bool bContributesToTerritory = ShouldContributeToVillageTerritory();
	if (!bAutoNotifyVillageAnchor || !bContributesToTerritory)
	{
		AINPC_BUILDING_LOG_VERBOSE("[BuildingBase] Auto notify skipped: Building=%s AutoNotify=%s Contributes=%s",
			*GetName(),
			bAutoNotifyVillageAnchor ? TEXT("true") : TEXT("false"),
			bContributesToTerritory ? TEXT("true") : TEXT("false"));
		return;
	}

	if (VillageAnchorOverride)
	{
		AINPC_BUILDING_LOG(Log, "[BuildingBase] Using explicit VillageAnchorOverride: Building=%s Anchor=%s",
			*GetName(),
			*VillageAnchorOverride->GetName());
		BindToVillageAnchor(VillageAnchorOverride, true);
		return;
	}

	if (bAutoResolveVillageAnchorOnBeginPlay)
	{
		ResolveAndBindVillageAnchor(true);
	}
	else
	{
		AINPC_BUILDING_LOG_VERBOSE("[BuildingBase] Auto resolve disabled and no override: Building=%s", *GetName());
	}
}

void ABuildingBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bAutoNotifyVillageAnchor && bHasNotifiedConstructed)
	{
		NotifyRemovedToAnchor();
	}

	Super::EndPlay(EndPlayReason);
}

void ABuildingBase::BindToVillageAnchor(AVillageAnchor* InVillageAnchor, bool bNotifyConstruction)
{
	if (BoundVillageAnchor.Get() == InVillageAnchor)
	{
		if (bNotifyConstruction && !bHasNotifiedConstructed && bAutoNotifyVillageAnchor && ShouldContributeToVillageTerritory())
		{
			NotifyConstructedToAnchor();
		}
		return;
	}

	AVillageAnchor* PreviousAnchor = BoundVillageAnchor.Get();
	if (bHasNotifiedConstructed)
	{
		NotifyRemovedToAnchor();
	}

	BoundVillageAnchor = InVillageAnchor;
	if (IsValid(InVillageAnchor))
	{
		AINPC_BUILDING_LOG(Log, "[BuildingBase] Bound village anchor changed: Building=%s Old=%s New=%s",
			*GetName(),
			PreviousAnchor ? *PreviousAnchor->GetName() : TEXT("None"),
			*InVillageAnchor->GetName());
	}
	else
	{
		AINPC_BUILDING_LOG(Warning, "[BuildingBase] Bound village anchor cleared: Building=%s Old=%s",
			*GetName(),
			PreviousAnchor ? *PreviousAnchor->GetName() : TEXT("None"));
	}

	if (bNotifyConstruction && bAutoNotifyVillageAnchor && ShouldContributeToVillageTerritory())
	{
		NotifyConstructedToAnchor();
	}
}

void ABuildingBase::ResolveAndBindVillageAnchor(bool bNotifyConstruction)
{
	AVillageAnchor* ResolvedAnchor = FindBestVillageAnchor();
	if (!IsValid(ResolvedAnchor))
	{
		AINPC_BUILDING_LOG(Warning, "[BuildingBase] ResolveAndBindVillageAnchor failed: Building=%s SearchRadius=%.0f",
			*GetName(),
			MaxVillageAnchorSearchRadius);
	}

	BindToVillageAnchor(ResolvedAnchor, bNotifyConstruction);
}

bool ABuildingBase::ShouldContributeToVillageTerritory_Implementation() const
{
	return true;
}

AVillageAnchor* ABuildingBase::FindBestVillageAnchor() const
{
	if (VillageAnchorOverride)
	{
		return VillageAnchorOverride;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		AINPC_BUILDING_LOG_WARNING("[BuildingBase] FindBestVillageAnchor failed: World is null for %s", *GetName());
		return nullptr;
	}

	const FVector BuildingLocation = GetActorLocation();
	const float MaxSearchRadius = FMath::Max(100.0f, MaxVillageAnchorSearchRadius);
	const float MaxSearchDistSq = FMath::Square(MaxSearchRadius);
	auto SelectBetterAnchor = [this, BuildingLocation](AVillageAnchor* CurrentBest, AVillageAnchor* Candidate) -> AVillageAnchor*
	{
		if (!IsValid(Candidate))
		{
			return CurrentBest;
		}

		const float CandidateDistSq = FVector::DistSquared(BuildingLocation, Candidate->GetActorLocation());
		const float CandidateRadius = FMath::Max(Candidate->GetCurrentClaimRadius(), Candidate->BaseClaimRadius);
		const bool bInsideCandidate = CandidateDistSq <= FMath::Square(CandidateRadius);
		if (bRequireInsideVillageTerritory && !bInsideCandidate)
		{
			return CurrentBest;
		}

		if (!CurrentBest)
		{
			return Candidate;
		}

		const float BestDistSq = FVector::DistSquared(BuildingLocation, CurrentBest->GetActorLocation());
		const float BestRadius = FMath::Max(CurrentBest->GetCurrentClaimRadius(), CurrentBest->BaseClaimRadius);
		const bool bInsideBest = BestDistSq <= FMath::Square(BestRadius);
		if (bInsideCandidate != bInsideBest)
		{
			return bInsideCandidate ? Candidate : CurrentBest;
		}

		const float CandidateScore = static_cast<float>(Candidate->ClaimPriority) * 100000000.0f - CandidateDistSq;
		const float BestScore = static_cast<float>(CurrentBest->ClaimPriority) * 100000000.0f - BestDistSq;
		return CandidateScore > BestScore ? Candidate : CurrentBest;
	};

	AVillageAnchor* BestAnchor = nullptr;
	{
		FVector BoundsOrigin = BuildingLocation;
		FVector BoundsExtent = FVector::ZeroVector;
		GetActorBounds(true, BoundsOrigin, BoundsExtent);
		const float BoundsRadius = FMath::Max3(BoundsExtent.X, BoundsExtent.Y, BoundsExtent.Z);
		const float QueryRadius = FMath::Min(MaxSearchRadius, FMath::Max(30.0f, BoundsRadius + 50.0f));

		TArray<FOverlapResult> OverlapResults;
		FCollisionObjectQueryParams ObjectQuery;
		ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BuildingResolveVillageAnchor), false, this);
		if (World->OverlapMultiByObjectType(
			OverlapResults,
			BuildingLocation,
			FQuat::Identity,
			ObjectQuery,
			FCollisionShape::MakeSphere(QueryRadius),
			QueryParams))
		{
			for (const FOverlapResult& Overlap : OverlapResults)
			{
				AVillageAnchor* Candidate = Cast<AVillageAnchor>(Overlap.GetActor());
				if (!Candidate && Overlap.Component.IsValid())
				{
					Candidate = Cast<AVillageAnchor>(Overlap.Component->GetOwner());
				}

				if (!Candidate)
				{
					continue;
				}

				const float DistSq = FVector::DistSquared(BuildingLocation, Candidate->GetActorLocation());
				if (DistSq > MaxSearchDistSq)
				{
					continue;
				}

				BestAnchor = SelectBetterAnchor(BestAnchor, Candidate);
			}
		}
	}

	// Fallback for anchors that do not expose collision query volumes.
	if (BestAnchor)
	{
		AINPC_BUILDING_LOG_VERBOSE("[BuildingBase] Anchor resolved by overlap: Building=%s Anchor=%s",
			*GetName(),
			*BestAnchor->GetName());
		return BestAnchor;
	}

	for (TActorIterator<AVillageAnchor> It(World); It; ++It)
	{
		AVillageAnchor* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(BuildingLocation, Candidate->GetActorLocation());
		if (DistSq > MaxSearchDistSq)
		{
			continue;
		}

		BestAnchor = SelectBetterAnchor(BestAnchor, Candidate);
	}

	if (BestAnchor)
	{
		AINPC_BUILDING_LOG_VERBOSE("[BuildingBase] Anchor resolved by fallback scan: Building=%s Anchor=%s",
			*GetName(),
			*BestAnchor->GetName());
	}

	return BestAnchor;
}

void ABuildingBase::NotifyConstructedToAnchor()
{
	AVillageAnchor* Anchor = BoundVillageAnchor.Get();
	if (!IsValid(Anchor))
	{
		bHasNotifiedConstructed = false;
		AINPC_BUILDING_LOG(Warning, "[BuildingBase] Constructed notify skipped: no valid bound anchor for %s", *GetName());
		return;
	}

	Anchor->NotifyBuildingConstructed(this);
	bHasNotifiedConstructed = true;
	AINPC_BUILDING_LOG(Log, "[BuildingBase] Constructed notify: %s -> %s", *GetName(), *Anchor->GetName());
}

void ABuildingBase::NotifyRemovedToAnchor()
{
	AVillageAnchor* Anchor = BoundVillageAnchor.Get();
	if (IsValid(Anchor))
	{
		Anchor->NotifyBuildingRemoved(this);
		AINPC_BUILDING_LOG(Log, "[BuildingBase] Removed notify: %s -> %s", *GetName(), *Anchor->GetName());
	}
	else
	{
		AINPC_BUILDING_LOG_VERBOSE("[BuildingBase] Removed notify skipped: no valid bound anchor for %s", *GetName());
	}

	bHasNotifiedConstructed = false;
}
