#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Social/SocialTypes.h"
#include "VillageScenarioTypes.generated.h"

/**
 * Required role entry: guaranteed to spawn at least Count NPCs from one template.
 */
USTRUCT(BlueprintType)
struct AINPC_API FVillageRequiredRoleEntry
{
	GENERATED_BODY()

	/** Role slot key used by relationship seeds (e.g. Lord, Guard, Commoner). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	FName SlotID = NAME_None;

	/** Template row name in DT_NPCDefinitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	FName DefinitionTemplateID = NAME_None;

	/** Spawn count for this role. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario", meta = (ClampMin = "1"))
	int32 Count = 1;

	/**
	 * Optional fixed name IDs for deterministic key residents.
	 * Index 0 -> first spawned resident in this slot, etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	TArray<FName> FixedNameIDs;
};

/**
 * Optional role entry: spawns by chance and random count from candidate templates.
 */
USTRUCT(BlueprintType)
struct AINPC_API FVillageOptionalRoleEntry
{
	GENERATED_BODY()

	/** Role slot key used by relationship seeds (e.g. Merchant, Visitor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	FName SlotID = NAME_None;

	/** Candidate template row names in DT_NPCDefinitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	TArray<FName> CandidateTemplateIDs;

	/** Spawn count range when this optional role is selected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario", meta = (ClampMin = "0"))
	int32 MinCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario", meta = (ClampMin = "0"))
	int32 MaxCount = 0;

	/** Chance to spawn this optional role group. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnChance = 1.0f;

	/** Optional fixed name IDs for deterministic optional residents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	TArray<FName> FixedNameIDs;
};

/**
 * Authorable key relationship seeds by role slot IDs.
 */
USTRUCT(BlueprintType)
struct AINPC_API FVillageScenarioRelationshipSeed
{
	GENERATED_BODY()

	/** Role slot key (or slot prefix) on source side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	FName SourceSlotID = NAME_None;

	/** Role slot key (or slot prefix) on target side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	FName TargetSlotID = NAME_None;

	/** Initial numeric attitude (0-100). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float InitialAttitude = 60.0f;

	/** Initial semantic bond type for this key relationship. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	ESocialBondType InitialBondType = ESocialBondType::None;

	/** Relationship summary injected into social bond context. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	FString InitialSummary;

	/** Initial salience (0-10). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario", meta = (ClampMin = "0", ClampMax = "10"))
	int32 InitialSalience = 3;

	/** If true, also applies reverse direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	bool bBidirectional = true;
};

/**
 * One row in DT_VillageScenarios.
 */
USTRUCT(BlueprintType)
struct AINPC_API FVillageScenarioRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Village ID assigned to spawned residents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	FName VillageID = NAME_None;

	/** Guaranteed residents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	TArray<FVillageRequiredRoleEntry> RequiredRoles;

	/** Optional residents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	TArray<FVillageOptionalRoleEntry> OptionalRoles;

	/** Key relationship seeds authored by role slot IDs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario")
	TArray<FVillageScenarioRelationshipSeed> RelationshipSeeds;

	/** Spawn radius around VillageSpawner actor location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario", meta = (ClampMin = "100.0"))
	float SpawnRadius = 1800.0f;

	/** Minimum distance between generated spawn points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Scenario", meta = (ClampMin = "0.0"))
	float MinSpawnDistance = 150.0f;
};

