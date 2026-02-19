#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NPCVillageSubsystem.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ENPCVillageSocialTier : uint8
{
	Commoner UMETA(DisplayName = "Commoner"),
	Noble    UMETA(DisplayName = "Noble"),
	Leader   UMETA(DisplayName = "Leader")
};

USTRUCT(BlueprintType)
struct AINPC_API FVillageTerritoryClaim
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Territory")
	FName VillageID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Territory")
	FVector Center = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Territory", meta = (ClampMin = "100.0"))
	float Radius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Territory")
	int32 Priority = 0;

	/** Optional actor that owns this claim (e.g. a newly built house). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village Territory")
	TObjectPtr<AActor> SourceActor = nullptr;
};

USTRUCT(BlueprintType)
struct AINPC_API FVillageMemberList
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC Village")
	TArray<TObjectPtr<AActor>> Members;
};

/**
 * Global NPC registry grouped by VillageID.
 * Provides lightweight social bootstrap after NPC identity initialization.
 */
UCLASS()
class AINPC_API UNPCVillageSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Register an NPC into a village group. Missing VillageID behavior is controlled by bUseDefaultVillageIDWhenMissing. */
	UFUNCTION(BlueprintCallable, Category = "NPC Village")
	void RegisterNPC(AActor* NPC, FName VillageID = NAME_None);

	/** Remove an NPC from village registry. */
	UFUNCTION(BlueprintCallable, Category = "NPC Village")
	void UnregisterNPC(AActor* NPC);

	/** Re-run social bootstrap for all members in a village. */
	UFUNCTION(BlueprintCallable, Category = "NPC Village")
	void RebuildVillageRelationships(FName VillageID);

	/** Get all valid members in a village. */
	UFUNCTION(BlueprintCallable, Category = "NPC Village")
	void GetVillageMembers(FName VillageID, TArray<AActor*>& OutMembers) const;

	/** Return current village membership for an NPC. None if not registered. */
	UFUNCTION(BlueprintPure, Category = "NPC Village")
	FName GetVillageIDForNPC(AActor* NPC) const;

	/** Claim territory for a village; useful when a building is completed. */
	UFUNCTION(BlueprintCallable, Category = "NPC Village|Territory")
	void ClaimTerritory(FName VillageID, FVector Center, float Radius, int32 Priority = 0, AActor* SourceActor = nullptr);

	/** Helper: claim territory from an actor's current location. */
	UFUNCTION(BlueprintCallable, Category = "NPC Village|Territory")
	void ClaimTerritoryFromActor(FName VillageID, AActor* SourceActor, float Radius, int32 Priority = 0);

	/** Revoke all territory claims owned by SourceActor. */
	UFUNCTION(BlueprintCallable, Category = "NPC Village|Territory")
	void RevokeTerritoryBySource(AActor* SourceActor);

	/** Resolve village ownership by world position based on active territory claims. */
	UFUNCTION(BlueprintPure, Category = "NPC Village|Territory")
	FName ResolveVillageByLocation(FVector Location) const;

	/** Re-evaluate one NPC village assignment by location (claims). */
	UFUNCTION(BlueprintCallable, Category = "NPC Village|Territory")
	void RefreshNPCVillageByLocation(AActor* NPC);

	/** Re-evaluate all living NPC village assignments by location (claims). */
	UFUNCTION(BlueprintCallable, Category = "NPC Village|Territory")
	void RefreshAllNPCVillageAssignments();

	/** Fallback group when VillageID is unspecified. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Village")
	FName DefaultVillageID = FName(TEXT("Village.Default"));

	/**
	 * If true, missing VillageID will fallback to DefaultVillageID.
	 * If false (default), missing VillageID means "no village social bootstrap".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Village")
	bool bUseDefaultVillageIDWhenMissing = false;

	/** If true, unresolved NPCs can inherit village from dynamic territory claims. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Village|Territory")
	bool bResolveVillageFromTerritoryWhenMissing = true;

	/** If true, territory claim/revoke will refresh NPC village assignments immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Village|Territory")
	bool bAutoRefreshNPCOnTerritoryClaim = true;

	/** If true, relationship bootstrap runs as soon as NPC registers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Village")
	bool bAutoBootstrapRelationshipsOnRegister = true;

	/** Base same-village attitude used when no explicit seed exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Village", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float DefaultVillageAttitude = 60.0f;

	/** Deterministic variation range applied to base attitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Village", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float VillageAttitudeNoiseRange = 8.0f;

	/** Existing profession IDs treated as leadership tier (configured from DT_Professions row names). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Village")
	TArray<FName> LeaderProfessionIDs;

	/** Existing profession IDs treated as noble tier (configured from DT_Professions row names). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Village")
	TArray<FName> NobleProfessionIDs;

private:
	UFUNCTION()
	void OnVillageNPCDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnTerritorySourceDestroyed(AActor* DestroyedActor);

	FName ResolveVillageID(FName InVillageID) const;
	FName ResolveStableID(const AActor* NPC) const;
	ENPCVillageSocialTier ResolveTier(const AActor* NPC) const;
	void CleanupInvalidTerritoryClaims();
	void CleanupInvalidMembers(FName VillageID);
	void BootstrapRelationshipsFor(AActor* NPC, FName VillageID);
	float ComputeInitialAttitude(AActor* Source, AActor* Target, FName VillageID) const;
	FString BuildVillageSummary(const AActor* Source, const AActor* Target, FName VillageID) const;

	UPROPERTY(Transient)
	TMap<FName, FVillageMemberList> VillageMembers;

	UPROPERTY(Transient)
	TMap<AActor*, FName> ActorVillageMap;

	UPROPERTY(Transient)
	TArray<FVillageTerritoryClaim> TerritoryClaims;
};
