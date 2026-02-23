
#include "Components/FactionReputationComponent.h"

#include "AIController.h"
#include "Social/FactionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "AINPC.h"
#include "NPCDefinitionComponent.h"
#include "Social/SocialGameplayTags.h"
#include "Subsystems/NarrativeHistorySubsystem.h"
#include "Utilities/FactionHelpers.h"

namespace
{
	float BondTypeToAttitude(ESocialBondType BondType)
	{
		switch (BondType)
		{
		case ESocialBondType::BestFriend:   return 95.0f;
		case ESocialBondType::CloseFriend:  return 85.0f;
		case ESocialBondType::Friend:       return 70.0f;
		case ESocialBondType::Acquaintance: return 55.0f;
		case ESocialBondType::Lover:        return 92.0f;
		case ESocialBondType::Family:       return 90.0f;
		case ESocialBondType::Rival:        return 35.0f;
		case ESocialBondType::Enemy:        return 15.0f;
		case ESocialBondType::Nemesis:      return 5.0f;
		default:                            return 50.0f;
		}
	}

	ESocialBondType AttitudeToBondType(float Attitude)
	{
		if (Attitude >= 95.0f) return ESocialBondType::BestFriend;
		if (Attitude >= 85.0f) return ESocialBondType::CloseFriend;
		if (Attitude >= 70.0f) return ESocialBondType::Friend;
		if (Attitude >= 55.0f) return ESocialBondType::Acquaintance;
		if (Attitude >= 35.0f) return ESocialBondType::Rival;
		if (Attitude >= 15.0f) return ESocialBondType::Enemy;
		return ESocialBondType::Nemesis;
	}

	const TCHAR* BondTypeToText(ESocialBondType BondType)
	{
		switch (BondType)
		{
		case ESocialBondType::Acquaintance: return TEXT("Acquaintance");
		case ESocialBondType::Friend:       return TEXT("Friend");
		case ESocialBondType::CloseFriend:  return TEXT("Close Friend");
		case ESocialBondType::BestFriend:   return TEXT("Best Friend");
		case ESocialBondType::Lover:        return TEXT("Lover");
		case ESocialBondType::Family:       return TEXT("Family");
		case ESocialBondType::Rival:        return TEXT("Rival");
		case ESocialBondType::Enemy:        return TEXT("Enemy");
		case ESocialBondType::Nemesis:      return TEXT("Nemesis");
		default:                            return TEXT("None");
		}
	}
}

UFactionReputationComponent::UFactionReputationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	FactionID = "Neutral";
}

void UFactionReputationComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// ✅ Auto-sync FactionID from Pawn's NPCDefinitionComponent if FactionID is default
	// 自动从 Pawn 的 NPCDefinitionComponent 同步 FactionID（如果当前是默认值）
	if (FactionID == "Neutral" || FactionID.IsNone())
	{
		// Try to get FactionID from Pawn's NPCDefinitionComponent
		if (AAIController* AICon = Cast<AAIController>(GetOwner()))
		{
			if (APawn* ControlledPawn = AICon->GetPawn())
			{
				if (UNPCDefinitionComponent* PawnDefComp = ControlledPawn->FindComponentByClass<UNPCDefinitionComponent>())
				{
					if (!PawnDefComp->FactionID.IsNone() && PawnDefComp->FactionID != "None")
					{
						FactionID = PawnDefComp->FactionID;
						FACTION_REPUTATION_LOG(Log, "Auto-synced FactionID from Pawn's NPCDefinitionComponent: %s", *FactionID.ToString());
					}
				}
			}
		}
	}
}

// Helper to determine if combat logic should supersede safety checks
bool UFactionReputationComponent::EvaluateCombatPolicy(const AActor* Source, const AActor* Target) const
{
	if (!Source || !Target) return false;

	FACTION_REPUTATION_LOG(Warning, "🔍 [CombatPolicy] Evaluating: %s → %s", *Source->GetName(), *Target->GetName());

	// 1. Physical Incapacitation (Highest Priority - Never Combat)
	if (Source->ActorHasTag("Status.Dead") || Target->ActorHasTag("Status.Dead"))
	{
		FACTION_REPUTATION_LOG(Verbose, "   ❌ DENIED: Dead actor");
		return false;
	}
	if (Source->ActorHasTag("Status.Unconscious") || Target->ActorHasTag("Status.Unconscious"))
	{
		FACTION_REPUTATION_LOG(Verbose, "   ❌ DENIED: Unconscious actor");
		return false;
	}

	// 2. Explicit Combat Triggers (High Priority - Override Scene Safety)
	auto HasCombatTag = [&](const AActor* Actor) -> bool
	{
		bool bHasCombat = Actor->ActorHasTag("Event.Danger.Combat");
		bool bHasDirective = Actor->ActorHasTag("Directive.Combat");
		bool bHasGuardsHostile = Actor->ActorHasTag("Event.GuardsHostile");
		bool bHasLegacy = Actor->ActorHasTag("Combat.Allowed");
		
		bool bHasAny = bHasCombat || bHasDirective || bHasGuardsHostile || bHasLegacy;
		
		if (bHasAny)
		{
			FACTION_REPUTATION_LOG(Warning, "   ✅ %s has Combat Tag: Event.Danger=%d, Directive=%d, GuardsHostile=%d, Legacy=%d", 
				*Actor->GetName(), bHasCombat, bHasDirective, bHasGuardsHostile, bHasLegacy);
		}
		
		return bHasAny;
	};

	bool bSourceHasCombat = HasCombatTag(Source);
	bool bTargetHasCombat = HasCombatTag(Target);
	
	if (bSourceHasCombat || bTargetHasCombat)
	{
		FACTION_REPUTATION_LOG(Warning, "   ✅ ALLOWED: Combat Tag Override (Source=%d, Target=%d)", bSourceHasCombat, bTargetHasCombat);
		return true;
	}

	// 2b. Monsters bypass scene safety (always hostile regardless of narrative state)
	if (Source->ActorHasTag("Combat.AlwaysHostile") || Target->ActorHasTag("Combat.AlwaysHostile"))
	{
		// Also check Pawn/Controller pair for the tag
		FACTION_REPUTATION_LOG(Warning, "   ✅ ALLOWED: Combat.AlwaysHostile tag (Monster bypass)");
		return true;
	}
	// Check Pawn/Controller pair for AlwaysHostile tag
	auto HasAlwaysHostileTag = [](const AActor* Actor) -> bool
	{
		if (const APawn* P = Cast<APawn>(Actor))
		{
			if (P->GetController() && P->GetController()->ActorHasTag("Combat.AlwaysHostile")) return true;
		}
		if (const AController* C = Cast<AController>(Actor))
		{
			if (C->GetPawn() && C->GetPawn()->ActorHasTag("Combat.AlwaysHostile")) return true;
		}
		return false;
	};
	if (HasAlwaysHostileTag(Source) || HasAlwaysHostileTag(Target))
	{
		FACTION_REPUTATION_LOG(Warning, "   ✅ ALLOWED: Combat.AlwaysHostile on Pawn/Controller pair (Monster bypass)");
		return true;
	}

	// 3. Narrative Scene Safety (Medium Priority - Default Peace)
	bool bSourceInScene = Source->ActorHasTag("Status.InScene");
	bool bTargetInScene = Target->ActorHasTag("Status.InScene");

	if (bSourceInScene || bTargetInScene)
	{
		FACTION_REPUTATION_LOG(Warning, "   ❌ DENIED: Scene Safety (Source.InScene=%d, Target.InScene=%d)", bSourceInScene, bTargetInScene);
		return false;
	}

	// 4. Default: Allow Combat (Let Faction logic decide hostility)
	FACTION_REPUTATION_LOG(Verbose, "   ✅ ALLOWED: Default (no restrictions)");
	return true;
}

float UFactionReputationComponent::GetAttitudeTowards(AActor* Target) const
{
	if (!Target) return 50.0f;

	// ✅ SEMANTIC POLICY CHECK
	// Use centralized policy to determine if hostile interactions are permitted
	if (!EvaluateCombatPolicy(GetOwner(), Target))
	{
		return 50.0f; // Force Neutral/Peaceful
	}

	if (Target == GetOwner()) return 100.0f;

	// 1. Personal Override
	if (const float* PersonalVal = PersonalReputations.Find(Target))
	{
		return *PersonalVal;
	}

	// 1b. Semantic social bond fallback
	const FName StableTargetID = GetStableSocialID(Target);
	if (!StableTargetID.IsNone())
	{
		if (const FSocialBond* Bond = SocialBonds.Find(StableTargetID))
		{
			return BondTypeToAttitude(Bond->Type);
		}
	}

	// 1c. Fixed relationship seed (lazy cache to SocialBonds)
	float SeedAttitude = 50.0f;
	FSocialBond SeedBond;
	if (TryResolveSeedRelationship(Target, SeedAttitude, SeedBond))
	{
		if (!StableTargetID.IsNone())
		{
			UFactionReputationComponent* MutableThis = const_cast<UFactionReputationComponent*>(this);
			if (MutableThis && !MutableThis->SocialBonds.Contains(StableTargetID))
			{
				MutableThis->SocialBonds.Add(StableTargetID, SeedBond);
			}
		}
		return SeedAttitude;
	}

	// 2. Global Faction Baseline
	UWorld* World = GetWorld();
	if (World)
	{
		if (UFactionSubsystem* Subsystem = World->GetSubsystem<UFactionSubsystem>())
		{
			FName TargetFaction = GetFactionID(Target);
			return Subsystem->GetBaseAttitude(FactionID, TargetFaction);
		}
	}

	return 50.0f; // Default Neutral
}

FName UFactionReputationComponent::GetStableSocialID(AActor* Target) const
{
	if (!Target)
	{
		return NAME_None;
	}

	auto ResolveFromActor = [](AActor* Actor) -> FName
	{
		if (!Actor)
		{
			return NAME_None;
		}

		if (const UNPCDefinitionComponent* DefComp = Actor->FindComponentByClass<UNPCDefinitionComponent>())
		{
			const FName NameID = DefComp->GetNameID();
			if (!NameID.IsNone() && NameID != "None")
			{
				return NameID;
			}
		}

		return NAME_None;
	};

	FName StableID = ResolveFromActor(Target);
	if (!StableID.IsNone())
	{
		return StableID;
	}

	if (AAIController* AsController = Cast<AAIController>(Target))
	{
		StableID = ResolveFromActor(AsController->GetPawn());
		if (!StableID.IsNone())
		{
			return StableID;
		}
	}

	if (APawn* AsPawn = Cast<APawn>(Target))
	{
		StableID = ResolveFromActor(AsPawn->GetController());
		if (!StableID.IsNone())
		{
			return StableID;
		}
	}

	return Target->GetFName();
}

bool UFactionReputationComponent::TryResolveSeedRelationship(AActor* Target, float& OutAttitude, FSocialBond& OutBond) const
{
	OutAttitude = 50.0f;
	OutBond = FSocialBond();

	if (!Target)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UFactionSubsystem* FactionSubsystem = World->GetSubsystem<UFactionSubsystem>();
	if (!FactionSubsystem)
	{
		return false;
	}

	FName SourceID = GetStableSocialID(GetOwner());
	FName TargetID = GetStableSocialID(Target);
	if (SourceID.IsNone() || TargetID.IsNone())
	{
		return false;
	}

	ESocialBondType SeedBondType = ESocialBondType::None;
	FString SeedSummary;
	int32 SeedSalience = 1;
	float SeedAttitude = 50.0f;

	if (!FactionSubsystem->TryGetSeedRelationship(
		SourceID,
		TargetID,
		SeedAttitude,
		SeedBondType,
		SeedSummary,
		SeedSalience))
	{
		return false;
	}

	OutAttitude = FMath::Clamp(SeedAttitude, 0.0f, 100.0f);
	OutBond.Type = SeedBondType;
	OutBond.BondSalience = FMath::Clamp(SeedSalience, 0, 10);
	OutBond.RelationshipSummary = SeedSummary;
	OutBond.LastInteractionTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return true;
}

void UFactionReputationComponent::EnsureInitialRelationshipWith(AActor* Target, float SuggestedAttitude, const FString& SuggestedSummary)
{
	if (!Target || Target == GetOwner())
	{
		return;
	}

	const FName StableTargetID = GetStableSocialID(Target);
	if (StableTargetID.IsNone() || SocialBonds.Contains(StableTargetID))
	{
		return;
	}

	float SeedAttitude = 50.0f;
	FSocialBond SeedBond;
	if (TryResolveSeedRelationship(Target, SeedAttitude, SeedBond))
	{
		SocialBonds.Add(StableTargetID, SeedBond);
		return;
	}

	const float ClampedAttitude = FMath::Clamp(SuggestedAttitude, 0.0f, 100.0f);
	FSocialBond InitialBond;
	InitialBond.Type = AttitudeToBondType(ClampedAttitude);
	InitialBond.BondSalience = 1;
	InitialBond.RelationshipSummary = SuggestedSummary.IsEmpty()
		? FString::Printf(TEXT("My initial impression of %s is %s (%.0f/100)."),
			*Target->GetName(),
			*FactionHelpers::GetAttitudeDescription(ClampedAttitude),
			ClampedAttitude)
		: SuggestedSummary;
	InitialBond.LastInteractionTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	SocialBonds.Add(StableTargetID, InitialBond);
}

FString UFactionReputationComponent::GetRelationshipSummaryTowards(AActor* Target) const
{
	if (!Target)
	{
		return TEXT("No target.");
	}

	const FName StableID = GetStableSocialID(Target);
	if (!StableID.IsNone())
	{
		if (const FSocialBond* Bond = SocialBonds.Find(StableID))
		{
			if (!Bond->RelationshipSummary.IsEmpty())
			{
				return Bond->RelationshipSummary;
			}

			return UEnum::GetValueAsString(Bond->Type);
		}
	}

	float SeedAttitude = 50.0f;
	FSocialBond SeedBond;
	if (TryResolveSeedRelationship(Target, SeedAttitude, SeedBond))
	{
		if (!SeedBond.RelationshipSummary.IsEmpty())
		{
			return SeedBond.RelationshipSummary;
		}
		return UEnum::GetValueAsString(SeedBond.Type);
	}

	float Attitude = 50.0f;
	if (const float* PersonalVal = PersonalReputations.Find(Target))
	{
		Attitude = *PersonalVal;
	}
	else if (UWorld* World = GetWorld())
	{
		if (UFactionSubsystem* Subsystem = World->GetSubsystem<UFactionSubsystem>())
		{
			Attitude = Subsystem->GetBaseAttitude(FactionID, GetFactionID(Target));
		}
	}

	return FString::Printf(TEXT("%s (%s)"), *FactionHelpers::GetAttitudeDescription(Attitude), *Target->GetName());
}

float UFactionReputationComponent::GetReputationWith(FName TargetFactionID) const
{
	// Query global faction relations from FactionSubsystem
	UWorld* World = GetWorld();
	if (World)
	{
		if (UFactionSubsystem* Subsystem = World->GetSubsystem<UFactionSubsystem>())
		{
			return Subsystem->GetBaseAttitude(FactionID, TargetFactionID);
		}
	}

	return 50.0f; // Default Neutral
}

bool UFactionReputationComponent::IsHostile(AActor* Target) const
{
	float Attitude = GetAttitudeTowards(Target);
	// TODO: Make threshold configurable or read from FactionDef?
	// For now, hardcode generic threshold
	return Attitude < 25.0f;
}

void UFactionReputationComponent::ModifyReputation(AActor* Target, float Delta)
{
	if (!Target) return;

	const FName TargetID = GetStableSocialID(Target);
	const FName FinalTargetID = TargetID.IsNone() ? Target->GetFName() : TargetID;

	// Persistent relationship attitude should NOT be affected by temporary combat policy overrides.
	float Current = 50.0f;
	if (const float* PersonalVal = PersonalReputations.Find(Target))
	{
		Current = *PersonalVal;
	}
	else if (!TargetID.IsNone())
	{
		if (const FSocialBond* ExistingBond = SocialBonds.Find(TargetID))
		{
			Current = BondTypeToAttitude(ExistingBond->Type);
		}
		else
		{
			float SeedAttitude = 50.0f;
			FSocialBond SeedBond;
			if (TryResolveSeedRelationship(Target, SeedAttitude, SeedBond))
			{
				Current = SeedAttitude;
			}
			else if (UWorld* World = GetWorld())
			{
				if (UFactionSubsystem* Subsystem = World->GetSubsystem<UFactionSubsystem>())
				{
					Current = Subsystem->GetBaseAttitude(FactionID, GetFactionID(Target));
				}
			}
		}
	}
	else
	{
		float SeedAttitude = 50.0f;
		FSocialBond SeedBond;
		if (TryResolveSeedRelationship(Target, SeedAttitude, SeedBond))
		{
			Current = SeedAttitude;
		}
		else if (UWorld* World = GetWorld())
		{
			if (UFactionSubsystem* Subsystem = World->GetSubsystem<UFactionSubsystem>())
			{
				Current = Subsystem->GetBaseAttitude(FactionID, GetFactionID(Target));
			}
		}
	}

	float NewVal = FMath::Clamp(Current + Delta, 0.0f, 100.0f);
	if (FMath::IsNearlyEqual(Current, NewVal))
	{
		return;
	}

	const ESocialBondType OldBondType = AttitudeToBondType(Current);
	const ESocialBondType NewBondType = AttitudeToBondType(NewVal);
	const bool bCrossedBondThreshold = (OldBondType != NewBondType);
	
	PersonalReputations.Add(Target, NewVal);

	FName SourceID = GetStableSocialID(GetOwner());
	if (SourceID.IsNone() && GetOwner())
	{
		SourceID = GetOwner()->GetFName();
	}

	if (!TargetID.IsNone() && !SocialBonds.Contains(TargetID))
	{
		float InitSeedAttitude = 50.0f;
		FSocialBond InitSeedBond;
		if (TryResolveSeedRelationship(Target, InitSeedAttitude, InitSeedBond))
		{
			SocialBonds.Add(TargetID, InitSeedBond);
		}
	}

	if (!TargetID.IsNone())
	{
		FSocialBond& Bond = SocialBonds.FindOrAdd(TargetID);
		Bond.Type = NewBondType;
		Bond.BondSalience = FMath::Clamp(Bond.BondSalience + (bCrossedBondThreshold ? 2 : 1), 0, 10);
		Bond.LastInteractionTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

		if (bCrossedBondThreshold)
		{
			Bond.RelationshipSummary = FString::Printf(
				TEXT("After recent interactions, %s shifted from %s to %s in my eyes."),
				*Target->GetName(),
				BondTypeToText(OldBondType),
				BondTypeToText(NewBondType));
		}
		else if (Bond.RelationshipSummary.IsEmpty())
		{
			Bond.RelationshipSummary = FString::Printf(
				TEXT("My relationship with %s is %s (%.0f/100)."),
				*Target->GetName(),
				*FactionHelpers::GetAttitudeDescription(NewVal),
				NewVal
			);
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (UNarrativeHistorySubsystem* History = World->GetSubsystem<UNarrativeHistorySubsystem>())
		{
			History->RecordRelationshipChange(SourceID, FinalTargetID, Current, NewVal);

			// Lightweight reflection: only emit semantic reflection log when bond tier changes.
			if (bCrossedBondThreshold)
			{
				FGameplayTagContainer Tags;
				Tags.AddTag(AINPCTags::Social_Relationship);
				History->RecordEvent(
					FString::Printf(
						TEXT("[Reflection] %s now sees %s as %s (was %s)."),
						GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"),
						*Target->GetName(),
						BondTypeToText(NewBondType),
						BondTypeToText(OldBondType)),
					Tags);
			}
		}
	}

	OnRelationshipChanged.Broadcast(GetOwner(), Target, SourceID, FinalTargetID, Current, NewVal, bCrossedBondThreshold);
	
	FACTION_REPUTATION_LOG(Log, "Reputation Modified for %s: %.1f -> %.1f", *Target->GetName(), Current, NewVal);
}

FName UFactionReputationComponent::GetFactionID(AActor* Actor)
{
	// DEPRECATED: Wrapper for backward compatibility
	// Use FactionHelpers::GetFactionID() instead
	return FactionHelpers::GetFactionID(Actor);
}
