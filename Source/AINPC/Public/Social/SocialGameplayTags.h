#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * Native Gameplay Tags for the AI NPC System.
 * These correspond to the "Social Taxonomy" defined in Phase 1.
 */
namespace AINPCTags
{
	// Interaction - Physical/Behavioral Actions for Smart Objects & Emotion Matrix
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Rest);      // Sad/Tired -> Sleep, Sit, Idle ("Interaction.Rest")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Work);      // Neutral -> Patrol, Craft, Guard ("Interaction.Work")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Eat);       // Hungry -> Eat, Drink ("Interaction.Eat")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Mundane);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Combat);    // Angry -> Attack, Shoot, Chase ("Interaction.Combat")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Flee);      // Scared -> RunAway, Hide, Beg ("Interaction.Flee")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Social);    // Happy -> Chat, Greet, Trade ("Interaction.Social")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Investigate); // Curious -> LookAt, Approach ("Interaction.Investigate")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Avoid);     // Disgust -> Reject, WalkAway ("Interaction.Avoid")

	// Social
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Social_Chat);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Social_Greet);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Social_Conflict);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Social_Conflict_Insult);

	// Event
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Danger);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Danger_Combat);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Global);

	// Perception (Sensory Input)
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perception_Vision);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perception_Hearing);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perception_Damage);

	// Life Events
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death_Self);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death_Witnessed);

	// Goal Directives (High-Level Motivations)
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Directive_Work);      // Go to work
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Directive_Rest);      // Relax, Sleep
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Directive_Survival);  // Eat, Heal, Flee
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Directive_Social);    // Talk, Trade
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Directive_Idle);      // Wander, Stand

	// Specific Work Interactions (Sub-types of Interaction.Work)
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Work_Mine);           // Mining ("Interaction.Work.Mine") - Kept as Work_Mine per request
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Work_Farm);           // Farming ("Interaction.Work.Farm")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Work_Guard);          // Guarding ("Interaction.Work.Guard")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Work_Scavenge);       // Scavenging ("Interaction.Work.Scavenge")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Work_Pray);           // Praying ("Interaction.Work.Pray")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Work_Study);          // Studying ("Interaction.Work.Study")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Work_Trade);          // Trading ("Interaction.Work.Trade")
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Work_Command);        // Commanding ("Interaction.Work.Command")

	// Intention Tags (LLM Output Mapping)
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Intention_Attack);      // "Attack"
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Intention_Flee);        // "Flee"
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Intention_Idle);        // "Idle"
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Intention_Talk);        // "Talk"
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Intention_Investigate); // "Investigate"
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Intention_Beg);         // "Beg"
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Intention_Work);        // "Work"
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Intention_Work_Command); // "Command"

	// Trauma & Phobias (Soul Layer)
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trauma_Fire);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trauma_LoudNoises);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trauma_Humiliation);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trauma_Poverty);

	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phobia_Fire);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phobia_Magic);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phobia_LoudNoises);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phobia_Crowds);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phobia_Disease);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phobia_Touch);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phobia_Betrayal);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phobia_Confinement);

	// Status Tags
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Dead);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Unconscious);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_InScene);
}
