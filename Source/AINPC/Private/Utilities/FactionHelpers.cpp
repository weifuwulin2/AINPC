#include "Utilities/FactionHelpers.h"
#include "Components/FactionReputationComponent.h"
#include "Components/NPCDefinitionComponent.h"
#include "Social/FactionSubsystem.h"
#include "AIController.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace FactionHelpers
{
	bool IsAttitudeHostile(float Attitude, float Threshold)
	{
		return Attitude < Threshold;
	}

	FName GetFactionID(AActor* Actor)
	{
		if (!Actor) return "None";

		// 1. Narrative Override (Centralized Logic)
		// Check for InScene Tag on Actor or Pawn/Controller
		// ✅ EXCEPTION: If Directive.Combat is present, DO NOT force Neutral.
		// This allows plot-driven combat (e.g. Uprisings) to use real Factions.
		bool bHasCombatDirective = Actor->ActorHasTag("Directive.Combat");

		if (!bHasCombatDirective)
		{
			if (Actor->ActorHasTag("Status.InScene")) return "Neutral";

			if (APawn* P = Cast<APawn>(Actor))
			{
				if (P->GetController() && P->GetController()->ActorHasTag("Status.InScene")) return "Neutral";
			}

			if (AController* C = Cast<AController>(Actor))
			{
				if (C->GetPawn() && C->GetPawn()->ActorHasTag("Status.InScene")) return "Neutral";
			}
		}

		// 2. Try finding FactionReputationComponent
		if (UFactionReputationComponent* FacComp = Actor->FindComponentByClass<UFactionReputationComponent>())
		{
			return FacComp->FactionID;
		}

		// 3. Fallback: Parse from Tags (Legacy/Simple support)
		// Expect tag: "Faction.Examples"
		for (const FName& Tag : Actor->Tags)
		{
			FString TagStr = Tag.ToString();
			if (TagStr.StartsWith("Faction."))
			{
				return FName(*TagStr.Mid(8)); // Remove "Faction." prefix
			}
		}

		// 4. Fallback: Check NPCDefinitionComponent
		if (UNPCDefinitionComponent* DefComp = Actor->FindComponentByClass<UNPCDefinitionComponent>())
		{
			if (!DefComp->FactionID.IsNone() && DefComp->FactionID != "None")
			{
				return DefComp->FactionID;
			}
		}

		// 5. CRITICAL: If Actor is Controller, also check Pawn's NPCDefinitionComponent
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

		// 6. Fallback: "Player" tag
		if (Actor->ActorHasTag("Player"))
		{
			return "Player"; // Special faction for player
		}

		// 7. Default
		return "Neutral";
	}

	bool AreActorsHostile(AActor* ActorA, AActor* ActorB)
	{
		if (!ActorA || !ActorB) return false;
		if (ActorA == ActorB) return false;

		// 1. Personal Reputation Check (Highest Priority)
		// Check Local Components for Override (e.g. "I personally hate YOU")
		if (UFactionReputationComponent* FacCompA = ActorA->FindComponentByClass<UFactionReputationComponent>())
		{
			return FacCompA->IsHostile(ActorB);
		}
		// Check fallback Controller/Pawn for A
		if (APawn* PawnA = Cast<APawn>(ActorA))
		{
			if (AController* ConA = PawnA->GetController())
			{
				if (UFactionReputationComponent* FacCompCon = ConA->FindComponentByClass<UFactionReputationComponent>())
				{
					return FacCompCon->IsHostile(ActorB);
				}
			}
		}

		// 2. Global Faction Relation Check (Data-Driven)
		// Even if components are missing (e.g. Player), we can derive FactionID and check the Matrix.
		UWorld* World = ActorA ? ActorA->GetWorld() : nullptr;

		if (World)
		{
			if (UFactionSubsystem* Subsystem = World->GetSubsystem<UFactionSubsystem>())
			{
				FName FactionA = GetFactionID(ActorA);
				FName FactionB = GetFactionID(ActorB);

				// This returns true ONLY if the Faction Matrix explicitly says < 25.0
				// If not defined, it defaults to Neutral (50), which is NOT hostile.
				return Subsystem->AreFactionsHostile(FactionA, FactionB);
			}
		}

		// If FactionSubsystem is not available, default to non-hostile
		// This is a safe fallback - better to have false negatives than false positives
		return false;
	}
}
