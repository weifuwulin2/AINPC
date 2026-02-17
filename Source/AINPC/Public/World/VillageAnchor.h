#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VillageAnchor.generated.h"

class USphereComponent;
class UNPCVillageSubsystem;

/**
 * Dynamic village anchor:
 * - Claims village territory in NPCVillageSubsystem
 * - Scales with buildings inside territory
 * - Updates claim radius as visual scale changes
 */
UCLASS(Blueprintable)
class AINPC_API AVillageAnchor : public AActor
{
	GENERATED_BODY()

public:
	AVillageAnchor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Force immediate territory scan + claim push. */
	UFUNCTION(BlueprintCallable, Category = "Village")
	void ForceRefreshTerritory();

	/** Event-driven hook: call when a building is constructed. */
	UFUNCTION(BlueprintCallable, Category = "Village|Buildings")
	void NotifyBuildingConstructed(AActor* BuildingActor);

	/** Event-driven hook: call when a building is removed/demolished. */
	UFUNCTION(BlueprintCallable, Category = "Village|Buildings")
	void NotifyBuildingRemoved(AActor* BuildingActor);

	/** Optional fallback: rebuild tracked buildings by world scan. */
	UFUNCTION(BlueprintCallable, Category = "Village|Buildings")
	void RebuildTrackedBuildingsByScan();

	/** Current runtime claim radius. */
	UFUNCTION(BlueprintPure, Category = "Village")
	float GetCurrentClaimRadius() const { return CurrentClaimRadius; }

	/** Village identity to claim for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village")
	FName VillageID = NAME_None;

	/** Territory claim priority used by NPCVillageSubsystem conflict resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village")
	int32 ClaimPriority = 0;

	/** Whether to auto-claim territory on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village")
	bool bAutoClaimOnBeginPlay = true;

	/** Base territory radius before building growth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village", meta = (ClampMin = "100.0"))
	float BaseClaimRadius = 2000.0f;

	/** Max allowed territory radius after growth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village", meta = (ClampMin = "100.0"))
	float MaxClaimRadius = 8000.0f;

	/** Enable dynamic scale/radius updates from buildings in territory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale")
	bool bScaleWithTerritoryBuildings = true;

	/** If true, buildings are tracked through explicit events instead of constant scan. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale")
	bool bUseEventDrivenBuildingUpdates = true;

	/** Do one initial scan on BeginPlay to seed pre-placed buildings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale")
	bool bInitialScanOnBeginPlay = true;

	/** If true, still periodically rescan as fallback drift-correction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale")
	bool bEnablePeriodicScanFallback = false;

	/** Optional fallback: listen to world actor spawn and auto-detect building candidates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale")
	bool bListenWorldSpawnFallback = false;

	/** How often to rescan buildings when fallback scan is enabled (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale", meta = (ClampMin = "0.1"))
	float BuildingScanInterval = 1.0f;

	/** Radius bonus per building inside territory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale", meta = (ClampMin = "0.0"))
	float RadiusPerBuilding = 120.0f;

	/** Spread padding when computing farthest-building based territory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale", meta = (ClampMin = "0.0"))
	float BuildingRadiusPadding = 250.0f;

	/** Interp speed for radius/scale smoothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale", meta = (ClampMin = "0.1"))
	float ScaleInterpSpeed = 3.0f;

	/** Base visual scale when claim radius equals BaseClaimRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale")
	FVector BaseVisualScale = FVector(1.0f, 1.0f, 1.0f);

	/** Clamp to avoid extreme scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale", meta = (ClampMin = "0.1"))
	float MaxScaleMultiplier = 3.0f;

	/** Push claim update only when radius delta exceeds this threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale", meta = (ClampMin = "1.0"))
	float ClaimRadiusUpdateThreshold = 50.0f;

	/** Optional class filter for building detection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale")
	TSubclassOf<AActor> BuildingClassFilter;

	/** Optional actor tags filter for building detection (any match). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale")
	TArray<FName> BuildingTags;

	/** Exclude pawns from building scan candidates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Scale")
	bool bExcludePawnsFromBuildingScan = true;

	/** Debug sphere showing current claim radius. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village|Debug")
	TObjectPtr<USphereComponent> TerritorySphere = nullptr;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village|Runtime")
	float CurrentClaimRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village|Runtime")
	float TargetClaimRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village|Runtime")
	float CurrentScaleMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village|Runtime")
	int32 CachedBuildingCount = 0;

private:
	void HandleWorldActorSpawned(AActor* SpawnedActor);

	UFUNCTION()
	void OnTrackedBuildingDestroyed(AActor* DestroyedActor);

	void UpdateBuildingDrivenScale(float DeltaSeconds);
	void RecalculateTargetsFromTrackedBuildings();
	void PushClaimToSubsystem(bool bForce);
	int32 CountBuildingsInTerritory(float& OutMaxDistance) const;
	bool IsBuildingCandidate(AActor* Candidate) const;
	bool IsBuildingInsideCurrentTerritory(AActor* Candidate) const;

	float TimeSinceLastScan = 0.0f;
	float LastClaimedRadius = -1.0f;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> TrackedBuildings;

	FDelegateHandle WorldActorSpawnedHandle;
	TWeakObjectPtr<UNPCVillageSubsystem> CachedVillageSubsystem;
};
