
#include "Subsystems/NarrativeDirectorSubsystem.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Social/SocialGameplayTags.h"
#include "Utilities/AINPCHelpers.h"

void UNarrativeDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	HistoryLog.Empty();
	DeadVIPs.Empty();
	FactionPopulation.Empty();
	RelationshipSnapshot.Empty();
}

void UNarrativeDirectorSubsystem::RecordEvent(FString Description, FGameplayTagContainer Tags)
{
	float Time = 0.f;
	if (UWorld* World = GetWorld())
	{
		Time = World->GetTimeSeconds();
	}

	FNarrativeEvent NewEvent(Description, Time);
	NewEvent.Tags = Tags;
	HistoryLog.Add(NewEvent);

	UE_LOG(LogTemp, Log, TEXT("[NarrativeDirector] Recorded: %s | Tags: %s"), *Description, *Tags.ToStringSimple());

	// Broadcast to listeners (NarrativeSquadSubsystem)
	OnEventRecorded.Broadcast(NewEvent);
}

void UNarrativeDirectorSubsystem::RecordEvent(FString Description, TArray<FName> Tags)
{
	// Legacy adapter: Convert FNames to GameplayTags
	FGameplayTagContainer TagContainer;
	for (const FName& TagName : Tags)
	{
		// Try to find exact match first, else add as naive tag if possible (though RequestGameplayTag requires it to exist)
		FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		if (Tag.IsValid())
		{
			TagContainer.AddTag(Tag);
		}
	}
	RecordEvent(Description, TagContainer);
}

void UNarrativeDirectorSubsystem::RecordNPCDeath(AActor* Victim, AActor* Killer)
{
	if (!Victim) return;
	
	FString VictimName = AINPCHelpers::GetSmartActorName(Victim);
	FString KillerName = Killer ? AINPCHelpers::GetSmartActorName(Killer) : TEXT("Unknown");

	FString Desc = FString::Printf(TEXT("%s was killed by %s"), *VictimName, *KillerName);
	
	FGameplayTagContainer Tags;
	Tags.AddTag(AINPCTags::Event_Death);
	
	DeadVIPs.Add(FName(*VictimName));

	RecordEvent(Desc, Tags);
}

void UNarrativeDirectorSubsystem::RecordPlayerAction(FString ActionDescription, TArray<FName> Tags)
{
	// Prefix with Player marker
	FString Desc = FString::Printf(TEXT("[PLAYER] %s"), *ActionDescription);
	
	FGameplayTagContainer TagContainer;
	for (const FName& Name : Tags) TagContainer.AddTag(FGameplayTag::RequestGameplayTag(Name, false));
	
	RecordEvent(Desc, TagContainer);
}

void UNarrativeDirectorSubsystem::RecordRelationshipChange(FName SourceActor, FName TargetActor, float OldValue, float NewValue)
{
	// Create a unique key for this relationship
	FName RelKey = FName(*FString::Printf(TEXT("%s_%s"), *SourceActor.ToString(), *TargetActor.ToString()));
	
	// Update snapshot
	RelationshipSnapshot.Add(RelKey, NewValue);
	
	// Determine change type
	FString ChangeType = (NewValue > OldValue) ? TEXT("improved") : TEXT("worsened");
	FString Desc = FString::Printf(TEXT("%s's relationship with %s %s (%.0f -> %.0f)"), 
		*SourceActor.ToString(), *TargetActor.ToString(), *ChangeType, OldValue, NewValue);
	
	FGameplayTagContainer Tags;
	Tags.AddTag(AINPCTags::Social_Relationship);
	
	RecordEvent(Desc, Tags);
}

void UNarrativeDirectorSubsystem::RecordFactionReputationChange(FName FactionID, float Delta, FString Reason)
{
	FString ChangeType = (Delta > 0) ? TEXT("increased") : TEXT("decreased");
	FString Desc = FString::Printf(TEXT("Reputation with %s %s by %.0f: %s"), 
		*FactionID.ToString(), *ChangeType, FMath::Abs(Delta), *Reason);
	
	FGameplayTagContainer Tags;
	Tags.AddTag(AINPCTags::Social_Faction);
	
	RecordEvent(Desc, Tags);
}

void UNarrativeDirectorSubsystem::UpdateFactionPopulation(FName FactionID, int32 Delta)
{
	int32& Count = FactionPopulation.FindOrAdd(FactionID);
	int32 OldCount = Count;
	Count = FMath::Max(0, Count + Delta);
	
	FString Desc = FString::Printf(TEXT("%s population changed: %d -> %d"), 
		*FactionID.ToString(), OldCount, Count);
	
	FGameplayTagContainer Tags;
	Tags.AddTag(AINPCTags::Social_Faction);
	
	RecordEvent(Desc, Tags);
}

int32 UNarrativeDirectorSubsystem::GetFactionPopulation(FName FactionID) const
{
	const int32* Count = FactionPopulation.Find(FactionID);
	return Count ? *Count : 0;
}

FString UNarrativeDirectorSubsystem::GetWorldStateDescription(int32 MaxEvents) const
{
	FString Output = TEXT("World History:\n");

	int32 Count = 0;
	for (int32 i = HistoryLog.Num() - 1; i >= 0; --i)
	{
		if (Count >= MaxEvents) break;
		
		const FNarrativeEvent& E = HistoryLog[i];
		Output += FString::Printf(TEXT("- [%.0fs ago] %s\n"), (GetWorld()->GetTimeSeconds() - E.Timestamp), *E.Description);
		Count++;
	}

	// Dead NPCs
	if (DeadVIPs.Num() > 0)
	{
		Output += TEXT("Dead NPCs: ");
		for (const FName& Name : DeadVIPs)
		{
			Output += Name.ToString() + TEXT(", ");
		}
		Output += TEXT("\n");
	}

	// Faction Population
	if (FactionPopulation.Num() > 0)
	{
		Output += TEXT("Faction Population: ");
		for (const auto& Pair : FactionPopulation)
		{
			Output += FString::Printf(TEXT("%s=%d, "), *Pair.Key.ToString(), Pair.Value);
		}
		Output += TEXT("\n");
	}

	return Output;
}
