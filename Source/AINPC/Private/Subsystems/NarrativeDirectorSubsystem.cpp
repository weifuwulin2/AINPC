
#include "Subsystems/NarrativeDirectorSubsystem.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

void UNarrativeDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	HistoryLog.Empty();
	DeadVIPs.Empty();
	FactionPopulation.Empty();
	RelationshipSnapshot.Empty();
}

void UNarrativeDirectorSubsystem::RecordEvent(FString Description, TArray<FName> Tags)
{
	float Time = 0.f;
	if (UWorld* World = GetWorld())
	{
		Time = World->GetTimeSeconds();
	}

	FNarrativeEvent NewEvent(Description, Time);
	NewEvent.Tags = Tags;
	HistoryLog.Add(NewEvent);

	UE_LOG(LogTemp, Log, TEXT("[NarrativeDirector] Recorded: %s"), *Description);

	// Broadcast to listeners (NarrativeSquadSubsystem)
	OnEventRecorded.Broadcast(NewEvent);
}

void UNarrativeDirectorSubsystem::RecordNPCDeath(AActor* Victim, AActor* Killer)
{
	if (!Victim) return;

	FString VictimName = Victim->GetActorLabel();
	FString KillerName = Killer ? Killer->GetActorLabel() : TEXT("Unknown");

	FString Desc = FString::Printf(TEXT("%s was killed by %s"), *VictimName, *KillerName);
	
	TArray<FName> Tags;
	Tags.Add(FName("Death"));
	Tags.Add(FName(*FString::Printf(TEXT("Death_%s"), *VictimName)));

	DeadVIPs.Add(FName(*VictimName));

	// Also update faction population if the victim has a faction
	// (This would need faction lookup - for now just record the event)

	RecordEvent(Desc, Tags);
}

void UNarrativeDirectorSubsystem::RecordPlayerAction(FString ActionDescription, TArray<FName> Tags)
{
	// Prefix with Player marker
	FString Desc = FString::Printf(TEXT("[PLAYER] %s"), *ActionDescription);
	
	// Ensure PlayerAction tag is present
	Tags.AddUnique(FName("PlayerAction"));
	
	RecordEvent(Desc, Tags);
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
	
	TArray<FName> Tags;
	Tags.Add(FName("Relationship"));
	Tags.Add(SourceActor);
	Tags.Add(TargetActor);
	
	RecordEvent(Desc, Tags);
}

void UNarrativeDirectorSubsystem::RecordFactionReputationChange(FName FactionID, float Delta, FString Reason)
{
	FString ChangeType = (Delta > 0) ? TEXT("increased") : TEXT("decreased");
	FString Desc = FString::Printf(TEXT("Reputation with %s %s by %.0f: %s"), 
		*FactionID.ToString(), *ChangeType, FMath::Abs(Delta), *Reason);
	
	TArray<FName> Tags;
	Tags.Add(FName("FactionReputation"));
	Tags.Add(FactionID);
	
	RecordEvent(Desc, Tags);
}

void UNarrativeDirectorSubsystem::UpdateFactionPopulation(FName FactionID, int32 Delta)
{
	int32& Count = FactionPopulation.FindOrAdd(FactionID);
	int32 OldCount = Count;
	Count = FMath::Max(0, Count + Delta);
	
	FString Desc = FString::Printf(TEXT("%s population changed: %d -> %d"), 
		*FactionID.ToString(), OldCount, Count);
	
	TArray<FName> Tags;
	Tags.Add(FName("Population"));
	Tags.Add(FactionID);
	
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
