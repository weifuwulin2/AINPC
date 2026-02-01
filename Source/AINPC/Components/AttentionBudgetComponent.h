// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Events/EventTypes.h"
#include "AttentionBudgetComponent.generated.h"

class UTargetSelectionSubsystem;
class USensoryComponent;

// EEventPriority and FPendingEvent moved to Events/EventTypes.h
// EEventPriority 和 FPendingEvent 已移至 Events/EventTypes.h

/**
 * Attention Budget Component
 *
 * Manages observation attention budget to prevent information overload.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AINPC_API UAttentionBudgetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttentionBudgetComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========================================
	// Configuration (Extensible)
	// ========================================

	/** Maximum events to process per second (attention budget) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention Budget")
	float MaxEventsPerSecond = 2.0f;

	/** Priority threshold for immediate processing (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention Budget")
	float ImmediateProcessingThreshold = 0.7f;

	/** Enable event batching for low-priority events */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention Budget")
	bool bEnableBatching = true;

	/** Batching interval (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention Budget")
	float BatchingInterval = 3.0f;

	// ========================================
	// Core API
	// ========================================

	/**
	 * Submit an observed action event for attention processing.
	 * Will be scored, prioritized, and either processed immediately or batched.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Attention")
	void SubmitObservation(const FSemanticEvent& Event);

	/**
	 * Force process all pending events immediately (e.g., when entering combat)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Attention")
	void FlushPendingEvents();

	// ========================================
	// Events (Extensibility Points)
	// ========================================

	/** Fired when a high-priority event should be processed immediately */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnImmediateEvent, const FSemanticEvent&, Event);

	UPROPERTY(BlueprintAssignable, Category = "AI|Attention")
	FOnImmediateEvent OnImmediateEvent;

	/** Fired when batched events are ready (merged/summarized) */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBatchedEvents, const TArray<FSemanticEvent>&, Events);

	UPROPERTY(BlueprintAssignable, Category = "AI|Attention")
	FOnBatchedEvents OnBatchedEvents;

protected:
	// ========================================
	// Priority Calculation (Reuses TargetSelectionSubsystem)
	// ========================================

	/**
	 * Calculate priority score for an observed event.
	 */
	float CalculateEventPriority(const FSemanticEvent& Event);

	/**
	 * Categorize event priority based on score
	 */
	EEventPriority DeterminePriorityCategory(float Score);

	// ========================================
	// Event Queue Management
	// ========================================

	/** Pending events waiting for processing (Wrapped with Score) */
	TArray<FPendingEvent> PendingEvents;

	/** Events processed this second (for budget tracking) */
	int32 EventsProcessedThisSecond = 0;
	float CurrentSecondStart = 0.0f;

	/** Batching timer */
	float TimeSinceLastBatch = 0.0f;

	/**
	 * Process pending events based on attention budget
	 */
	void ProcessEventQueue(float DeltaTime);

	/**
	 * Process batched low-priority events
	 */
	void ProcessBatchedEvents();

	// ========================================
	// Helpers
	// ========================================

	/** Get sensory component (cached) */
	UPROPERTY()
	USensoryComponent* CachedSensoryComponent = nullptr;

	USensoryComponent* GetSensoryComponent();

	/** Get target selection subsystem (for scoring reuse) */
	UTargetSelectionSubsystem* GetTargetSelectionSubsystem();
};
