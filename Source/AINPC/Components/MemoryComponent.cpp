#include "Components/MemoryComponent.h"
#include "Social/SocialGameplayTags.h"

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

	// Storage: Comprehensive Record (Append Only)
	MemoryStream.Add(NewItem);

	// Accumulate Importance for Reflection Trigger
	CurrentImportanceSum += NewItem.ImportanceScore;

	UE_LOG(LogTemp, Log, TEXT("[Memory] Stored: %s (Imp: %.2f)"), *NewItem.Description, NewItem.ImportanceScore);

	// Check Trigger (Logic only, actual LLM call would be async)
	if (CurrentImportanceSum >= ReflectionThreshold)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Memory] 'Slow System' Reflection Triggered! (Sum: %.2f)"), CurrentImportanceSum);
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
	UE_LOG(LogTemp, Display, TEXT("=== Memory Stream Dump (%d items) ==="), MemoryStream.Num());
	for (const FMemoryItem& Item : MemoryStream)
	{
		UE_LOG(LogTemp, Display, TEXT("[%s] (Imp: %.1f) %s"), *Item.Timestamp.ToString(), Item.ImportanceScore, *Item.Description);
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
	
	UE_LOG(LogTemp, Log, TEXT("[Memory] Consolidating %d Insights..."), NewInsights.Num());
	
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