#include "Characters/NarrativeCompanion.h"
#include "Components/CognitionComponent.h"
#include "Components/SensoryComponent.h"
#include "Subsystems/NarrativeHistorySubsystem.h"
#include "Subsystems/PlayerSquadSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"

ANarrativeCompanion::ANarrativeCompanion()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ANarrativeCompanion::BeginPlay()
{
	Super::BeginPlay();

	// Register with Player Squad
	if (UWorld* World = GetWorld())
	{
		if (UPlayerSquadSubsystem* SquadSys = World->GetSubsystem<UPlayerSquadSubsystem>())
		{
			SquadSys->RegisterPartyMember(this);
		}
		
		// Attempt to bind sensory system (might need delay for Controller)
		BindSensorySystem();
		
		// Retry in 1s just in case Controller wasn't ready
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, this, &ANarrativeCompanion::BindSensorySystem, 1.0f, false);
	}
}

void ANarrativeCompanion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsFollowing)
	{
		AAIController* AI = Cast<AAIController>(GetController());
		if (!AI) return;

		// Don't follow if we are busy fighting (have a focus actor)
		if (AI->GetFocusActor()) return;

		if (AActor* Leader = GetLeader())
		{
			float DistSq = GetSquaredDistanceTo(Leader);
			if (DistSq > (FollowDistance * FollowDistance))
			{
				// Simple MoveTo
				// Optimization: Could check MoveStatus to avoid spamming MoveTo
				AI->MoveToActor(Leader, FollowDistance - 50.f);
			}
		}
	}
}

UCognitionComponent* ANarrativeCompanion::GetCognitionComp() const
{
	if (AController* Con = GetController())
	{
		return Con->FindComponentByClass<UCognitionComponent>();
	}
	return nullptr;
}

// ===========================================
// AUTOMATIC SENSORY BINDING
// ===========================================

void ANarrativeCompanion::BindSensorySystem()
{
	if (bSensoryBound) return;

	// Access SensoryComponent via Controller (it lives there)
	if (AController* Con = GetController())
	{
		if (USensoryComponent* Sensory = Con->FindComponentByClass<USensoryComponent>())
		{
			// Bind to the Semantic Event output
			Sensory->OnSemanticEventSensed.AddDynamic(this, &ANarrativeCompanion::OnSensoryEvent);
			bSensoryBound = true;
			UE_LOG(LogTemp, Log, TEXT("[NarrativeCompanion] Successfully bound to SensoryComponent."));
		}
	}
}

void ANarrativeCompanion::OnSensoryEvent(const FSemanticEvent& Event)
{
	// Translate Semantic Event (Perception) -> Narrative Record (History)
	
	// 1. Determine Tags
	TArray<FName> RecordTags;
	RecordTags.Add(Event.Verb.GetTagName()); // e.g. "Event.Death.Witnessed"
	
	if (Event.Target)
	{
		// Tag specific actors involved
		// We could add Faction tags here if we had access to them easily
	}

	// 2. Determine Event Type & Call appropriate Director API
	
	if (Event.Verb.MatchesTag(FGameplayTag::RequestGameplayTag("Event.Death")))
	{
		// Filter: Only record VIP deaths to global history to avoid spam
		bool bIsVIP = false;
		if (Event.Target)
		{
			bIsVIP = Event.Target->ActorHasTag("VIP") || 
					 Event.Target->ActorHasTag("Boss") || 
					 Event.Target->ActorHasTag("Important");
		}

		if (bIsVIP)
		{
			// Death Event - Only for Important Characters
			ObserveNPCDeath(Event.Target, Event.Instigator);
		}
		else
		{
			// For non-VIPs, we might just comment locally but NOT record to Director
			if (UCognitionComponent* Cognition = GetCognitionComp())
			{
				// Local reaction only
				Cognition->ProcessStimulus(FString::Printf(TEXT("I saw %s die."), *Event.Target->GetName()));
			}
		}
	}
	else if (Event.Verb.MatchesTag(FGameplayTag::RequestGameplayTag("Social.Relationship")))
	{
		// Relationship Change (if Sensory supports this tag)
		// For now, generic record
		ObserveEvent(Event.Content, RecordTags);
	}
	else if (Event.Instigator && Event.Instigator->ActorHasTag("Player"))
	{
		// Player Action
		ObservePlayerAction(Event.Content, RecordTags);
	}
	else
	{
		// Generic significant event
		if (Event.Magnitude > 0.3f) // Only record somewhat important things
		{
			ObserveEvent(Event.Content, RecordTags);
		}
	}
}

// ===========================================
// MANUAL OBSERVATION API IMPLEMENTATIONS
// ===========================================

void ANarrativeCompanion::ObserveEvent(FString Description, TArray<FName> EventTags)
{
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeHistorySubsystem* Director = World->GetSubsystem<UNarrativeHistorySubsystem>())
		{
			FString FinalDesc = FString::Printf(TEXT("[Witnessed] %s"), *Description);
			Director->RecordEvent(FinalDesc, EventTags);

			// Comment on it
			if (UCognitionComponent* Cognition = GetCognitionComp())
			{
				Cognition->ProcessStimulus(FString::Printf(TEXT("I just saw: %s"), *Description));
			}
		}
	}
}

void ANarrativeCompanion::ObserveNPCDeath(AActor* Victim, AActor* Killer)
{
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeHistorySubsystem* Director = World->GetSubsystem<UNarrativeHistorySubsystem>())
		{
			Director->RecordNPCDeath(Victim, Killer);

			// Comment on the death
			if (UCognitionComponent* Cognition = GetCognitionComp())
			{
				FString VictimName = Victim ? Victim->GetActorLabel() : TEXT("Someone");
				Cognition->ProcessStimulus(FString::Printf(TEXT("I just witnessed %s die!"), *VictimName));
			}
		}
	}
}

void ANarrativeCompanion::ObservePlayerAction(FString ActionDescription, TArray<FName> ActionTags)
{
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeHistorySubsystem* Director = World->GetSubsystem<UNarrativeHistorySubsystem>())
		{
			Director->RecordPlayerAction(ActionDescription, ActionTags);
		}
	}
}

void ANarrativeCompanion::ObserveRelationshipChange(FName SourceActor, FName TargetActor, float OldValue, float NewValue)
{
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeHistorySubsystem* Director = World->GetSubsystem<UNarrativeHistorySubsystem>())
		{
			Director->RecordRelationshipChange(SourceActor, TargetActor, OldValue, NewValue);
		}
	}
}

void ANarrativeCompanion::ObserveFactionReputationChange(FName FactionID, float Delta, FString Reason)
{
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeHistorySubsystem* Director = World->GetSubsystem<UNarrativeHistorySubsystem>())
		{
			Director->RecordFactionReputationChange(FactionID, Delta, Reason);

			// Comment if significant change
			if (FMath::Abs(Delta) >= 10.f)
			{
				if (UCognitionComponent* Cognition = GetCognitionComp())
				{
					FString ChangeType = (Delta > 0) ? TEXT("like us more") : TEXT("hate us now");
					Cognition->ProcessStimulus(FString::Printf(TEXT("The %s faction will %s!"), *FactionID.ToString(), *ChangeType));
				}
			}
		}
	}
}

void ANarrativeCompanion::ObserveFactionPopulationChange(FName FactionID, int32 Delta)
{
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeHistorySubsystem* Director = World->GetSubsystem<UNarrativeHistorySubsystem>())
		{
			Director->UpdateFactionPopulation(FactionID, Delta);
		}
	}
}

void ANarrativeCompanion::ReceiveNarrativeHint(const FNarrativeHint& Hint)
{
	if (UCognitionComponent* Cog = GetCognitionComp())
	{
		FString Prefix;
		switch (Hint.Phase)
		{
		case ENarrativeHintPhase::Before:
			Prefix = TEXT("[I have a feeling] ");
			break;
		case ENarrativeHintPhase::During:
			Prefix = TEXT("[I notice] ");
			break;
		case ENarrativeHintPhase::After:
			Prefix = TEXT("[Reflecting] ");
			break;
		}

		Cog->ProcessStimulus(Prefix + Hint.HintText, false, Hint.Priority);
	}
}

AActor* ANarrativeCompanion::GetLeader() const
{
	return UGameplayStatics::GetPlayerCharacter(this, 0);
}

