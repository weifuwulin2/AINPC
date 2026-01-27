
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Social/FactionTypes.h"
#include "FactionSubsystem.generated.h"

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

	// --- Debug ---
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	void DebugPrintRelations() const;

	/** 
		 * Runtime Matrix: [SourceFaction] -> [TargetFaction] -> Attitude
		 * Example: Matrix["Humans"]["Zombies"] = 0.0f
		 */
	TMap<FName, TMap<FName, float>> RuntimeFactionMatrix;
private:
	

	/** Cached pointers for thresholds */
	TMap<FName, float> HostilityThresholds;
};
