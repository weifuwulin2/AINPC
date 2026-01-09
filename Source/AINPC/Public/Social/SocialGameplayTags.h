#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * Native Gameplay Tags for the AI NPC System.
 * These correspond to the "Social Taxonomy" defined in Phase 1.
 */
namespace AINPCTags
{
	// Activity
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Rest);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Work);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Eat);
	AINPC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity_Mundane);

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
