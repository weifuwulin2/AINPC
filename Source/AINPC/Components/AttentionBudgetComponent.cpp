// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AttentionBudgetComponent.h"
#include "Components/SensoryComponent.h"
#include "Components/FactionReputationComponent.h"
#include "Components/GoalComponent.h"
#include "Subsystems/TargetSelectionSubsystem.h"
#include "Social/SocialGameplayTags.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Utilities/FactionHelpers.h" // ✅ Faction utilities

DEFINE_LOG_CATEGORY_STATIC(LogAttentionBudget, Log, All);

UAttentionBudgetComponent::UAttentionBudgetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // Tick every 0.5s for efficiency
}

void UAttentionBudgetComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentSecondStart = GetWorld()->GetTimeSeconds();
	TimeSinceLastBatch = 0.0f;
}

void UAttentionBudgetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ProcessEventQueue(DeltaTime);
}

// ========================================
// Core API
// ========================================

// ========================================
// Core API
// ========================================

void UAttentionBudgetComponent::SubmitObservation(const FSemanticEvent& Event)
{
	if (!Event.Instigator)
	{
		UE_LOG(LogAttentionBudget, Warning, TEXT("SubmitObservation: Invalid Instigator"));
		return;
	}

	// Calculate priority
	FPendingEvent PendingEvent;
	PendingEvent.EventData = Event;
	PendingEvent.PriorityScore = CalculateEventPriority(Event);
	PendingEvent.PriorityCategory = DeterminePriorityCategory(PendingEvent.PriorityScore);

	// Try to get a readable name for logging (Verb or Content)
	FString ActionDesc = Event.Verb.IsValid() ? Event.Verb.ToString() : Event.Content;

	UE_LOG(LogAttentionBudget, Verbose, TEXT("[%s] Observed: %s doing %s (Score: %.2f, Priority: %d)"),
		*GetOwner()->GetName(),
		*Event.Instigator->GetName(),
		*ActionDesc,
		PendingEvent.PriorityScore,
		(int32)PendingEvent.PriorityCategory);

	// Critical events bypass budget and are processed immediately
	if (PendingEvent.PriorityCategory == EEventPriority::Critical)
	{
		UE_LOG(LogAttentionBudget, Log, TEXT("[%s] CRITICAL EVENT: %s"),
			*GetOwner()->GetName(),
			*ActionDesc);

		OnImmediateEvent.Broadcast(Event);
		return;
	}

	// High priority events processed immediately if budget allows
	if (PendingEvent.PriorityScore >= ImmediateProcessingThreshold)
	{
		// Check budget
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - CurrentSecondStart >= 1.0f)
		{
			// New second, reset counter
			CurrentSecondStart = CurrentTime;
			EventsProcessedThisSecond = 0;
		}

		if (EventsProcessedThisSecond < MaxEventsPerSecond)
		{
			OnImmediateEvent.Broadcast(Event);
			EventsProcessedThisSecond++;
			return;
		}
	}

	// Otherwise, add to pending queue
	PendingEvents.Add(PendingEvent);
}

void UAttentionBudgetComponent::FlushPendingEvents()
{
	UE_LOG(LogAttentionBudget, Log, TEXT("[%s] Flushing %d pending events"),
		*GetOwner()->GetName(),
		PendingEvents.Num());

	if (PendingEvents.Num() > 0)
	{
		// Sort by priority
		PendingEvents.Sort([](const FPendingEvent& A, const FPendingEvent& B) {
			return A.PriorityScore > B.PriorityScore;
		});

		// Unwrap and broadcast
		TArray<FSemanticEvent> OutEvents;
		for (const FPendingEvent& Wrapper : PendingEvents)
		{
			OutEvents.Add(Wrapper.EventData);
		}

		OnBatchedEvents.Broadcast(OutEvents);
		PendingEvents.Empty();
	}
}

// ========================================
// Priority Calculation
// ========================================

float UAttentionBudgetComponent::CalculateEventPriority(const FSemanticEvent& Event)
{
	if (!Event.Instigator)
	{
		return 0.0f;
	}

	APawn* MyPawn = Cast<APawn>(GetOwner());
	if (!MyPawn)
	{
		return 0.0f;
	}

	float Score = 0.5f; // Base score

	// ========================================
	// Factor 1: Involves me? (Critical)
	// ========================================
	if (Event.Target == GetOwner())
	{
		// Someone is doing something TO ME!
		return 1.0f; // Critical priority
	}

	// ========================================
	// Factor 2: Distance (closer = more important)
	// ========================================
	float Distance = FVector::Dist(MyPawn->GetActorLocation(), Event.Instigator->GetActorLocation());
	
	// Refined Distance: 100m range (10000 units), linear falloff
	float DistanceScore = FMath::Clamp(1.0f - (Distance / 10000.0f), 0.0f, 1.0f);
	Score *= DistanceScore;

	// ========================================
	// Factor 3: Relationship (friend/enemy > stranger)
	// ========================================
	UFactionReputationComponent* MyFaction = MyPawn->FindComponentByClass<UFactionReputationComponent>();
	if (MyFaction)
	{
		FName ActorFactionID = FactionHelpers::GetFactionID(Event.Instigator);
		if (ActorFactionID != "None")
		{
			float Reputation = MyFaction->GetReputationWith(ActorFactionID);
			float RelationshipScore = FMath::Abs(Reputation) / 100.0f; // 0-1
			Score += RelationshipScore * 0.3f; // Bonus up to +0.3
		}
	}

	// ========================================
	// Factor 4: Action Type (based on Verb / Context Tags)
	// ========================================

	// Determine type from Tags (Semantic Event uses Verb + Context)
	bool bIsCombat = Event.Verb.MatchesTag(AINPCTags::Interaction_Combat) || Event.ContextTags.HasTag(AINPCTags::Directive_Combat);
	bool bIsSocial = Event.Verb.MatchesTag(AINPCTags::Interaction_Social) || Event.ContextTags.HasTag(AINPCTags::Directive_Social);
	bool bIsSurvival = Event.ContextTags.HasTag(AINPCTags::Directive_Survival);
	
	if (bIsCombat || Event.Verb.ToString().Contains("Attack")) 
	{
		Score += 0.3f;
	}
	else if (bIsSurvival)
	{
		Score += 0.25f;
	}
	else if (bIsSocial)
	{
		Score += 0.15f;
	}
	
	// ========================================
	// Factor 5: Target Relationship (if action has a target)
	// ========================================
	if (Event.Target && MyFaction)
	{
		FName TargetFactionID = FactionHelpers::GetFactionID(Event.Target);
		if (TargetFactionID != "None")
		{
			float TargetReputation = MyFaction->GetReputationWith(TargetFactionID);

			// If someone attacks my friend, I care!
			if (TargetReputation > 50.0f && bIsCombat)
			{
				Score += 0.2f;
			}
			// If someone attacks my enemy, also interesting (but less)
			else if (TargetReputation < -50.0f && bIsCombat)
			{
				Score += 0.1f; 
			}
		}
	}

	// ========================================
	// Factor 6: Directive Relevance (Goal Compatibility)
	// ========================================
	UGoalComponent* GoalComp = MyPawn->FindComponentByClass<UGoalComponent>();
	if (GoalComp)
	{
		FGameplayTag CurrentDirective = GoalComp->GetCurrentDirective();
		FGameplayTag EventDirective = Event.GetDirective();
		
		if (EventDirective.IsValid() && CurrentDirective.IsValid())
		{
			if (EventDirective.MatchesTag(CurrentDirective))
			{
				Score += 0.2f; 
			}
			else if (CurrentDirective.MatchesTag(AINPCTags::Directive_Combat))
			{
				// Tunnel vision in combat
				if (!EventDirective.MatchesTag(AINPCTags::Directive_Combat))
				{
					Score -= 0.2f; 
				}
			}
		}
	}

	return FMath::Clamp(Score, 0.0f, 1.0f);
}

EEventPriority UAttentionBudgetComponent::DeterminePriorityCategory(float Score)
{
	if (Score >= 0.9f) return EEventPriority::Critical;
	if (Score >= 0.7f) return EEventPriority::High;
	if (Score >= 0.4f) return EEventPriority::Medium;
	if (Score >= 0.2f) return EEventPriority::Low;
	return EEventPriority::Ambient;
}

// ========================================
// Event Queue Management
// ========================================

void UAttentionBudgetComponent::ProcessEventQueue(float DeltaTime)
{
	if (PendingEvents.Num() == 0)
	{
		return;
	}

	// Reset event counter every second
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - CurrentSecondStart >= 1.0f)
	{
		CurrentSecondStart = CurrentTime;
		EventsProcessedThisSecond = 0;
	}

	// Process high-priority events if budget allows
	if (EventsProcessedThisSecond < MaxEventsPerSecond)
	{
		// Sort by priority
		PendingEvents.Sort([](const FPendingEvent& A, const FPendingEvent& B) {
			return A.PriorityScore > B.PriorityScore;
		});

		// Process top events
		int32 EventsToProcess = FMath::Min((int32)(MaxEventsPerSecond - EventsProcessedThisSecond), PendingEvents.Num());
		for (int32 i = 0; i < EventsToProcess; i++)
		{
			OnImmediateEvent.Broadcast(PendingEvents[i].EventData);
			EventsProcessedThisSecond++;
		}

		// Remove processed events
		PendingEvents.RemoveAt(0, EventsToProcess);
	}

	// Batching logic
	if (bEnableBatching)
	{
		TimeSinceLastBatch += DeltaTime;
		if (TimeSinceLastBatch >= BatchingInterval)
		{
			ProcessBatchedEvents();
			TimeSinceLastBatch = 0.0f;
		}
	}
}

void UAttentionBudgetComponent::ProcessBatchedEvents()
{
	if (PendingEvents.Num() == 0)
	{
		return;
	}

	UE_LOG(LogAttentionBudget, Log, TEXT("[%s] Batching %d low-priority events"),
		*GetOwner()->GetName(),
		PendingEvents.Num());

	// Deduplicate events (Merge similar events from same actor)
	TArray<FSemanticEvent> DedupedEvents;
	TSet<uint32> ProcessedKeys; // Hash of Actor + Verb

	for (const FPendingEvent& Entry : PendingEvents)
	{
		const FSemanticEvent& Event = Entry.EventData;
		
		// Create unique key for Actor + Action
		uint32 Key = GetTypeHash(Event.Instigator);
		if (Event.Verb.IsValid())
		{
			Key = HashCombine(Key, GetTypeHash(Event.Verb));
		}
		else
		{
			Key = HashCombine(Key, GetTypeHash(Event.Content));
		}

		if (!ProcessedKeys.Contains(Key))
		{
			DedupedEvents.Add(Event);
			ProcessedKeys.Add(Key);
		}
	}

	UE_LOG(LogAttentionBudget, Log, TEXT("   -> Deduplicated to %d events"), DedupedEvents.Num());

	OnBatchedEvents.Broadcast(DedupedEvents);
	PendingEvents.Empty();
}

// ========================================
// Helpers
// ========================================

USensoryComponent* UAttentionBudgetComponent::GetSensoryComponent()
{
	if (!CachedSensoryComponent)
	{
		CachedSensoryComponent = GetOwner()->FindComponentByClass<USensoryComponent>();
	}
	return CachedSensoryComponent;
}

UTargetSelectionSubsystem* UAttentionBudgetComponent::GetTargetSelectionSubsystem()
{
	if (GetWorld())
	{
		return GetWorld()->GetSubsystem<UTargetSelectionSubsystem>();
	}
	return nullptr;
}
