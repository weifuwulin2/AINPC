
#include "Subsystems/NarrativeHistorySubsystem.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Social/SocialGameplayTags.h"
#include "Utilities/AINPCHelpers.h"
#include "Utilities/FactionHelpers.h"
#include "Utilities/NPCQueryHelpers.h"
#include "Components/NPCDefinitionComponent.h"
#include "Components/GoalComponent.h"
#include "Components/FactionReputationComponent.h"
#include "Controller/UtilityAIController.h"
#include "EngineUtils.h"

void UNarrativeHistorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	HistoryLog.Empty();
	DeadVIPs.Empty();
	FactionPopulation.Empty();
	RelationshipSnapshot.Empty();
}

void UNarrativeHistorySubsystem::RecordEvent(FString Description, FGameplayTagContainer Tags)
{
	float Time = 0.f;
	if (UWorld* World = GetWorld())
	{
		Time = World->GetTimeSeconds();
	}

	FNarrativeEvent NewEvent(Description, Time);
	NewEvent.Tags = Tags;
	HistoryLog.Add(NewEvent);

	UE_LOG(LogTemp, Log, TEXT("[NarrativeHistory] Recorded: %s | Tags: %s"), *Description, *Tags.ToStringSimple());

	// Broadcast to listeners (NarrativeSquadSubsystem)
	OnEventRecorded.Broadcast(NewEvent);
}

void UNarrativeHistorySubsystem::RecordEvent(FString Description, TArray<FName> Tags)
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

void UNarrativeHistorySubsystem::RecordNPCDeath(AActor* Victim, AActor* Killer)
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

void UNarrativeHistorySubsystem::RecordPlayerAction(FString ActionDescription, TArray<FName> Tags)
{
	// Prefix with Player marker
	FString Desc = FString::Printf(TEXT("[PLAYER] %s"), *ActionDescription);
	
	FGameplayTagContainer TagContainer;
	for (const FName& Name : Tags) TagContainer.AddTag(FGameplayTag::RequestGameplayTag(Name, false));
	
	RecordEvent(Desc, TagContainer);
}

void UNarrativeHistorySubsystem::RecordRelationshipChange(FName SourceActor, FName TargetActor, float OldValue, float NewValue)
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

void UNarrativeHistorySubsystem::RecordFactionReputationChange(FName FactionID, float Delta, FString Reason)
{
	FString ChangeType = (Delta > 0) ? TEXT("increased") : TEXT("decreased");
	FString Desc = FString::Printf(TEXT("Reputation with %s %s by %.0f: %s"), 
		*FactionID.ToString(), *ChangeType, FMath::Abs(Delta), *Reason);
	
	FGameplayTagContainer Tags;
	Tags.AddTag(AINPCTags::Social_Faction);
	
	RecordEvent(Desc, Tags);
}

void UNarrativeHistorySubsystem::UpdateFactionPopulation(FName FactionID, int32 Delta)
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

int32 UNarrativeHistorySubsystem::GetFactionPopulation(FName FactionID) const
{
	const int32* Count = FactionPopulation.Find(FactionID);
	return Count ? *Count : 0;
}

FString UNarrativeHistorySubsystem::GetWorldStateDescription(int32 MaxEvents) const
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

FString UNarrativeHistorySubsystem::GetStructuredWorldSnapshot(int32 MaxRecentEvents) const
{
	FString Output;

	// === Section 1: Critical Events (deaths, faction wars - never expire) ===
	Output += TEXT("[CRITICAL EVENTS (permanent)]:\n");
	bool bHasCritical = false;
	for (const FNarrativeEvent& E : HistoryLog)
	{
		if (E.Tags.HasTag(AINPCTags::Event_Death) || E.Tags.HasTag(AINPCTags::Social_Faction))
		{
			Output += FString::Printf(TEXT("- %s\n"), *E.Description);
			bHasCritical = true;
		}
	}
	if (!bHasCritical)
	{
		Output += TEXT("- (none)\n");
	}

	// === Section 2: Recent Events ===
	Output += FString::Printf(TEXT("\n[RECENT EVENTS (last %d)]:\n"), MaxRecentEvents);
	int32 Count = 0;
	for (int32 i = HistoryLog.Num() - 1; i >= 0 && Count < MaxRecentEvents; --i, ++Count)
	{
		const FNarrativeEvent& E = HistoryLog[i];
		float SecsAgo = GetWorld() ? (GetWorld()->GetTimeSeconds() - E.Timestamp) : 0.0f;
		Output += FString::Printf(TEXT("- [%.0fs ago] %s\n"), SecsAgo, *E.Description);
	}

	// === Section 3: Dead NPCs ===
	Output += TEXT("\n[DEAD NPCs (permanently unavailable)]:\n");
	if (DeadVIPs.Num() > 0)
	{
		for (const FName& Name : DeadVIPs)
		{
			Output += FString::Printf(TEXT("- %s\n"), *Name.ToString());
		}
	}
	else
	{
		Output += TEXT("- (none)\n");
	}

	// === Section 4: Faction State ===
	Output += TEXT("\n[FACTION STATE]:\n");
	if (FactionPopulation.Num() > 0)
	{
		for (const auto& Pair : FactionPopulation)
		{
			Output += FString::Printf(TEXT("- %s: population=%d\n"), *Pair.Key.ToString(), Pair.Value);
		}
	}
	else
	{
		Output += TEXT("- (no faction data)\n");
	}

	// === Section 5: Living NPCs ===
	Output += TEXT("\n");
	Output += GetLivingNPCRoster();

	// === Section 6: NPC Relationships (Social Bonds) ===
	Output += TEXT("\n[NPC RELATIONSHIPS]:\n");
	bool bHasRelationships = false;
	if (UWorld* WorldPtr = GetWorld())
	{
		TArray<AActor*> LivingNPCs = NPCQueryHelpers::GetAllLivingNPCs(WorldPtr);
		for (AActor* NPC : LivingNPCs)
		{
			if (!IsValid(NPC)) continue;

			UFactionReputationComponent* RepComp = NPC->FindComponentByClass<UFactionReputationComponent>();
			if (!RepComp) continue;

			// Check controller/pawn duality
			if (RepComp->SocialBonds.Num() == 0)
			{
				if (APawn* Pawn = Cast<APawn>(NPC))
				{
					if (AController* Ctrl = Pawn->GetController())
					{
						RepComp = Ctrl->FindComponentByClass<UFactionReputationComponent>();
					}
				}
			}

			if (!RepComp || RepComp->SocialBonds.Num() == 0) continue;

			FString OwnerName = AINPCHelpers::GetSmartActorName(NPC);
			for (const auto& BondPair : RepComp->SocialBonds)
			{
				const FSocialBond& Bond = BondPair.Value;
				if (Bond.Type == ESocialBondType::None) continue;

				FString BondTypeStr = UEnum::GetValueAsString(Bond.Type);
				FString CleanType;
				if (BondTypeStr.Split(TEXT("::"), nullptr, &CleanType))
				{
					BondTypeStr = CleanType;
				}

				if (!Bond.RelationshipSummary.IsEmpty())
				{
					Output += FString::Printf(TEXT("- %s -> %s: %s (%s)\n"),
						*OwnerName, *BondPair.Key.ToString(), *BondTypeStr, *Bond.RelationshipSummary);
				}
				else
				{
					Output += FString::Printf(TEXT("- %s -> %s: %s\n"),
						*OwnerName, *BondPair.Key.ToString(), *BondTypeStr);
				}
				bHasRelationships = true;
			}
		}
	}
	if (!bHasRelationships)
	{
		Output += TEXT("- (no notable relationships)\n");
	}

	// === Section 7: Recent Plot Threads ===
	Output += TEXT("\n");
	Output += GetRecentPlotThreads();

	return Output;
}

FString UNarrativeHistorySubsystem::GetLivingNPCRoster() const
{
	FString Output = TEXT("[LIVING NPCs (available for scenes)]:\n");

	UWorld* World = GetWorld();
	if (!World)
	{
		Output += TEXT("- (no world context)\n");
		return Output;
	}

	TArray<AActor*> LivingNPCs = NPCQueryHelpers::GetAllLivingNPCs(World);
	if (LivingNPCs.Num() == 0)
	{
		Output += TEXT("- (none)\n");
		return Output;
	}

	for (AActor* NPC : LivingNPCs)
	{
		if (!IsValid(NPC)) continue;

		FString Name = AINPCHelpers::GetSmartActorName(NPC);
		FName Faction = FactionHelpers::GetFactionID(NPC);
		FName Profession = NAME_None;

		if (UNPCDefinitionComponent* DefComp = AINPCHelpers::GetNPCDefinitionComponent(NPC))
		{
			Profession = DefComp->ProfessionID;
		}

		// Get emotion state from controller
		FString EmotionStr = TEXT("Unknown");
		if (APawn* Pawn = Cast<APawn>(NPC))
		{
			if (AUtilityAIController* AICtrl = Cast<AUtilityAIController>(Pawn->GetController()))
			{
				EmotionStr = UEnum::GetValueAsString(AICtrl->CurrentEmotion);
				// Clean up "EEmotionState::" prefix
				FString CleanStr;
				if (EmotionStr.Split(TEXT("::"), nullptr, &CleanStr))
				{
					EmotionStr = CleanStr;
				}
			}
		}

		// Check if in scene
		bool bInScene = false;
		if (UGoalComponent* GoalComp = AINPCHelpers::GetGoalComponent(NPC))
		{
			bInScene = GoalComp->HasContextTag(AINPCTags::Status_InScene);
		}

		Output += FString::Printf(TEXT("- %s (Faction: %s, Profession: %s, Emotion: %s%s)\n"),
			*Name,
			Faction.IsNone() ? TEXT("None") : *Faction.ToString(),
			Profession.IsNone() ? TEXT("Unknown") : *Profession.ToString(),
			*EmotionStr,
			bInScene ? TEXT(", IN SCENE") : TEXT(""));
	}

	return Output;
}

FString UNarrativeHistorySubsystem::GetRecentPlotThreads(int32 MaxThreads) const
{
	FString Output = TEXT("[RECENT PLOT THREADS (do NOT repeat)]:\n");

	if (CompletedPlotThreads.Num() == 0)
	{
		Output += TEXT("- (none)\n");
		return Output;
	}

	int32 StartIdx = FMath::Max(0, CompletedPlotThreads.Num() - MaxThreads);
	for (int32 i = CompletedPlotThreads.Num() - 1; i >= StartIdx; --i)
	{
		const FPlotThread& Thread = CompletedPlotThreads[i];
		Output += FString::Printf(TEXT("- [%s] %s\n"), *Thread.DramaticAction, *Thread.PlotOutline);
	}

	return Output;
}

FString UNarrativeHistorySubsystem::GetDeadNPCList() const
{
	FString Output;
	for (const FName& Name : DeadVIPs)
	{
		if (!Output.IsEmpty()) Output += TEXT(", ");
		Output += Name.ToString();
	}
	return Output;
}

void UNarrativeHistorySubsystem::RecordPlotThread(const FString& PlotOutline, const FString& DramaticAction)
{
	FPlotThread Thread;
	Thread.PlotOutline = PlotOutline;
	Thread.DramaticAction = DramaticAction;
	Thread.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	CompletedPlotThreads.Add(Thread);

	// Trim old threads
	while (CompletedPlotThreads.Num() > MaxPlotThreadHistory)
	{
		CompletedPlotThreads.RemoveAt(0);
	}
}


