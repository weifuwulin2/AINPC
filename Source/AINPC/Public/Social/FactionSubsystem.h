
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Social/FactionTypes.h"
#include "Social/SocialTypes.h"
#include "FactionSubsystem.generated.h"

USTRUCT()
struct AINPC_API FRelationshipSeedRuntime
{
	GENERATED_BODY()

	UPROPERTY()
	float Attitude = 50.0f;

	UPROPERTY()
	ESocialBondType BondType = ESocialBondType::None;

	UPROPERTY()
	FString Summary;

	UPROPERTY()
	int32 Salience = 1;
};

/**
 * Manages Global Faction Relationships (Macro Layer).
 * Handles "War", "Alliance", and base reputation values.
 */
UCLASS()
class AINPC_API UFactionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Initialization ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Load initial relationships from the Faction DataTable */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	void InitializeFactions(UDataTable* FactionTable);

	/** Load fixed NPC-to-NPC relationship seeds from DataTable (FRelationshipSeedRow). */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	void InitializeRelationshipSeeds(UDataTable* SeedTable);

	// --- Runtime API ---

	/** 
	 * Get the global base attitude from SourceFaction towards TargetFaction.
	 * Returns 0-100 (Default 50).
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	float GetBaseAttitude(FName SourceFaction, FName TargetFaction) const;

	/**
	 * Declare a specific relationship (e.g., War/Peace) globally.
	 * NewValue: 0=Hostile, 100=Friendly.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	void SetFactionRelation(FName SourceFaction, FName TargetFaction, float NewValue);

	/**
	 * Helper: Are these two factions hostile globally?
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	bool AreFactionsHostile(FName SourceFaction, FName TargetFaction) const;

	/**
	 * Query fixed initial relationship seed by stable IDs.
	 * Returns true if a seeded relationship exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	bool TryGetSeedRelationship(
		FName SourceNameID,
		FName TargetNameID,
		float& OutAttitude,
		ESocialBondType& OutBondType,
		FString& OutSummary,
		int32& OutSalience) const;

	// --- Debug ---
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	void DebugPrintRelations() const;

	/** Optional Relationship Seed table reference (FRelationshipSeedRow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction System")
	UDataTable* RelationshipSeedTable = nullptr;

	/** Default same-faction attitude when no explicit self-entry exists in table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction System")
	float DefaultIntraFactionAttitude = 60.0f;

	/** 
		 * Runtime Matrix: [SourceFaction] -> [TargetFaction] -> Attitude
		 * Example: Matrix["Humans"]["Zombies"] = 0.0f
		 */
	TMap<FName, TMap<FName, float>> RuntimeFactionMatrix;
private:
	

	/** Cached pointers for thresholds */
	TMap<FName, float> HostilityThresholds;

	/** Runtime fixed relationship matrix keyed by SourceNameID -> TargetNameID. */
	TMap<FName, TMap<FName, FRelationshipSeedRuntime>> RelationshipSeedMatrix;
};
