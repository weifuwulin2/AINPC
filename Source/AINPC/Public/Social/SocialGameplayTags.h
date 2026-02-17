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
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Social_Relationship);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Social_Faction);

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
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Directive_Combat);    // Fight, Attack
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Directive_Idle);      // Wander, Stand

	// Timeline Event Triggers (Generic)
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Scene_Trigger);    // Generic trigger for timeline (Payload defines specifics)
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_State_Hostile);    // Something became hostile (Payload: Role/Faction)
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_Damage);    // Damage taken (Payload: Victim Role)
    AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PlayerDetected);   // Keep this as it's a very common global event
    // Removed: Event_GuardsHostile, Event_PlayerStillNear, Event_BanditAttacked

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

	// WorldDirector Tags
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldDirector_Event);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldDirector_Escalate);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldDirector_Disrupt);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldDirector_Converge);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldDirector_Reveal);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldDirector_Relieve);

	// WorldEvent Tags (Major world events)
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldEvent_BeastTide);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldEvent_DemonInvasion);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldEvent_FactionWar);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldEvent_NaturalDisaster);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldEvent_Festival);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldEvent_Plague);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorldEvent_Custom);

	// Location Tags (Semantic anchor classification)
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Location_Tavern);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Location_TownSquare);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Location_Market);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Location_ForestEdge);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Location_Mine);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Location_Temple);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Location_Barracks);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Location_Generic);
}
