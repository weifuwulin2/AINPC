#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/VillageScenarioTypes.h"
#include "VillageSpawner.generated.h"

class APawn;
class UDataTable;

/**
 * Data-driven village bootstrap spawner.
 * Spawns residents from DT_VillageScenarios + DT_NPCDefinitions and applies key relationship seeds.
 */
UCLASS(Blueprintable)
class AINPC_API AVillageSpawner : public AActor
{
	GENERATED_BODY()

public:
	AVillageSpawner();

	virtual void BeginPlay() override;

	/** Spawn one scenario row now. */
	UFUNCTION(BlueprintCallable, Category = "Village Spawner")
	bool SpawnScenario(FName InScenarioRowID = NAME_None);

	/** Remove all NPCs spawned by this spawner. */
	UFUNCTION(BlueprintCallable, Category = "Village Spawner")
	void ClearSpawnedNPCs();

	/** Authoring validation for one scenario row. */
	UFUNCTION(BlueprintCallable, Category = "Village Spawner")
	bool ValidateScenarioTemplate(FName InScenarioRowID = NAME_None, bool bLogDetails = true) const;

	/** Print startup diagnostics: profession + goal + utility action state for all spawned NPCs. */
	UFUNCTION(BlueprintCallable, Category = "Village Spawner")
	void RunProfessionSanityCheck();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Data")
	UDataTable* VillageScenarioTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Data")
	UDataTable* NPCDefinitionTable = nullptr;

	/** Scenario row name in VillageScenarioTable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Data")
	FName ScenarioRowID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Runtime")
	bool bSpawnOnBeginPlay = true;

	/** If true, destroys previously spawned residents before spawning again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Runtime")
	bool bDestroyPreviousSpawnedNPCsOnRespawn = false;

	/** If true, scenario VillageID overrides template VillageID. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Runtime")
	bool bForceVillageIDOverride = true;

	/** Apply authored key relationship seeds after spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Runtime")
	bool bApplyScenarioRelationshipSeeds = true;

	/** Rebuild village relationships after all spawns complete. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Runtime")
	bool bRebuildVillageRelationshipsAfterSpawn = true;

	/** Startup diagnostics for profession flow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Diagnostics")
	bool bRunProfessionSanityCheckOnSpawn = true;

	/** Delay to wait for controller possession before diagnostics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Diagnostics", meta = (ClampMin = "0.0"))
	float ProfessionSanityCheckDelay = 1.5f;

	/** 0 means random seed from runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village Spawner|Runtime")
	int32 RandomSeed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village Spawner|Runtime")
	TArray<TObjectPtr<APawn>> SpawnedNPCs;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village Spawner|Runtime")
	TMap<FName, TArray<TObjectPtr<APawn>>> SpawnedSlotMap;

private:
	APawn* SpawnNPCFromTemplate(
		const FName TemplateID,
		const FName InstanceSlotID,
		const FName ScenarioVillageID,
		const FName FixedNameID,
		const FVector& SpawnLocation);

	void RegisterSpawnedNPC(FName InstanceSlotID, APawn* SpawnedPawn);
	void CollectActorsBySlotKey(FName SlotKey, TArray<APawn*>& OutActors) const;
	void ApplyScenarioRelationshipSeeds(const FVillageScenarioRow& ScenarioRow);
	void ApplySeedDirection(APawn* SourcePawn, APawn* TargetPawn, const FVillageScenarioRelationshipSeed& Seed);
	FName BuildInstanceSlotID(FName BaseSlotID, int32 Index, int32 Count) const;
	FVector BuildSpawnLocation(FRandomStream& Stream, float SpawnRadius, float MinSpawnDistance, TArray<FVector>& OccupiedLocations) const;
	bool ResolveScenarioRow(FName InScenarioRowID, const FVillageScenarioRow*& OutRow) const;

	FTimerHandle ProfessionDiagnosticsTimerHandle;
};

