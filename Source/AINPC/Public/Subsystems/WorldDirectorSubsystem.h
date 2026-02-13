#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/WorldDirectorTypes.h"
#include "Events/EventTypes.h"
#include "WorldDirectorSubsystem.generated.h"

class UNarrativeDirectorSubsystem;
class UNarrativeSquadSubsystem;
class UFactionSubsystem;
class UPlayerSquadSubsystem;
class UTimeManager;
class ULLMCommunicator;
class ANarrativeCompanion;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDramaticBeat, EDramaticAction, Action, const FString&, PlotOutline);

/**
 * The "Invisible Hand" of the world.
 * Periodically evaluates dramatic tension, uses LLM to generate narrative events,
 * assembles NarrativeSceneSquads, and feeds hints to the Companion.
 */
UCLASS()
class AINPC_API UWorldDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// ============================
	// Configuration
	// ============================

	/** Seconds between periodic evaluations (game time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldDirector|Config")
	float EvaluationIntervalSeconds = 120.0f;

	/** Minimum cooldown between dramatic beats (game time seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldDirector|Config")
	float MinBeatCooldownSeconds = 300.0f;

	/** Tension below this triggers RELIEVE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldDirector|Config")
	float RelieveThreshold = 0.2f;

	/** Tension above this triggers ESCALATE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldDirector|Config")
	float EscalateThreshold = 0.7f;

	/** Maximum concurrent director-spawned scenes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldDirector|Config")
	int32 MaxConcurrentScenes = 2;

	// ============================
	// DataTable References
	// ============================

	/** World timeline events DataTable (FWorldTimelineEvent rows) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldDirector|Data")
	UDataTable* WorldTimelineTable;

	/** NPC definition DataTable for scene spawning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldDirector|Data")
	UDataTable* NPCDefinitionTable;

	/** Scene template DataTable for pre-authored fallback scenes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldDirector|Data")
	UDataTable* SceneTemplateTable;

	// ============================
	// State (Read-Only)
	// ============================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WorldDirector|State")
	FTensionSnapshot LatestTension;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WorldDirector|State")
	int32 ActiveDirectorScenes = 0;

	// ============================
	// Delegates
	// ============================

	UPROPERTY(BlueprintAssignable, Category = "WorldDirector")
	FOnDramaticBeat OnDramaticBeat;

	// ============================
	// Debug / Manual API
	// ============================

	/** Force an immediate evaluation cycle (for testing/console) */
	UFUNCTION(BlueprintCallable, Category = "WorldDirector|Debug")
	void ForceEvaluation();

protected:

	// ============================
	// Core Pipeline
	// ============================

	/** Called on timer: the main evaluation loop */
	void PeriodicEvaluation();

	/** Check and fire any world timeline events whose time has come */
	bool CheckWorldTimeline(EDramaticAction& OutAction, FString& OutContext);

	/** Compute tension snapshot from world state */
	FTensionSnapshot EvaluateTension() const;

	/** Deterministic action selection from tension values */
	EDramaticAction SelectDramaticAction(const FTensionSnapshot& Tension) const;

	/** Build prompts and send LLM request for scene generation */
	void RequestLLMSceneGeneration(EDramaticAction Action, const FString& TimelineContext);

	/** Callback: process LLM response */
	void OnLLMSceneResponse(bool bSuccess, const FString& Response, EDramaticAction Action);

	/** Parse JSON response into scene blueprint */
	bool ParseLLMSceneBlueprint(const FString& JsonString, FLLMSceneBlueprint& OutBlueprint) const;

	/** Assemble a NarrativeSceneSquad from the blueprint */
	void AssembleAndSubmitScene(const FLLMSceneBlueprint& Blueprint, EDramaticAction Action);

	/** Deliver companion hints for the current beat */
	void DeliverCompanionHints(const FLLMSceneBlueprint& Blueprint);

	// ============================
	// Helpers
	// ============================

	/** Find the companion actor from PlayerSquadSubsystem */
	ANarrativeCompanion* FindCompanion() const;

	/** Get gameplay tag for a dramatic action */
	FGameplayTag GetTagForAction(EDramaticAction Action) const;

	/** Callback when a director-managed scene ends */
	UFUNCTION()
	void OnDirectorEventRecorded(const FNarrativeEvent& Event);

	// ============================
	// Subsystem Pointers (cached)
	// ============================

	UPROPERTY()
	UNarrativeDirectorSubsystem* NarrativeDirector = nullptr;

	UPROPERTY()
	UNarrativeSquadSubsystem* SquadSubsystem = nullptr;

	UPROPERTY()
	UFactionSubsystem* FactionSub = nullptr;

	UPROPERTY()
	UPlayerSquadSubsystem* PlayerSquad = nullptr;

	UTimeManager* TimeMgr = nullptr; // GameInstanceSubsystem, not UPROPERTY

	UPROPERTY()
	ULLMCommunicator* LLMService = nullptr;

	// ============================
	// Internal State
	// ============================

	float LastBeatGameTime = -999.0f;

	FTimerHandle EvaluationTimerHandle;
	FTimerHandle DuringHintTimerHandle;

	/** Track active director scene squad IDs for end-detection */
	TSet<int32> ActiveDirectorSquadIDs;

	/** Pending "After" hint text for delivery when scene ends */
	FString PendingAfterHint;
};
