#include "Social/SocialGameplayTags.h"

namespace AINPCTags
{
	// Interaction - Core Categories for Emotion Matrix
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Rest, "Interaction.Rest", "Resting: Sleep, Sit, Idle (Sad/Tired)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Work, "Interaction.Work", "Working: Patrol, Craft, Guard (Neutral)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Eat, "Interaction.Eat", "Eating: Eat, Drink (Hungry)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Mundane, "Interaction.Mundane", "Mundane background activity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Combat, "Interaction.Combat", "Combat: Attack, Shoot, Chase (Angry)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Flee, "Interaction.Flee", "Flee: RunAway, Hide, Beg (Scared)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Social, "Interaction.Social", "Social: Chat, Greet, Trade (Happy)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Investigate, "Interaction.Investigate", "Investigate: LookAt, Approach (Curious)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Avoid, "Interaction.Avoid", "Avoid: Reject, WalkAway (Disgust)");

	// Social
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Social_Chat, "Social.Chat", "Generic chatting");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Social_Greet, "Social.Greet", "Greeting interaction");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Social_Conflict, "Social.Conflict", "Generic conflict");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Social_Conflict_Insult, "Social.Conflict.Insult", "Insult event");

	// Event
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Danger, "Event.Danger", "General danger event");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Danger_Combat, "Event.Danger.Combat", "Combat danger triggering fight/flight");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Global, "Event.Global", "Global world event");

	// Perception
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perception_Vision, "Perception.Vision", "Visual perception of actor");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perception_Hearing, "Perception.Hearing", "Auditory perception");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perception_Damage, "Perception.Damage", "Damage taken event");

	// Life Events
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Death, "Event.Death", "Death event (generic)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Death_Self, "Event.Death.Self", "Death event (Self)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Death_Witnessed, "Event.Death.Witnessed", "Death event (Witnessed)");
	// Goal Directives
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Directive_Work, "Directive.Work", "Goal: Go to work");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Directive_Rest, "Directive.Rest", "Goal: Rest or Sleep");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Directive_Survival, "Directive.Survival", "Goal: Survive (Eat, Heal, Flee)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Directive_Social, "Directive.Social", "Goal: Socialize");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Directive_Combat, "Directive.Combat", "Goal: Fight / Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Directive_Idle, "Directive.Idle", "Goal: Idle / Wander");

	// Timeline Event Triggers
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Scene_Trigger, "Event.Scene.Trigger", "Generic Timeline Trigger");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_State_Hostile, "Event.State.Hostile", "Hostility State Change");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_Damage, "Event.Combat.Damage", "Damage Taken Event");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_PlayerDetected, "Event.PlayerDetected", "Timeline Trigger: Player enters perception range");
    // Generic abstraction replaces specific scenarions (GuardsHostile, etc.)

	// Work Activities
	// Defining them under Interaction.Work hierarchy so broad filters work
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Work_Mine, "Interaction.Work.Mine", "Work: Mining");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Work_Farm, "Interaction.Work.Farm", "Work: Farming");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Work_Guard, "Interaction.Work.Guard", "Work: Guarding");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Work_Scavenge, "Interaction.Work.Scavenge", "Work: Scavenging");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Work_Pray, "Interaction.Work.Pray", "Work: Praying");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Work_Study, "Interaction.Work.Study", "Work: Studying");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Work_Trade, "Interaction.Work.Trade", "Work: Trading (Merchant)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Work_Command, "Interaction.Work.Command", "Work: Command");

	// Intention Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Intention_Attack, "Intention.Attack", "LLM Intention: Attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Intention_Flee, "Intention.Flee", "LLM Intention: Flee");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Intention_Idle, "Intention.Idle", "LLM Intention: Idle");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Intention_Talk, "Intention.Talk", "LLM Intention: Talk");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Intention_Investigate, "Intention.Investigate", "LLM Intention: Investigate");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Intention_Beg, "Intention.Beg", "LLM Intention: Beg");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Intention_Work, "Intention.Work", "LLM Intention: Work");

	// Trauma
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Trauma_Fire,           "Trauma.Fire", "Trauma: Fire");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Trauma_LoudNoises,     "Trauma.LoudNoises", "Trauma: Loud Noises");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Trauma_Humiliation,    "Trauma.Humiliation", "Trauma: Humiliation");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Trauma_Poverty,        "Trauma.Poverty", "Trauma: Poverty");

	// Phobias
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phobia_Fire,           "Phobia.Fire", "Phobia: Fire");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phobia_Magic,          "Phobia.Magic", "Phobia: Magic");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phobia_LoudNoises,     "Phobia.LoudNoises", "Phobia: Loud Noises");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phobia_Crowds,         "Phobia.Crowds", "Phobia: Crowds");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phobia_Disease,        "Phobia.Disease", "Phobia: Disease");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phobia_Touch,          "Phobia.Touch", "Phobia: Touch");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phobia_Betrayal,       "Phobia.Betrayal", "Phobia: Betrayal");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phobia_Confinement,    "Phobia.Confinement", "Phobia: Confinement");
	// Status Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Dead, "Status.Dead", "Status: Dead");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Unconscious, "Status.Unconscious", "Status: Unconscious");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_InScene, "Status.InScene", "Status: Participating in a Narrative Scene");
}
