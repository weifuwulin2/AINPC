#include "Components/MemoryComponent.h"
#include "Social/SocialGameplayTags.h"
#include "AINPC.h"

UMemoryComponent::UMemoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ReflectionThreshold = 20.0f;
}

void UMemoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UMemoryComponent::CommitEvent(const FSemanticEvent& Event)
{
	FMemoryItem NewItem;
	NewItem.Description = Event.Content;
	NewItem.Tags.AddTag(Event.Verb);
	NewItem.ImportanceScore = CalculateImportance(Event);
	NewItem.Timestamp = FDateTime::Now();
	
	// ✅ Phase 4 P1: Set game time for decay calculation
	if (GetWorld())
	{
		NewItem.GameTimeSeconds = GetWorld()->GetTimeSeconds();
	}

	// Storage: Comprehensive Record (Append Only)
	MemoryStream.Add(NewItem);
	
	// ✅ Cleanup if exceeding max size
	CleanupOldMemories();

	// Accumulate Importance for Reflection Trigger
	CurrentImportanceSum += NewItem.ImportanceScore;

	MEMORY_LOG(Log, "Stored: %s (Imp: %.2f, Total: %d)", *NewItem.Description, NewItem.ImportanceScore, MemoryStream.Num());

	// Check Trigger (Logic only, actual LLM call would be async)
	if (CurrentImportanceSum >= ReflectionThreshold)
	{
		MEMORY_LOG(Warning, "'Slow System' Reflection Triggered! (Sum: %.2f)", CurrentImportanceSum);
		// Reset accumulator after triggering (in a real system, reset after successful reflection)
		CurrentImportanceSum = 0.0f; 
		
		// Note: The actual call to LLM is better handled by CognitionComponent listening or polling.
		// However, for simplicity given current roadmap, we can rely on the Timer in UtilityAIController for "Dreaming".
		// Or we could broadcast an event here "OnReflectionNeeded".
	}
}

float UMemoryComponent::CalculateImportance(const FSemanticEvent& Event)
{
	// Physiological Layer Logic: Heuristics based on Tags and Magnitude
	float BaseScore = Event.Magnitude * 10.0f; // Scale 0-1 to 0-10

	if (Event.Verb.MatchesTag(AINPCTags::Event_Danger))
	{
		BaseScore += 5.0f; // Danger is always important
	}
	else if (Event.Verb.MatchesTag(AINPCTags::Social_Conflict))
	{
		BaseScore += 3.0f; // Conflict is memorable
	}
	else if (Event.Verb.MatchesTag(AINPCTags::Interaction_Mundane))
	{
		BaseScore *= 0.1f; // Mundane things fade fast
	}

	return FMath::Clamp(BaseScore, 0.1f, 10.0f);
}

TArray<FMemoryItem> UMemoryComponent::RetrieveRelevantMemories(const FString& QueryContext, int32 Limit)
{
	if (MemoryStream.Num() == 0) return TArray<FMemoryItem>();

	// Scoring: Score = Recency + Importance + Relevance
	struct FScoredMemory
	{
		int32 Index;
		float Score;
	};

	TArray<FScoredMemory> ScoredMemories;
	FDateTime Now = FDateTime::Now();
	
	// Simple Relevance: check keyword overlap
	TArray<FString> Keywords;
	QueryContext.ParseIntoArray(Keywords, TEXT(" "), true);

	for (int32 i = 0; i < MemoryStream.Num(); ++i)
	{
		const FMemoryItem& Item = MemoryStream[i];

		// 1. Recency (Exponential Decay)
		FTimespan Age = Now - Item.Timestamp;
		double HoursOld = Age.GetTotalHours();
		float RecencyScore = 1.0f / (1.0f + 0.5f * (float)HoursOld); // Decay factor

		// 2. Importance
		float ImportanceScore = Item.ImportanceScore / 10.0f; // Normalize 0-1

		// 3. Relevance (Simple Keyword Match)
		float RelevanceScore = 0.0f;
		for (const FString& Word : Keywords)
		{
			if (Word.Len() > 3)
			{
				// Match against Description
				if (Item.Description.Contains(Word))
				{
					RelevanceScore += 0.5f;
				}

				// Match against Tags (Key Update: Allow querying by Tag semantics)
				// E.g. Query "Danger" matches tag "Event.Danger"
				if (Item.Tags.ToString().Contains(Word))
				{
					RelevanceScore += 1.0f; // Matching a structural tag is highly relevant
				}
			}
		}

		// Final Score
		float FinalScore = (RecencyScore * 1.0f) + (ImportanceScore * 1.0f) + (RelevanceScore * 2.0f);

		ScoredMemories.Add({ i, FinalScore });
	}

	// Sort by Score Descending
	ScoredMemories.Sort([](const FScoredMemory& A, const FScoredMemory& B) {
		return A.Score > B.Score;
	});

	// Return Top N
	TArray<FMemoryItem> Results;
	for (int32 i = 0; i < FMath::Min(Limit, ScoredMemories.Num()); ++i)
	{
		Results.Add(MemoryStream[ScoredMemories[i].Index]);
	}

	return Results;
}

void UMemoryComponent::DumpMemoryLog()
{
	MEMORY_LOG(Display, "=== Memory Stream Dump (%d items) ===", MemoryStream.Num());
	for (const FMemoryItem& Item : MemoryStream)
	{
		MEMORY_LOG(Display, "[%s] (Imp: %.1f) %s", *Item.Timestamp.ToString(), Item.ImportanceScore, *Item.Description);
	}
}

FString UMemoryComponent::GetAllRecentMemoriesAsString()
{
	FString Result = "";
	for (const FMemoryItem& Item : MemoryStream)
	{
		// TODO: Could filter by "Recent" flag if we had one, but dumping all for now
		Result += FString::Printf(TEXT("- [%s] %s\n"), *Item.Timestamp.ToString(), *Item.Description);
	}
	return Result;
}

void UMemoryComponent::ConsolidateMemories(const TArray<FString>& NewInsights)
{
	// 1. In a real system, you might archive old memories here.
	// For now, we simply ADD the insights as new, high-importance memories.
	
	MEMORY_LOG(Log, "Consolidating %d Insights...", NewInsights.Num());
	
	for (const FString& Insight : NewInsights)
	{
		FMemoryItem NewItem;
		NewItem.Description = FString::Printf(TEXT("[REFLECTION] %s"), *Insight);
		NewItem.Timestamp = FDateTime::Now();
		NewItem.ImportanceScore = 10.0f; // Insights are very important
		// Could add a "Reflection" tag here
		
		MemoryStream.Add(NewItem);
	}
	
	// Optional: Clear low-importance short-term memories?
	// MemoryStream.RemoveAll(...)
}

// ============================================
// Phase 4 P1: Temporal Formatting & Decay
// ============================================

FString UMemoryComponent::GetFormattedDescription(const FMemoryItem& Item, float CurrentGameTime) const
{
	float SecondsAgo = CurrentGameTime - Item.GameTimeSeconds;
	float MinutesAgo = SecondsAgo / 60.0f;
	
	FString Prefix;
	
	// 1. Resolved memories - already handled
	if (Item.bIsResolved)
	{
		Prefix = TEXT("[RESOLVED] ");
	}
	// 2. Very recent - still fresh
	else if (MinutesAgo < 0.5f)
	{
		Prefix = TEXT("[JUST NOW] ");
	}
	// 3. Recent - a few minutes ago
	else if (MinutesAgo < 5.0f)
	{
		Prefix = FString::Printf(TEXT("[%d MIN AGO] "), FMath::CeilToInt(MinutesAgo));
	}
	// 4. Old memories - history
	else
	{
		Prefix = TEXT("[HISTORY] ");
	}
	
	return Prefix + Item.Description;
}

float UMemoryComponent::CalculateRelevanceScore(const FMemoryItem& Item, float CurrentGameTime) const
{
	// 1. Half-life decay factor
	float TimeDelta = CurrentGameTime - Item.GameTimeSeconds;
	float TimeFactor = 1.0f / (1.0f + (TimeDelta / HalfLifeSeconds));
	
	// 2. Resolved memory penalty (already handled, lower priority)
	if (Item.bIsResolved)
	{
		TimeFactor *= 0.3f;
	}
	
	return Item.ImportanceScore * TimeFactor;
}

void UMemoryComponent::CleanupOldMemories()
{
	if (MemoryStream.Num() < MaxMemorySize)
	{
		return;
	}
	
	// ✅ Instead of brute-force deletion, trigger Dreaming for consolidation
	// This will broadcast to CognitionComponent which will:
	// 1. Call GetAllRecentMemoriesAsString()
	// 2. Send to LLM for analysis
	// 3. Call ConsolidateMemories() with insights
	// 4. Call ClearShortTermMemories()
	
	MEMORY_LOG(Warning, "🧠 Memory full (%d/%d)! Triggering Dream consolidation...", MemoryStream.Num(), MaxMemorySize);
	OnDreamingNeeded.Broadcast();
}

void UMemoryComponent::ClearShortTermMemories()
{
	// Keep only high-importance consolidated memories (REFLECTION)
	int32 OldCount = MemoryStream.Num();
	
	MemoryStream.RemoveAll([](const FMemoryItem& Item) {
		// Keep if it's a reflection/insight (high importance long-term memory)
		return !Item.Description.Contains(TEXT("[REFLECTION]"));
	});
	
	MEMORY_LOG(Log, "Cleared short-term memories: %d -> %d", OldCount, MemoryStream.Num());
}

void UMemoryComponent::MarkMemoriesResolvedForActor(const FString& ActorName)
{
	int32 ResolvedCount = 0;
	
	for (FMemoryItem& Item : MemoryStream)
	{
		// Mark memories containing the actor's name as resolved (but not already resolved)
		if (!Item.bIsResolved && Item.Description.Contains(ActorName))
		{
			Item.bIsResolved = true;
			ResolvedCount++;
		}
	}
	
	if (ResolvedCount > 0)
	{
		MEMORY_LOG(Log, "Marked %d memories about '%s' as RESOLVED", ResolvedCount, *ActorName);
	}
}

void UMemoryComponent::MarkAllHostileMemoriesResolved()
{
	int32 ResolvedCount = 0;
	
	for (FMemoryItem& Item : MemoryStream)
	{
		// Check for hostile keywords
		// 检查敌对关键词：HOSTILE, ATTACK, DANGER, THREAT
		bool bIsHostile = Item.Description.Contains(TEXT("HOSTILE")) || 
						  Item.Description.Contains(TEXT("ATTACK")) ||
						  Item.Description.Contains(TEXT("DANGER")) ||
						  Item.Tags.HasTag(FGameplayTag::RequestGameplayTag(FName("Event.Danger")));
						  
		if (!Item.bIsResolved && bIsHostile)
		{
			Item.bIsResolved = true;
			ResolvedCount++;
		}
	}
	
	if (ResolvedCount > 0)
	{
		MEMORY_LOG(Log, "Safety Restored: Marked %d HOSTILE memories as RESOLVED", ResolvedCount);
	}
}