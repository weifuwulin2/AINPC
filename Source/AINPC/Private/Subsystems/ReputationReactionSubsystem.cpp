#include "Subsystems/ReputationReactionSubsystem.h"

#include "AINPC.h"
#include "Subsystems/EventBusSubsystem.h"
#include "Components/FactionReputationComponent.h"
#include "Social/SocialGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"

// ============================================================
// Initialize / Deinitialize
// ============================================================

void UReputationReactionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadRules();

	UEventBusSubsystem* EventBus = GetWorld()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	// Subscribe with empty filter = receive all events (we filter by tag in OnEventReceived)
	FGameplayTagContainer EmptyFilter;
	FOnEventReceived Callback;
	Callback.BindUFunction(this, FName("OnEventReceived"));
	SubscriptionId = EventBus->Subscribe(this, EmptyFilter, EEventPriority::Deferred, Callback);

	FACTION_REPUTATION_LOG(Log, "Initialized with %d rules", Rules.Num());
}

void UReputationReactionSubsystem::Deinitialize()
{
	if (UEventBusSubsystem* EventBus = GetWorld() ? GetWorld()->GetSubsystem<UEventBusSubsystem>() : nullptr)
	{
		EventBus->Unsubscribe(SubscriptionId);
	}
	Super::Deinitialize();
}

// ============================================================
// Rule Loading
// ============================================================

void UReputationReactionSubsystem::LoadRules()
{
	Rules.Empty();

	// Try DataTable override first
	if (!ReactionRuleTable.IsNull())
	{
		if (UDataTable* Table = ReactionRuleTable.LoadSynchronous())
		{
			TArray<FReputationReactionRule*> Rows;
			Table->GetAllRows<FReputationReactionRule>(TEXT("ReputationReactionSubsystem"), Rows);
			for (const FReputationReactionRule* Row : Rows)
			{
				if (Row && Row->EventTag.IsValid())
				{
					Rules.Add(Row->EventTag, *Row);
				}
			}
			FACTION_REPUTATION_LOG(Log, "Loaded %d rules from DataTable", Rules.Num());
			return;
		}
	}

	// Fall back to built-in defaults
	RegisterDefaultRules();
}

void UReputationReactionSubsystem::RegisterDefaultRules()
{
	auto Add = [this](FReputationReactionRule Rule)
	{
		Rules.Add(Rule.EventTag, Rule);
	};

	// --- Combat ---
	{
		FReputationReactionRule R;
		R.EventTag                    = AINPCTags::Event_Combat_Damage;
		R.TargetDeltaTowardInstigator = -20.0f; // Victim hates attacker
		R.InstigatorDeltaTowardTarget =  -5.0f; // Attacker grows contempt
		R.WitnessDeltaTowardInstigator = -5.0f; // Witnesses dislike the aggressor
		R.WitnessRadius               = 1500.0f;
		R.bScaleByMagnitude           = true;   // Scale by damage ratio
		Add(R);
	}
	{
		FReputationReactionRule R;
		R.EventTag                     = AINPCTags::Event_Death_Witnessed;
		R.WitnessDeltaTowardInstigator = -30.0f; // Witnesses strongly dislike the killer
		R.WitnessRadius                = 2500.0f;
		Add(R);
	}

	// --- Social: Negative ---
	{
		FReputationReactionRule R;
		R.EventTag                    = AINPCTags::Social_Conflict_Insult;
		R.TargetDeltaTowardInstigator = -15.0f;
		R.WitnessDeltaTowardInstigator = -5.0f;
		R.WitnessRadius               = 800.0f;
		R.bScaleByMagnitude           = true;
		Add(R);
	}
	{
		FReputationReactionRule R;
		R.EventTag                    = AINPCTags::Social_Interact_Threat;
		R.TargetDeltaTowardInstigator = -10.0f;
		R.WitnessDeltaTowardInstigator = -8.0f;
		R.WitnessRadius               = 1000.0f;
		R.bScaleByMagnitude           = true;
		Add(R);
	}

	// --- Social: Positive ---
	{
		FReputationReactionRule R;
		R.EventTag                    = AINPCTags::Social_Interact_Compliment;
		R.TargetDeltaTowardInstigator = +15.0f;
		R.WitnessDeltaTowardInstigator = +3.0f;
		R.WitnessRadius               = 800.0f;
		R.bScaleByMagnitude           = true;
		Add(R);
	}
	{
		FReputationReactionRule R;
		R.EventTag                    = AINPCTags::Social_Interact_Gift;
		R.TargetDeltaTowardInstigator = +20.0f;
		R.WitnessDeltaTowardInstigator = +5.0f;
		R.WitnessRadius               = 1000.0f;
		R.bScaleByMagnitude           = true;
		Add(R);
	}
	{
		FReputationReactionRule R;
		R.EventTag                    = AINPCTags::Social_Interact_Beg;
		R.TargetDeltaTowardInstigator = +5.0f;   // Slight sympathy from the target
		R.WitnessDeltaTowardInstigator = -3.0f;  // Witnesses lose a bit of respect
		R.WitnessRadius               = 800.0f;
		Add(R);
	}

	FACTION_REPUTATION_LOG(Log, "Registered %d built-in default rules", Rules.Num());
}

// ============================================================
// Event Handling
// ============================================================

void UReputationReactionSubsystem::OnEventReceived(const FEventMetadata& EventData)
{
	const FSemanticEvent& Event = EventData.Event;

	// Find matching rule by exact tag or parent tag
	const FReputationReactionRule* MatchedRule = nullptr;
	for (const auto& Pair : Rules)
	{
		if (Event.Verb.MatchesTagExact(Pair.Key) || Event.Verb.MatchesTag(Pair.Key))
		{
			MatchedRule = &Pair.Value;
			break;
		}
	}

	if (!MatchedRule) return;

	ApplyReactionRule(Event, *MatchedRule);
}

void UReputationReactionSubsystem::ApplyReactionRule(const FSemanticEvent& Event, const FReputationReactionRule& Rule)
{
	const float Scale = Rule.bScaleByMagnitude ? FMath::Max(Event.Magnitude, 0.01f) : 1.0f;

	AActor* Instigator = Event.Instigator.Get();
	AActor* Target     = Event.Target.Get();

	// Helper to find FactionReputationComponent on Actor (checks Pawn and its Controller)
	auto FindFactionComp = [](AActor* Actor) -> UFactionReputationComponent*
	{
		if (!IsValid(Actor)) return nullptr;
		if (UFactionReputationComponent* C = Actor->FindComponentByClass<UFactionReputationComponent>())
			return C;
		// Also check the pawn's controller
		if (const APawn* P = Cast<APawn>(Actor))
			if (AController* Ctrl = P->GetController())
				return Ctrl->FindComponentByClass<UFactionReputationComponent>();
		return nullptr;
	};

	// --- Target → Instigator ---
	if (IsValid(Target) && IsValid(Instigator) && Rule.TargetDeltaTowardInstigator != 0.0f)
	{
		if (UFactionReputationComponent* TargetComp = FindFactionComp(Target))
		{
			TargetComp->ModifyReputation(Instigator, Rule.TargetDeltaTowardInstigator * Scale);
			FACTION_REPUTATION_LOG(Log, "%s → %s attitude toward %s: %+.1f",
				*Event.Verb.ToString(),
				*Target->GetName(),
				*Instigator->GetName(),
				Rule.TargetDeltaTowardInstigator * Scale);
		}
	}

	// --- Instigator → Target ---
	if (IsValid(Instigator) && IsValid(Target) && Rule.InstigatorDeltaTowardTarget != 0.0f)
	{
		if (UFactionReputationComponent* InstigatorComp = FindFactionComp(Instigator))
		{
			InstigatorComp->ModifyReputation(Target, Rule.InstigatorDeltaTowardTarget * Scale);
		}
	}

	// --- Witnesses ---
	if (Rule.WitnessDeltaTowardInstigator != 0.0f && IsValid(Instigator))
	{
		ApplyWitnessReaction(Event, Rule.WitnessDeltaTowardInstigator * Scale, Rule.WitnessRadius);
	}
}

void UReputationReactionSubsystem::ApplyWitnessReaction(const FSemanticEvent& Event, float WitnessDelta, float Radius)
{
	UWorld* World = GetWorld();
	if (!World) return;

	AActor* Instigator = Event.Instigator.Get();
	AActor* Target     = Event.Target.Get();
	if (!IsValid(Instigator)) return;

	const FVector Origin = Event.Location.IsNearlyZero()
		? Instigator->GetActorLocation()
		: Event.Location;

	// Sphere overlap to find nearby Pawns
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ReputationWitnessOverlap), false);
	Params.AddIgnoredActor(Instigator);
	if (IsValid(Target)) Params.AddIgnoredActor(Target);

	World->OverlapMultiByChannel(
		Overlaps,
		Origin,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params
	);

	int32 WitnessCount = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* WitnessActor = Overlap.GetActor();
		if (!IsValid(WitnessActor)) continue;

		// Only apply to NPCs that have FactionReputationComponent
		UFactionReputationComponent* WitnessComp = WitnessActor->FindComponentByClass<UFactionReputationComponent>();
		if (!WitnessComp)
		{
			if (const APawn* P = Cast<APawn>(WitnessActor))
				if (AController* Ctrl = P->GetController())
					WitnessComp = Ctrl->FindComponentByClass<UFactionReputationComponent>();
		}
		if (!WitnessComp) continue;

		WitnessComp->ModifyReputation(Instigator, WitnessDelta);
		WitnessCount++;
	}

	if (WitnessCount > 0)
	{
		FACTION_REPUTATION_LOG(Log, "%s witnessed by %d NPCs → attitude toward %s: %+.1f",
			*Event.Verb.ToString(), WitnessCount, *Instigator->GetName(), WitnessDelta);
	}
}
