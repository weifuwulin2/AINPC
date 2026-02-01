#include "Subsystems/EventBusSubsystem.h"
#include "Social/SocialGameplayTags.h"
#include "Utilities/AINPCHelpers.h"
#include "AINPC.h"

DEFINE_LOG_CATEGORY_STATIC(LogEventBus, Log, All);

void UEventBusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogEventBus, Log, TEXT("[EventBus] Initialized - Unified Event Routing System Active"));
}

void UEventBusSubsystem::Deinitialize()
{
	// Clean up all subscriptions
	Subscriptions.Empty();
	EventQueue.Empty();

	UE_LOG(LogEventBus, Log, TEXT("[EventBus] Deinitialized - Total events broadcast: %d"), TotalEventsBroadcast);

	Super::Deinitialize();
}

// ========================================
// Core API: Publishing Events
// ========================================

void UEventBusSubsystem::BroadcastEvent(const FEventMetadata& EventData)
{
	if (bEnableDebugLogging)
	{
		UE_LOG(LogEventBus, Log, TEXT("[EventBus] 📢 Broadcasting Event: %s | Priority: %d | Tags: %s"),
			*EventData.Event.Content,
			(int32)EventData.Priority,
			*EventData.EventTags.ToStringSimple());
	}

	// Track statistics
	EventsThisFrame++;
	TotalEventsBroadcast++;

	// Clean up dead subscriptions periodically
	if (TotalEventsBroadcast % 100 == 0)
	{
		CleanupDeadSubscriptions();
	}

	// Find matching subscribers
	int32 DeliveryCount = 0;

	for (FEventSubscription& Subscription : Subscriptions)
	{
		// Skip if subscriber was destroyed
		if (!Subscription.Subscriber.IsValid())
		{
			continue;
		}

		// Check if event matches subscription filter
		if (!MatchesFilter(EventData, Subscription))
		{
			continue;
		}

		// Check priority threshold
		if (EventData.Priority > Subscription.MinPriority)
		{
			continue; // Event priority too low for this subscriber
		}

		// Execute callback
		if (Subscription.Callback.IsBound())
		{
			Subscription.Callback.Execute(EventData);
			DeliveryCount++;

			if (bEnableDebugLogging)
			{
				UE_LOG(LogEventBus, Verbose, TEXT("[EventBus]   ↳ Delivered to: %s"),
					Subscription.Subscriber.IsValid() ? *Subscription.Subscriber->GetName() : TEXT("Unknown"));
			}
		}

		// Check if event was cancelled
		if (EventData.bIsCancelled && EventData.bCanBeCancelled)
		{
			if (bEnableDebugLogging)
			{
				UE_LOG(LogEventBus, Warning, TEXT("[EventBus] Event cancelled by subscriber"));
			}
			break;
		}
	}

	if (bEnableDebugLogging)
	{
		UE_LOG(LogEventBus, Log, TEXT("[EventBus] Event delivered to %d subscribers"), DeliveryCount);
	}
}

void UEventBusSubsystem::BroadcastSemanticEvent(const FSemanticEvent& Event, EEventPriority Priority)
{
	FEventMetadata Metadata(Event, Priority);

	// Auto-populate EventTags from Event.Verb
	if (Event.Verb.IsValid())
	{
		Metadata.EventTags.AddTag(Event.Verb);
	}

	// Set timestamp
	if (UWorld* World = GetWorld())
	{
		Metadata.Timestamp = World->GetTimeSeconds();
	}

	BroadcastEvent(Metadata);
}

// ========================================
// Core API: Subscribing to Events
// ========================================

FGuid UEventBusSubsystem::Subscribe(
	UObject* Subscriber,
	FGameplayTagContainer FilterTags,
	EEventPriority MinPriority,
	FOnEventReceived Callback)
{
	if (!Subscriber)
	{
		UE_LOG(LogEventBus, Error, TEXT("[EventBus] Subscribe failed: Subscriber is null"));
		return FGuid();
	}

	if (!Callback.IsBound())
	{
		UE_LOG(LogEventBus, Error, TEXT("[EventBus] Subscribe failed: Callback is not bound"));
		return FGuid();
	}

	FEventSubscription NewSubscription;
	NewSubscription.Subscriber = Subscriber;
	NewSubscription.FilterTags = FilterTags;
	NewSubscription.MinPriority = MinPriority;
	NewSubscription.Callback = Callback;

	Subscriptions.Add(NewSubscription);

	if (bEnableDebugLogging)
	{
		UE_LOG(LogEventBus, Log, TEXT("[EventBus] 🎧 Subscribed: %s | Filter: %s | MinPriority: %d"),
			*Subscriber->GetName(),
			FilterTags.IsEmpty() ? TEXT("All") : *FilterTags.ToStringSimple(),
			(int32)MinPriority);
	}

	return NewSubscription.SubscriptionId;
}

void UEventBusSubsystem::Unsubscribe(FGuid SubscriptionId)
{
	int32 RemovedCount = Subscriptions.RemoveAll([SubscriptionId](const FEventSubscription& Sub)
	{
		return Sub.SubscriptionId == SubscriptionId;
	});

	if (bEnableDebugLogging && RemovedCount > 0)
	{
		UE_LOG(LogEventBus, Log, TEXT("[EventBus] Unsubscribed: %s"), *SubscriptionId.ToString());
	}
}

void UEventBusSubsystem::UnsubscribeAll(UObject* Subscriber)
{
	if (!Subscriber)
	{
		return;
	}

	int32 RemovedCount = Subscriptions.RemoveAll([Subscriber](const FEventSubscription& Sub)
	{
		return Sub.Subscriber.Get() == Subscriber;
	});

	if (bEnableDebugLogging && RemovedCount > 0)
	{
		UE_LOG(LogEventBus, Log, TEXT("[EventBus] Unsubscribed all (%d) for: %s"), RemovedCount, *Subscriber->GetName());
	}
}

// ========================================
// Helper API: Quick Event Creation
// ========================================

void UEventBusSubsystem::BroadcastDeathEvent(AActor* DeadActor, AActor* Killer)
{
	if (!DeadActor)
	{
		return;
	}

	FSemanticEvent Event;
	Event.Instigator = Killer ? Killer : DeadActor;
	Event.Target = DeadActor;
	Event.Verb = AINPCTags::Event_Death;
	Event.Magnitude = 1.0f;

	// Generate description
	FString DeadName = AINPCHelpers::GetSmartActorName(DeadActor);
	if (Killer)
	{
		FString KillerName = AINPCHelpers::GetSmartActorName(Killer);
		Event.Content = FString::Printf(TEXT("%s was killed by %s"), *DeadName, *KillerName);
	}
	else
	{
		Event.Content = FString::Printf(TEXT("%s died"), *DeadName);
	}

	FEventMetadata Metadata(Event, EEventPriority::Critical);
	Metadata.EventTags.AddTag(AINPCTags::Event_Death);
	Metadata.EventTags.AddTag(AINPCTags::Event_Danger);

	BroadcastEvent(Metadata);

	UE_LOG(LogEventBus, Warning, TEXT("[EventBus] ⚰️ Death Event: %s"), *Event.Content);
}

void UEventBusSubsystem::BroadcastActionChangeEvent(AActor* Actor, const FString& OldAction, const FString& NewAction, FGameplayTag DirectiveTag)
{
	if (!Actor)
	{
		return;
	}

	FSemanticEvent Event;
	Event.Instigator = Actor;
	Event.Target = nullptr;
	Event.Verb = DirectiveTag; // Use Directive as Verb
	Event.Magnitude = 0.5f;

	FString ActorName = AINPCHelpers::GetSmartActorName(Actor);
	Event.Content = FString::Printf(TEXT("%s switched from %s to %s"), *ActorName, *OldAction, *NewAction);

	FEventMetadata Metadata(Event, EEventPriority::Normal);
	Metadata.EventTags.AddTag(DirectiveTag);

	BroadcastEvent(Metadata);
}

void UEventBusSubsystem::BroadcastDamageEvent(AActor* Attacker, AActor* Victim, float DamageAmount)
{
	if (!Victim)
	{
		return;
	}

	FSemanticEvent Event;
	Event.Instigator = Attacker;
	Event.Target = Victim;
	Event.Verb = AINPCTags::Event_Combat_Damage;
	Event.Magnitude = FMath::Clamp(DamageAmount / 100.0f, 0.0f, 1.0f);

	FString VictimName = AINPCHelpers::GetSmartActorName(Victim);
	if (Attacker)
	{
		FString AttackerName = AINPCHelpers::GetSmartActorName(Attacker);
		Event.Content = FString::Printf(TEXT("%s dealt %.1f damage to %s"), *AttackerName, DamageAmount, *VictimName);
	}
	else
	{
		Event.Content = FString::Printf(TEXT("%s took %.1f damage"), *VictimName, DamageAmount);
	}

	FEventMetadata Metadata(Event, EEventPriority::High);
	Metadata.EventTags.AddTag(AINPCTags::Event_Combat_Damage);
	Metadata.EventTags.AddTag(AINPCTags::Event_Danger_Combat);

	BroadcastEvent(Metadata);
}

// ========================================
// Internal Implementation
// ========================================

bool UEventBusSubsystem::MatchesFilter(const FEventMetadata& Event, const FEventSubscription& Subscription) const
{
	// Empty filter = receive all events
	if (Subscription.FilterTags.IsEmpty())
	{
		return true;
	}

	// Check if any of the event's tags match the subscription filter
	// Uses "OR" logic: event matches if it has ANY of the filter tags
	return Event.EventTags.HasAny(Subscription.FilterTags);
}

void UEventBusSubsystem::CleanupDeadSubscriptions()
{
	int32 OriginalCount = Subscriptions.Num();

	Subscriptions.RemoveAll([](const FEventSubscription& Sub)
	{
		return !Sub.Subscriber.IsValid();
	});

	int32 RemovedCount = OriginalCount - Subscriptions.Num();

	if (RemovedCount > 0 && bEnableDebugLogging)
	{
		UE_LOG(LogEventBus, Log, TEXT("[EventBus] Cleaned up %d dead subscriptions"), RemovedCount);
	}
}

void UEventBusSubsystem::ProcessEventQueue()
{
	// Reserved for future deferred event processing
	// Currently all events are processed immediately
}
