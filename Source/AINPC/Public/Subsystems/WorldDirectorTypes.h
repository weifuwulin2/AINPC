#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "WorldDirectorTypes.generated.h"

/**
 * High-level dramatic action the WorldDirector can take.
 * Drives LLM prompt construction and scene tone.
 */
UENUM(BlueprintType)
enum class EDramaticAction : uint8
{
	ESCALATE	UMETA(DisplayName = "Escalate",  ToolTip = "Raise stakes: combat, betrayal, invasion"),
	DISRUPT		UMETA(DisplayName = "Disrupt",   ToolTip = "Break monotony: unexpected visitor, accident, discovery"),
	CONVERGE	UMETA(DisplayName = "Converge",  ToolTip = "Bring factions together: negotiation, trade, alliance"),
	REVEAL		UMETA(DisplayName = "Reveal",    ToolTip = "Expose hidden truth: secret, identity, conspiracy"),
	RELIEVE		UMETA(DisplayName = "Relieve",   ToolTip = "Reduce tension: celebration, gift, peace")
};

/**
 * Phase of a companion narrative hint relative to the dramatic beat.
 */
UENUM(BlueprintType)
enum class ENarrativeHintPhase : uint8
{
	Before	UMETA(DisplayName = "Before"),
	During	UMETA(DisplayName = "During"),
	After	UMETA(DisplayName = "After")
};

/**
 * Snapshot of dramatic tension dimensions.
 * Computed by WorldDirectorSubsystem each evaluation cycle.
 */
USTRUCT(BlueprintType)
struct AINPC_API FTensionSnapshot
{
	GENERATED_BODY()

	/** Recent VIP deaths (0-1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tension")
	float DeathPressure = 0.0f;

	/** Cross-faction hostility ratio (0-1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tension")
	float FactionVolatility = 0.0f;

	/** Recent player action density (0-1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tension")
	float PlayerActivity = 0.0f;

	/** Faction population deltas (0-1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tension")
	float PopulationStress = 0.0f;

	/** Proximity to next anchor event (0-1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tension")
	float TimelinePressure = 0.0f;

	/** Time since last dramatic beat (0-1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tension")
	float Monotony = 0.0f;

	/** Weighted composite of all dimensions (0-1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tension")
	float OverallTension = 0.0f;
};

/**
 * Hint delivered to the Companion (Paimon) for player-facing narration.
 */
USTRUCT(BlueprintType)
struct AINPC_API FNarrativeHint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
	ENarrativeHintPhase Phase = ENarrativeHintPhase::Before;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
	FString HintText;

	/** Priority for ProcessStimulus (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
	float Priority = 0.5f;
};

/**
 * Authored world timeline event (DataTable row).
 * The WorldDirector checks these each cycle and fires them at the designated time.
 */
USTRUCT(BlueprintType)
struct AINPC_API FWorldTimelineEvent : public FTableRowBase
{
	GENERATED_BODY()

	/** Game day on which this event triggers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline")
	int32 TriggerDay = 0;

	/** Game hour at which this event triggers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline")
	float TriggerHour = 12.0f;

	/** Human-readable event name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline")
	FString EventName;

	/** Description injected into LLM context */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline", meta = (MultiLine = true))
	FString EventDescription;

	/** Forced dramatic action (overrides tension-based selection) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline")
	EDramaticAction ForcedAction = EDramaticAction::DISRUPT;

	/** Optional: pre-authored scene template ID to use instead of LLM generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline")
	FName SceneTemplateID;

	/** Runtime flag: has this event already fired? */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Timeline")
	bool bHasFired = false;
};

// ============================================================================
// World Major Events (system-level events: beast tides, invasions, etc.)
// ============================================================================

UENUM(BlueprintType)
enum class EWorldMajorEventType : uint8
{
	BeastTide		UMETA(DisplayName = "Beast Tide"),
	DemonInvasion	UMETA(DisplayName = "Demon Invasion"),
	FactionWar		UMETA(DisplayName = "Faction War"),
	NaturalDisaster	UMETA(DisplayName = "Natural Disaster"),
	Festival		UMETA(DisplayName = "Festival"),
	Plague			UMETA(DisplayName = "Plague"),
	Custom			UMETA(DisplayName = "Custom")
};

/**
 * DataTable row defining a world major event.
 * These are large-scale events that affect the entire world (beast tides, invasions, etc.).
 * Separate from FWorldTimelineEvent which drives narrative scene generation.
 */
USTRUCT(BlueprintType)
struct AINPC_API FWorldMajorEventRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Game day on which this event triggers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MajorEvent")
	int32 TriggerDay = 0;

	/** Game hour at which this event triggers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MajorEvent")
	float TriggerHour = 8.0f;

	/** Type of world event */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MajorEvent")
	EWorldMajorEventType EventType = EWorldMajorEventType::Custom;

	/** Human-readable event name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MajorEvent")
	FString EventName;

	/** Description injected into LLM context */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MajorEvent", meta = (MultiLine = true))
	FString EventDescription;

	/** Duration in days (supports fractional; 0 = instantaneous) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MajorEvent", meta = (ClampMin = "0.0"))
	float DurationDays = 1.0f;

	/** Additive boost to OverallTension while this event is active (0-0.5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MajorEvent", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float TensionBoost = 0.1f;

	/** Forced dramatic action when event starts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MajorEvent")
	EDramaticAction ForcedStartAction = EDramaticAction::ESCALATE;

	/** Runtime flag: has this event already started? */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	bool bHasStarted = false;
};

/**
 * Runtime tracking for an active world major event.
 * Created when the event triggers, removed when it expires.
 */
USTRUCT(BlueprintType)
struct AINPC_API FActiveWorldEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	FName RowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	EWorldMajorEventType EventType = EWorldMajorEventType::Custom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	FString EventName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	FString EventDescription;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	int32 StartDay = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	float StartHour = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	int32 EndDay = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	float EndHour = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	float TensionBoost = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MajorEvent")
	FGameplayTag EventTag;
};

/**
 * Scene blueprint parsed from LLM JSON response.
 * Intermediate representation before assembling a NarrativeSceneSquad.
 */
USTRUCT(BlueprintType)
struct AINPC_API FLLMSceneBlueprint
{
	GENERATED_BODY()

	/** One-paragraph plot outline for the scene */
	UPROPERTY(BlueprintReadOnly, Category = "Scene Blueprint")
	FString PlotOutline;

	/** Natural-language NPC descriptions (e.g. "an orc merchant", "a human guard") */
	UPROPERTY(BlueprintReadOnly, Category = "Scene Blueprint")
	TArray<FString> CastDescriptions;

	/** Desired location type (e.g. "tavern", "town_square") */
	UPROPERTY(BlueprintReadOnly, Category = "Scene Blueprint")
	FString LocationType;

	/** Event matchers that signal scene completion */
	UPROPERTY(BlueprintReadOnly, Category = "Scene Blueprint")
	TArray<FString> CompletionHints;

	/** Companion hint text for each phase */
	UPROPERTY(BlueprintReadOnly, Category = "Scene Blueprint")
	FString CompanionHintBefore;

	UPROPERTY(BlueprintReadOnly, Category = "Scene Blueprint")
	FString CompanionHintDuring;

	UPROPERTY(BlueprintReadOnly, Category = "Scene Blueprint")
	FString CompanionHintAfter;
};
