#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Social/SocialTypes.h"
#include "MemoryComponent.generated.h"


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

	// Receive insights from LLM and consolidate them as long-term memory
	UFUNCTION(BlueprintCallable, Category = "AI | Memory")
	void ConsolidateMemories(const TArray<FString>& NewInsights);

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

	// Calculate importance based on event data (Physiological Layer Logic)
	float CalculateImportance(const FSemanticEvent& Event);
};
