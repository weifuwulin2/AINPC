#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EventTypes.generated.h"

/**
 * ============================================================================
 * Unified Event Type Definitions
 * 统一的事件类型定义
 * ============================================================================
 *
 * This file contains all event structures used across the AINPC system.
 * 本文件包含 AINPC 系统中使用的所有事件结构。
 *
 * Event Types:
 * 1. FSemanticEvent - AI core system (individual NPC perception)
 * 2. FNarrativeEvent - Global world history (long-term storage)
 * 3. FEventMetadata - Event bus routing wrapper (optional)
 *
 * ============================================================================
 */

// ============================================================================
// 1. FSemanticEvent - AI Core System Event
//    Used by: Sensory, Memory, Cognition, Attention Budget
// ============================================================================

/**
 * Semantic Event - The standard data package for AI perception
 * 语义事件 - AI 感知的标准数据包
 *
 * This is the primary event type flowing through the AI system.
 * Decouples "what happened" from "how to react".
 *
 * Flow: Sensory → Memory → Cognition → Action Selection
 */
USTRUCT(BlueprintType)
struct AINPC_API FSemanticEvent
{
	GENERATED_BODY()

	// --- Core Fields (Phase 1) ---

	/** Who performed the action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	TObjectPtr<AActor> Instigator = nullptr;

	/** What was done (Taxonomy Tag, e.g., Social.Conflict.Insult, Interaction.Combat) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FGameplayTag Verb;

	/** Who was the target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	TObjectPtr<AActor> Target = nullptr;

	/** Natural language description (e.g., "Soldier A insulted Recruit B.") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FString Content;

	/** Intensity of the event (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	float Magnitude = 0.0f;

	// --- Extended Fields (Phase 2 - Unified Event Refactor) ---

	/** World time when event occurred */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	float Timestamp = 0.0f;

	/** World location (important for non-actor stimuli like explosions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FVector Location = FVector::ZeroVector;

	/** Contextual tags (e.g., Directive.Combat, Status.Panicked)
	 *  Replaces need for separate DirectiveTag fields */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FGameplayTagContainer ContextTags;

	// --- Helper Functions ---

	/** Extract Directive tag from ContextTags (e.g., Directive.Combat) */
	FGameplayTag GetDirective() const
	{
		// Find first tag starting with "Directive"
		for (const FGameplayTag& Tag : ContextTags)
		{
			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Directive"))))
			{
				return Tag;
			}
		}
		return FGameplayTag();
	}

	/** Check if event involves combat */
	bool IsCombatEvent() const
	{
		return Verb.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Interaction.Combat"))) ||
		       ContextTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Directive.Combat")));
	}

	/** Check if event involves social interaction */
	bool IsSocialEvent() const
	{
		return Verb.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Interaction.Social"))) ||
		       ContextTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Directive.Social")));
	}
};

// ============================================================================
// 2. FNarrativeEvent - Global World History Event
//    Used by: NarrativeDirectorSubsystem
// ============================================================================

/**
 * Narrative Event - Global world history record
 * 叙事事件 - 全局世界历史记录
 *
 * Lightweight structure for long-term storage.
 * Unlike FSemanticEvent, this doesn't store Actor references (which can become invalid).
 *
 * Use Case: World history log, quest tracking, faction events
 */
USTRUCT(BlueprintType)
struct AINPC_API FNarrativeEvent
{
	GENERATED_BODY()

	/** Natural language description of the event */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Event")
	FString Description;

	/** Event classification tags (e.g., Event.Death, Event.Combat, Faction.Orc) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Event")
	FGameplayTagContainer Tags;

	/** World time when event occurred */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Event")
	float Timestamp = 0.0f;

	// --- Constructors ---

	FNarrativeEvent() : Timestamp(0.f) {}

	FNarrativeEvent(FString InDesc, float InTime)
		: Description(InDesc), Timestamp(InTime) {}

	FNarrativeEvent(FString InDesc, FGameplayTagContainer InTags, float InTime)
		: Description(InDesc), Tags(InTags), Timestamp(InTime) {}

	// --- Helper Functions ---

	/** Convert from FSemanticEvent to FNarrativeEvent (for recording) */
	static FNarrativeEvent FromSemanticEvent(const FSemanticEvent& Event)
	{
		FNarrativeEvent NarrativeEvent;
		NarrativeEvent.Description = Event.Content;
		NarrativeEvent.Timestamp = Event.Timestamp;

		// Copy relevant tags
		if (Event.Verb.IsValid())
		{
			NarrativeEvent.Tags.AddTag(Event.Verb);
		}
		NarrativeEvent.Tags.AppendTags(Event.ContextTags);

		return NarrativeEvent;
	}
};

// ============================================================================
// 3. Event Priority Levels (Shared by Multiple Systems)
// ============================================================================

/**
 * Event Priority Level
 * Used by: AttentionBudgetComponent, EventBusSubsystem
 */
UENUM(BlueprintType)
enum class EEventPriority : uint8
{
	Critical  = 0  UMETA(DisplayName = "Critical"),   // Death, self-damage
	High      = 1  UMETA(DisplayName = "High"),       // Combat, important social
	Medium    = 2  UMETA(DisplayName = "Medium"),     // Moderate importance events
	Normal    = 3  UMETA(DisplayName = "Normal"),     // Regular observations
	Low       = 4  UMETA(DisplayName = "Low"),        // Background activities
	Ambient   = 5  UMETA(DisplayName = "Ambient"),    // Environmental noise
	Deferred  = 6  UMETA(DisplayName = "Deferred")    // Lowest priority / accept all
};

// ============================================================================
// 4. FEventMetadata - Event Bus Routing Wrapper (Optional)
//    Used by: EventBusSubsystem
// ============================================================================

/**
 * Event Metadata - Wrapper for event routing
 * 事件元数据 - 用于事件路由的包装器
 *
 * Wraps FSemanticEvent with additional routing/filtering information.
 * Used by EventBusSubsystem for pub/sub routing.
 */
USTRUCT(BlueprintType)
struct AINPC_API FEventMetadata
{
	GENERATED_BODY()

	/** The core semantic event data */
	UPROPERTY(BlueprintReadWrite, Category = "Event")
	FSemanticEvent Event;

	/** Priority level for processing order */
	UPROPERTY(BlueprintReadWrite, Category = "Event")
	EEventPriority Priority = EEventPriority::Normal;

	/** Game time when event was created */
	UPROPERTY(BlueprintReadWrite, Category = "Event")
	float Timestamp = 0.0f;

	/** Can this event be cancelled by listeners? */
	UPROPERTY(BlueprintReadWrite, Category = "Event")
	bool bCanBeCancelled = false;

	/** Was this event cancelled by a listener? */
	UPROPERTY(BlueprintReadWrite, Category = "Event")
	bool bIsCancelled = false;

	/** Optional additional tags for filtering (e.g., Event.Danger.Combat) */
	UPROPERTY(BlueprintReadWrite, Category = "Event")
	FGameplayTagContainer EventTags;

	// --- Constructors ---

	FEventMetadata() {}

	FEventMetadata(const FSemanticEvent& InEvent, EEventPriority InPriority = EEventPriority::Normal)
		: Event(InEvent)
		, Priority(InPriority)
		, Timestamp(InEvent.Timestamp)
	{
		// Auto-populate EventTags from Event.Verb
		if (InEvent.Verb.IsValid())
		{
			EventTags.AddTag(InEvent.Verb);
		}
		EventTags.AppendTags(InEvent.ContextTags);
	}
};

// ============================================================================
// Internal Wrapper Structures
// ============================================================================

/**
 * Pending Event Wrapper (Internal use only)
 * Used by: AttentionBudgetComponent
 *
 * Wraps FSemanticEvent with priority score for queue management.
 */
struct FPendingEvent
{
	FSemanticEvent EventData;
	float PriorityScore = 0.0f;
	EEventPriority PriorityCategory = EEventPriority::Normal;
};
