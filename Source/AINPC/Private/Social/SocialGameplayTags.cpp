#include "Social/SocialGameplayTags.h"

namespace AINPCTags
{
	// Activity
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity_Rest, "Activity.Rest", "Resting behavior");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity_Work, "Activity.Work", "Working behavior");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity_Eat, "Activity.Eat", "Eating behavior");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity_Mundane, "Activity.Mundane", "Mundane background activity");

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
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Death_Self, "Event.Death.Self", "Self death event");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Death_Witnessed, "Event.Death.Witnessed", "Witnessed another's death");
}
