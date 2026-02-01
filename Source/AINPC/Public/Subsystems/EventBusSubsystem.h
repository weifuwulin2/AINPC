#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "Events/EventTypes.h"
#include "EventBusSubsystem.generated.h"

// EEventPriority and FEventMetadata moved to Events/EventTypes.h
// EEventPriority 和 FEventMetadata 已移至 Events/EventTypes.h

/**
 * Event Listener Delegate
 * Called when a subscribed event is broadcast
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnEventReceived, const FEventMetadata&, EventData);

/**
 * Event Subscription Handle
 * Identifies a specific subscription for filtering and unsubscription
 */
USTRUCT()
struct FEventSubscription
{
	GENERATED_BODY()

	/** Unique ID for this subscription */
	UPROPERTY()
	FGuid SubscriptionId;

	/** Filter tags - only events matching these tags will be delivered */
	UPROPERTY()
	FGameplayTagContainer FilterTags;

	/** Callback delegate */
	FOnEventReceived Callback;

	/** Subscriber object (weak reference for auto-cleanup) */
	UPROPERTY()
	TWeakObjectPtr<UObject> Subscriber;

	/** Minimum priority level to receive (filters out low-priority events) */
	EEventPriority MinPriority = EEventPriority::Deferred;

	FEventSubscription()
		: SubscriptionId(FGuid::NewGuid())
		, MinPriority(EEventPriority::Deferred)
	{
	}
};

/**
 * Unified Event Bus Subsystem
 *
 * Central event routing hub for all AI systems.
 * Replaces fragmented delegate chains with unified pub/sub architecture.
 *
 * Design Philosophy:
 * - Single Source of Truth: All events flow through this subsystem
 * - Tag-Based Filtering: Subscribers only receive events matching their filter
 * - Priority Queue: Critical events processed before low-priority ones
 * - Auto-Cleanup: Weak references prevent memory leaks
 * - Backward Compatible: Uses existing FSemanticEvent structure
 *
 * Usage Example:
 * ```cpp
 * // Publishing an event
 * FSemanticEvent Event;
 * Event.Instigator = Attacker;
 * Event.Target = Victim;
 * Event.Verb = AINPCTags::Event_Death;
 * Event.Content = "NPC died";
 *
 * FEventMetadata Metadata(Event, EEventPriority::Critical);
 * Metadata.EventTags.AddTag(AINPCTags::Event_Death);
 *
 * EventBus->BroadcastEvent(Metadata);
 *
 * // Subscribing to events
 * FGameplayTagContainer Filter;
 * Filter.AddTag(AINPCTags::Event_Death);
 *
 * EventBus->Subscribe(this, Filter, EEventPriority::Normal,
 *     [this](const FEventMetadata& Event) {
 *         HandleDeathEvent(Event);
 *     }
 * );
 * ```
 */
UCLASS()
class AINPC_API UEventBusSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========================================
	// Core API: Publishing Events
	// ========================================

	/**
	 * Broadcast an event to all matching subscribers.
	 * Events are processed based on priority and tag filters.
	 *
	 * @param EventData - Event metadata containing the semantic event and routing info
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|EventBus")
	void BroadcastEvent(const FEventMetadata& EventData);

	/**
	 * Simplified broadcast using just FSemanticEvent (assumes Normal priority).
	 * Automatically creates metadata wrapper.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|EventBus")
	void BroadcastSemanticEvent(const FSemanticEvent& Event, EEventPriority Priority = EEventPriority::Normal);

	// ========================================
	// Core API: Subscribing to Events
	// ========================================

	/**
	 * Subscribe to events matching specific tags.
	 *
	 * @param Subscriber - Object subscribing (for auto-cleanup)
	 * @param FilterTags - Only receive events with these tags (empty = receive all)
	 * @param MinPriority - Minimum priority level to receive
	 * @param Callback - Delegate to call when matching event occurs
	 * @return Subscription ID for later unsubscription
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|EventBus")
	FGuid Subscribe(
		UObject* Subscriber,
		FGameplayTagContainer FilterTags,
		EEventPriority MinPriority,
		FOnEventReceived Callback
	);

	/**
	 * Unsubscribe from events.
	 * @param SubscriptionId - ID returned from Subscribe()
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|EventBus")
	void Unsubscribe(FGuid SubscriptionId);

	/**
	 * Unsubscribe all subscriptions from a specific subscriber.
	 * Automatically called when subscriber is destroyed.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|EventBus")
	void UnsubscribeAll(UObject* Subscriber);

	// ========================================
	// Helper API: Quick Event Creation
	// ========================================

	/**
	 * Helper: Broadcast a death event with proper tags and priority.
	 * Replaces manual event construction in SensoryComponent, NarrativeDirector, etc.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|EventBus|Helpers")
	void BroadcastDeathEvent(AActor* DeadActor, AActor* Killer);

	/**
	 * Helper: Broadcast an action change event (for observation system).
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|EventBus|Helpers")
	void BroadcastActionChangeEvent(AActor* Actor, const FString& OldAction, const FString& NewAction, FGameplayTag DirectiveTag);

	/**
	 * Helper: Broadcast a damage event.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|EventBus|Helpers")
	void BroadcastDamageEvent(AActor* Attacker, AActor* Victim, float DamageAmount);

	// ========================================
	// Debug & Statistics
	// ========================================

	/** Get total number of active subscriptions */
	UFUNCTION(BlueprintPure, Category = "AI|EventBus|Debug")
	int32 GetSubscriptionCount() const { return Subscriptions.Num(); }

	/** Get total number of events broadcast this frame */
	UFUNCTION(BlueprintPure, Category = "AI|EventBus|Debug")
	int32 GetEventCountThisFrame() const { return EventsThisFrame; }

	/** Enable verbose logging for debugging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|EventBus|Debug")
	bool bEnableDebugLogging = false;

protected:
	// ========================================
	// Internal Implementation
	// ========================================

	/** All active subscriptions */
	UPROPERTY()
	TArray<FEventSubscription> Subscriptions;

	/** Events queued for processing (priority queue) */
	TArray<FEventMetadata> EventQueue;

	/** Performance tracking */
	int32 EventsThisFrame = 0;
	int32 TotalEventsBroadcast = 0;

	/**
	 * Process event queue (called during Tick or immediately for Critical events).
	 */
	void ProcessEventQueue();

	/**
	 * Check if event matches subscription filter.
	 */
	bool MatchesFilter(const FEventMetadata& Event, const FEventSubscription& Subscription) const;

	/**
	 * Clean up dead subscriptions (subscribers that were destroyed).
	 */
	void CleanupDeadSubscriptions();
};
