#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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
