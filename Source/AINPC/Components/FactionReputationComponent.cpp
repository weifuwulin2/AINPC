
#include "Components/FactionReputationComponent.h"

#include "AIController.h"
#include "Social/FactionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "AINPC.h"
#include "NPCDefinitionComponent.h"

UFactionReputationComponent::UFactionReputationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentFactionID = "Neutral";
}

void UFactionReputationComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// ✅ Auto-sync FactionID from Pawn's NPCDefinitionComponent if CurrentFactionID is default
	// 自动从 Pawn 的 NPCDefinitionComponent 同步 FactionID（如果当前是默认值）
	if (CurrentFactionID == "Neutral" || CurrentFactionID.IsNone())
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
						CurrentFactionID = PawnDefComp->FactionID;
						AINPC_LOG(Log, "[FactionReputation] Auto-synced FactionID from Pawn's NPCDefinitionComponent: %s", *CurrentFactionID.ToString());
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
			return Subsystem->GetBaseAttitude(CurrentFactionID, TargetFaction);
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
	if (!Actor) return "None";

	// 1. Narrative Override (Centralized Logic)
	// Check for InScene Tag on Actor or Pawn/Controller
	if (Actor->ActorHasTag("Status.InScene")) return "Neutral";
	
	if (APawn* P = Cast<APawn>(Actor)) 
	{ 
		if (P->GetController() && P->GetController()->ActorHasTag("Status.InScene")) return "Neutral"; 
	}
	
	if (AController* C = Cast<AController>(Actor)) 
	{ 
		if (C->GetPawn() && C->GetPawn()->ActorHasTag("Status.InScene")) return "Neutral"; 
	}

	// 2. Try finding this component
	if (UFactionReputationComponent* FacComp = Actor->FindComponentByClass<UFactionReputationComponent>())
	{
		return FacComp->CurrentFactionID;
	}

	// 2. Fallback: Parse from Tags (Legacy/Simple support)
	// Expect tag: "Faction.Examples"
	for (const FName& Tag : Actor->Tags)
	{
		FString TagStr = Tag.ToString();
		if (TagStr.StartsWith("Faction."))
		{
			return FName(*TagStr.Mid(8)); // Remove "Faction." prefix
		}
	}
	
	// 3. Fallback: Check NPCDefinitionComponent (NEW - Proper Fallback)
	// 回退：检查 NPCDefinitionComponent（新增 - 正确的回退逻辑）
	if (UNPCDefinitionComponent* DefComp = Actor->FindComponentByClass<UNPCDefinitionComponent>())
	{
		if (!DefComp->FactionID.IsNone() && DefComp->FactionID != "None")
		{
			return DefComp->FactionID;
		}
	}
	
	// 3.5. CRITICAL: If Actor is Controller, also check Pawn's NPCDefinitionComponent
	// 关键：如果 Actor 是 Controller，也检查 Pawn 的 NPCDefinitionComponent
	if (AAIController* AICon = Cast<AAIController>(Actor))
	{
		if (APawn* ControlledPawn = AICon->GetPawn())
		{
			if (UNPCDefinitionComponent* PawnDefComp = ControlledPawn->FindComponentByClass<UNPCDefinitionComponent>())
			{
				if (!PawnDefComp->FactionID.IsNone() && PawnDefComp->FactionID != "None")
				{
					return PawnDefComp->FactionID;
				}
			}
		}
	}
	
	// 4. Fallback: "Player" tag
	if (Actor->ActorHasTag("Player"))
	{
		return "Player"; // Special faction for player
	}

	// 5. Default
	return "Neutral";
}
