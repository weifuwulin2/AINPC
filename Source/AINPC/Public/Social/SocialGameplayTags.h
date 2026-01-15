#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * Native Gameplay Tags for the AI NPC System.
 * These correspond to the "Social Taxonomy" defined in Phase 1.
 */
namespace AINPCTags
{
	// Activity - Core Categories for Emotion Matrix
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Rest);      // Sad/Tired -> Sleep, Sit, Idle
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Work);      // Neutral -> Patrol, Craft, Guard
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Eat);       // Hungry -> Eat, Drink
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Mundane);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Combat);    // Angry -> Attack, Shoot, Chase
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Flee);      // Scared -> RunAway, Hide, Beg
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Social);    // Happy -> Chat, Greet, Trade
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Investigate); // Curious -> LookAt, Approach
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Avoid);     // Disgust -> Reject, WalkAway

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
}
