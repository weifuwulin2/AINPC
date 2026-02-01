// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TargetSelectionSubsystem.generated.h"

class UCognitionComponent;
class UFactionReputationComponent;
class USensoryComponent;

/** Context for target selection - what is the target needed for? */
UENUM(BlueprintType)
enum class ETargetSelectionContext : uint8
{
	Combat      UMETA(DisplayName = "Combat"),           // Attack action
	Social      UMETA(DisplayName = "Social"),           // Speak/Gossip action
	Follow      UMETA(DisplayName = "Follow"),           // Follow action
	Trade       UMETA(DisplayName = "Trade"),            // Trade/Barter
	Help        UMETA(DisplayName = "Help"),             // Assist ally
	Investigate UMETA(DisplayName = "Investigate")       // Investigate event
};

/** Configuration for target selection */
USTRUCT(BlueprintType)
struct FTargetSelectionConfig
{
	GENERATED_BODY()

	/** Use LLM for intelligent selection (slow but narrative-aware) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Selection")
	bool bUseLLMEnhancement = false;

	/** Maximum distance to consider targets (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Selection")
	float MaxDistance = 3000.0f;

	/** Minimum reputation to be considered friendly (-100 to 100) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Selection")
	float FriendlyReputationThreshold = 50.0f;

	/** Minimum reputation to be considered hostile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Selection")
	float HostileReputationThreshold = -50.0f;
};

/**
 * Universal target selection service.
 * Provides intelligent target selection for all actions that need a target.
 * 
 * Features:
 * - Rule-based scoring (fast, deterministic)
 * - Optional LLM-enhanced selection (slow, narrative-aware)
 * - Caching mechanism (5s default TTL)
 * - Event-driven cache invalidation
 */
UCLASS(Config=Game)
class AINPC_API UTargetSelectionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	
	virtual TStatId GetStatId() const override;
	// ========================================
	// Events
	// ========================================
	
	/** Delegate fired when a cached target becomes invalid (died, left range, etc.) */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTargetInvalidated, AAIController*, Controller, AActor*, OldTarget);
	
	/** Broadcast when a cached target is no longer valid */
	UPROPERTY(BlueprintAssignable, Category = "AI|Target Selection")
	FOnTargetInvalidated OnTargetInvalidated;

	// ========================================
	// Core API
	// ========================================
	
	/**
	 * Select best target for the given context.
	 * @param Controller - The AI controller requesting target
	 * @param Context - What is the target needed for? (Combat, Social, etc.)
	 * @param Config - Configuration for selection criteria
	 * @return Best target actor, or nullptr if none found
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Target Selection")
	AActor* SelectTarget(
		AAIController* Controller,
		ETargetSelectionContext Context,
		const FTargetSelectionConfig& Config
	);

	/**
	 * Get all valid target candidates for the given context.
	 * Filters out dead, invalid, or inappropriate targets.
	 */
	TArray<AActor*> GetTargetCandidates(
		AAIController* Controller,
		ETargetSelectionContext Context,
		const FTargetSelectionConfig& Config,
		APawn* MyPawn,
		UFactionReputationComponent* FactionComp
	);

	/** Clear cached results for a specific controller (e.g., when situation changes dramatically) */
	UFUNCTION(BlueprintCallable, Category = "AI|Target Selection")
	void InvalidateCache(AAIController* Controller);

	/**
	 * Notify subsystem that a target has died.
	 * This triggers immediate cache invalidation and broadcasts OnTargetInvalidated.
	 * Call this from death handlers (e.g., SensoryComponent::HandleDeath).
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Target Selection")
	void NotifyTargetDied(AActor* DeadTarget);

private:
	/** Rule-based scoring for quick target selection */
	AActor* SelectTargetByScoring(
		AAIController* Controller,
		const TArray<AActor*>& Candidates,
		ETargetSelectionContext Context,
		const FTargetSelectionConfig& Config,
		APawn* MyPawn,
		UCognitionComponent* Cognition,
		UFactionReputationComponent* FactionComp
	);

	/** LLM-enhanced selection for narrative consistency */
	AActor* SelectTargetByLLM(
		AAIController* Controller,
		const TArray<AActor*>& Candidates,
		ETargetSelectionContext Context,
		UCognitionComponent* Cognition
	);

	/** Calculate score for a target in given context */
	float CalculateTargetScore(
		AActor* Target,
		APawn* MyPawn,
		ETargetSelectionContext Context,
		UCognitionComponent* Cognition,
		UFactionReputationComponent* FactionComp
	);

	/** Check if target is valid for the given context */
	bool IsValidTarget(
		AActor* Target,
		APawn* MyPawn,
		ETargetSelectionContext Context,
		const FTargetSelectionConfig& Config,
		UFactionReputationComponent* FactionComp
	);

	/** Get faction ID for an actor */
	FName GetActorFaction(AActor* Actor);

	/** Get focus actor for an actor (if it's an AI) */
	AActor* GetActorFocus(AActor* Actor);

	// ========================================
	// Performance Optimization: Caching
	// ========================================

	/** Cache key for target selection results (immutable lookup key) */
	struct FTargetCacheKey
	{
		AAIController* Controller;
		ETargetSelectionContext Context;

		bool operator==(const FTargetCacheKey& Other) const
		{
			return Controller == Other.Controller && Context == Other.Context;
		}

		friend uint32 GetTypeHash(const FTargetCacheKey& Key)
		{
			return HashCombine(GetTypeHash(Key.Controller), (uint32)Key.Context);
		}
	};

	/** Cache entry containing target and timestamp (mutable cached data) */
	struct FTargetCacheEntry
	{
		TWeakObjectPtr<AActor> Target;
		float CachedTime = 0.0f;
	};

	/** Cached target selection results */
	TMap<FTargetCacheKey, FTargetCacheEntry> TargetCache;

	/** Cache validity duration (seconds) */
	UPROPERTY(Config)
	float CacheDuration = 5.0f;

	/** Check if cached result is still valid */
	bool IsCacheValid(const FTargetCacheEntry& Entry) const;

	/** Get cached target if valid */
	AActor* GetCachedTarget(
		AAIController* Controller, 
		ETargetSelectionContext Context,
		const FTargetSelectionConfig& Config,
		APawn* MyPawn,
		UFactionReputationComponent* FactionComp
	);

	/** Cache a target selection result */
	void CacheTarget(AAIController* Controller, ETargetSelectionContext Context, AActor* Target);
};
