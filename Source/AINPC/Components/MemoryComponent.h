#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Social/SocialTypes.h"
#include "MemoryComponent.generated.h"

// ✅ Delegate: Broadcast when memory is full and needs dreaming
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDreamingNeeded);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API UMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:    
	UMemoryComponent();

protected:
	virtual void BeginPlay() override;

public:
	// --- Phase 4: Storage (Comprehensive Record) ---

	/**
	 * Core Input: Commit a semantic event to the comprehensive memory stream.
	 * Calculates importance, timestamps it, and stores it.
	 * Also triggers the check for Reflection (Slow System).
	 */
	UFUNCTION(BlueprintCallable, Category = "AI | Memory")
	void CommitEvent(const FSemanticEvent& Event);

	// --- Phase 5: Retrieval (RAG) ---

	/**
	 * Finds the most relevant memories for the given context.
	 * Score = Recency + Importance + Relevance.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI | Memory")
	TArray<FMemoryItem> RetrieveRelevantMemories(const FString& QueryContext, int32 Limit = 5);

	// --- Debug ---
	
	UFUNCTION(BlueprintCallable, Category = "AI | Debug")
	void DumpMemoryLog();

	// --- Phase 6: Reflection Helpers ---

	// Dumping all memories for "Dreaming" (The Reflection process)
	UFUNCTION(BlueprintCallable, Category = "AI | Memory")
	FString GetAllRecentMemoriesAsString();

	// Get top N memories by importance score (for token-efficient Dreaming)
	UFUNCTION(BlueprintCallable, Category = "AI | Memory")
	FString GetTopMemoriesAsString(int32 Limit = 15);

	// Receive insights from LLM and consolidate them as long-term memory
	UFUNCTION(BlueprintCallable, Category = "AI | Memory")
	void ConsolidateMemories(const TArray<FString>& NewInsights);
	
	// ✅ Delegate: Fires when memory reaches MaxMemorySize and needs Dreaming
	UPROPERTY(BlueprintAssignable, Category = "AI | Memory")
	FOnDreamingNeeded OnDreamingNeeded;
	
	// Clear all short-term memories (called after Dreaming consolidation)
	void ClearShortTermMemories();
	
	// ✅ Mark all memories about a specific actor as resolved (e.g., when target dies)
	UFUNCTION(BlueprintCallable, Category = "AI | Memory")
	void MarkMemoriesResolvedForActor(const FString& ActorName);
	
	// ✅ Mark all HOSTILE memories as resolved (e.g., when feeling safe again)
	UFUNCTION(BlueprintCallable, Category = "AI | Memory")
	void MarkAllHostileMemoriesResolved();
	
	// ✅ Phase 4 P1: Temporal Formatting - Get description with time prefix
	FString GetFormattedDescription(const FMemoryItem& Item, float CurrentGameTime) const;
	
protected:
	// The stream of all memories (Comprehensive Record)
	UPROPERTY(VisibleAnywhere, Category = "Memory")
	TArray<FMemoryItem> MemoryStream;

	// --- Reflection Triggers ---

	// Accumulated importance since last reflection
	UPROPERTY(VisibleAnywhere, Category = "Memory | Reflection")
	float CurrentImportanceSum = 0.0f;

	// Threshold to trigger a reflection cycle (The "Capacity" of short term buffer)
	UPROPERTY(EditDefaultsOnly, Category = "Memory | Reflection")
	float ReflectionThreshold = 20.0f;

	// ✅ Phase 4 P1: Decay Configuration
	// Half-life for memory decay (seconds)
	UPROPERTY(EditDefaultsOnly, Category = "Memory | Decay")
	float HalfLifeSeconds = 120.0f;
	
	// ✅ Maximum memories to keep (prevents unbounded growth)
	UPROPERTY(EditDefaultsOnly, Category = "Memory | Decay")
	int32 MaxMemorySize = 30;
	
	// Clean up old low-importance memories when limit exceeded
	void CleanupOldMemories();

	// Calculate importance based on event data (Physiological Layer Logic)
	float CalculateImportance(const FSemanticEvent& Event);
	
	
	// ✅ Phase 4 P1: Decay-weighted relevance score
	float CalculateRelevanceScore(const FMemoryItem& Item, float CurrentGameTime) const;
};
