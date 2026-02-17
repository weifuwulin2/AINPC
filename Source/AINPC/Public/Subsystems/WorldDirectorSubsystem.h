#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/WorldDirectorTypes.h"
#include "Events/EventTypes.h"
#include "WorldDirectorSubsystem.generated.h"

class UNarrativeHistorySubsystem;
class UNarrativeSquadSubsystem;
class UFactionSubsystem;
class UPlayerSquadSubsystem;
class UTimeManager;
class ULLMCommunicator;
class ANarrativeCompanion;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDramaticBeat, EDramaticAction, Action, const FString&, PlotOutline);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWorldMajorEventStarted, EWorldMajorEventType, EventType, const FActiveWorldEvent&, EventData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWorldMajorEventEnded, EWorldMajorEventType, EventType, const FActiveWorldEvent&, EventData);

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

	/** World major events DataTable (FWorldMajorEventRow rows) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldDirector|Data")
	UDataTable* MajorEventsTable;

	// ============================
	// State (Read-Only)
	// ============================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WorldDirector|State")
	FTensionSnapshot LatestTension;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WorldDirector|State")
	int32 ActiveDirectorScenes = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WorldDirector|State")
	TArray<FActiveWorldEvent> ActiveMajorEvents;

	// ============================
	// Delegates
	// ============================

	UPROPERTY(BlueprintAssignable, Category = "WorldDirector")
	FOnDramaticBeat OnDramaticBeat;

	UPROPERTY(BlueprintAssignable, Category = "WorldDirector")
	FOnWorldMajorEventStarted OnWorldMajorEventStarted;

	UPROPERTY(BlueprintAssignable, Category = "WorldDirector")
	FOnWorldMajorEventEnded OnWorldMajorEventEnded;

	// ============================
	// Debug / Manual API
	// ============================

	/** Force an immediate evaluation cycle (for testing/console) */
	UFUNCTION(BlueprintCallable, Category = "WorldDirector|Debug")
	void ForceEvaluation();

	/** Force a world prediction LLM call (for testing/console) */
	UFUNCTION(BlueprintCallable, Category = "WorldDirector|Debug")
	void ForcePrediction();

	/** Get all currently active world major events */
	UFUNCTION(BlueprintCallable, Category = "WorldDirector")
	const TArray<FActiveWorldEvent>& GetActiveWorldEvents() const { return ActiveMajorEvents; }

	/** Get a formatted description of active events (for LLM context injection) */
	UFUNCTION(BlueprintCallable, Category = "WorldDirector")
	FString GetActiveEventsDescription() const;

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

	/** Request LLM to predict future world events based on history (called on day change) */
	void RequestWorldPrediction();

	/** Callback: process LLM world prediction response */
	void OnWorldPredictionResponse(bool bSuccess, const FString& Response);

	/** Parse LLM prediction JSON into dynamic timeline events */
	bool ParsePredictionEvents(const FString& JsonString, TArray<FWorldTimelineEvent>& OutEvents) const;

	/** Validate a scene blueprint before assembly (dead NPC check, dedup, faction logic) */
	bool ValidateSceneBlueprint(const FLLMSceneBlueprint& Blueprint, EDramaticAction Action) const;

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

	/** Check MajorEventsTable for events whose trigger time has arrived */
	void CheckMajorEvents();

	/** Tick active major events and remove expired ones */
	void TickActiveEvents();

	/** Map EWorldMajorEventType to a gameplay tag */
	FGameplayTag GetTagForEventType(EWorldMajorEventType EventType) const;

	/** Callback when a director-managed scene ends */
	UFUNCTION()
	void OnDirectorEventRecorded(const FNarrativeEvent& Event);

	// ============================
	// Subsystem Pointers (cached)
	// ============================

	UPROPERTY()
	UNarrativeHistorySubsystem* NarrativeHistory = nullptr;

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

	/** LLM-generated dynamic timeline events (runtime, not from DataTable) */
	TArray<FWorldTimelineEvent> DynamicTimeline;

	/** Whether a world prediction LLM call is currently in flight */
	bool bPredictionInFlight = false;

	/** Day change handler */
	UFUNCTION()
	void OnDayChanged(int32 NewDay);
};


