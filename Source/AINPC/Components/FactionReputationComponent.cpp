
#include "Components/FactionReputationComponent.h"

#include "AIController.h"
#include "Social/FactionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "AINPC.h"
#include "NPCDefinitionComponent.h"
#include "Utilities/FactionHelpers.h"

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
						AINPC_LOG(Log, "[FactionReputation] Auto-synced FactionID from Pawn's NPCDefinitionComponent: %s", *FactionID.ToString());
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

	UE_LOG(LogAINPC, Warning, TEXT("🔍 [CombatPolicy] Evaluating: %s → %s"), *Source->GetName(), *Target->GetName());

	// 1. Physical Incapacitation (Highest Priority - Never Combat)
	if (Source->ActorHasTag("Status.Dead") || Target->ActorHasTag("Status.Dead"))
	{
		UE_LOG(LogAINPC, Verbose, TEXT("   ❌ DENIED: Dead actor"));
		return false;
	}
	if (Source->ActorHasTag("Status.Unconscious") || Target->ActorHasTag("Status.Unconscious"))
	{
		UE_LOG(LogAINPC, Verbose, TEXT("   ❌ DENIED: Unconscious actor"));
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
			UE_LOG(LogAINPC, Warning, TEXT("   ✅ %s has Combat Tag: Event.Danger=%d, Directive=%d, GuardsHostile=%d, Legacy=%d"), 
				*Actor->GetName(), bHasCombat, bHasDirective, bHasGuardsHostile, bHasLegacy);
		}
		
		return bHasAny;
	};

	bool bSourceHasCombat = HasCombatTag(Source);
	bool bTargetHasCombat = HasCombatTag(Target);
	
	if (bSourceHasCombat || bTargetHasCombat)
	{
		UE_LOG(LogAINPC, Warning, TEXT("   ✅ ALLOWED: Combat Tag Override (Source=%d, Target=%d)"), bSourceHasCombat, bTargetHasCombat);
		return true;
	}

	// 2b. Monsters bypass scene safety (always hostile regardless of narrative state)
	if (Source->ActorHasTag("Combat.AlwaysHostile") || Target->ActorHasTag("Combat.AlwaysHostile"))
	{
		// Also check Pawn/Controller pair for the tag
		UE_LOG(LogAINPC, Warning, TEXT("   ✅ ALLOWED: Combat.AlwaysHostile tag (Monster bypass)"));
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
		UE_LOG(LogAINPC, Warning, TEXT("   ✅ ALLOWED: Combat.AlwaysHostile on Pawn/Controller pair (Monster bypass)"));
		return true;
	}

	// 3. Narrative Scene Safety (Medium Priority - Default Peace)
	bool bSourceInScene = Source->ActorHasTag("Status.InScene");
	bool bTargetInScene = Target->ActorHasTag("Status.InScene");

	if (bSourceInScene || bTargetInScene)
	{
		UE_LOG(LogAINPC, Warning, TEXT("   ❌ DENIED: Scene Safety (Source.InScene=%d, Target.InScene=%d)"), bSourceInScene, bTargetInScene);
		return false;
	}

	// 4. Default: Allow Combat (Let Faction logic decide hostility)
	UE_LOG(LogAINPC, Verbose, TEXT("   ✅ ALLOWED: Default (no restrictions)"));
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

	float Current = GetAttitudeTowards(Target);
	float NewVal = FMath::Clamp(Current + Delta, 0.0f, 100.0f);
	
	PersonalReputations.Add(Target, NewVal);
	
	AINPC_LOG(Log, "Reputation Modified for %s: %.1f -> %.1f", *Target->GetName(), Current, NewVal);
}

FName UFactionReputationComponent::GetFactionID(AActor* Actor)
{
	// DEPRECATED: Wrapper for backward compatibility
	// Use FactionHelpers::GetFactionID() instead
	return FactionHelpers::GetFactionID(Actor);
}
