#pragma once

#include "CoreMinimal.h"

// Forward declarations
class UCognitionComponent;
class UGoalComponent;
class UPersonalityComponent;
class UUtilityAIComponent;
class UNPCDefinitionComponent;

/**
 * Utility functions for AINPC actor lookups.
 * 
 * These helpers handle the Controller/Pawn duality common in AI setups:
 * - Components may live on Controller (CognitionComponent, PersonalityComponent)
 * - Components may live on Pawn (GoalComponent, NPCDefinitionComponent)
 * - Some components exist on both and need prioritization
 * 
 * Usage:
 *   #include "Utilities/AINPCHelpers.h"
 *   if (UCognitionComponent* Cognition = AINPCHelpers::GetCognitionComponent(SomeActor))
 *   {
 *       Cognition->ProcessStimulus(...);
 *   }
 */
namespace AINPCHelpers
{
	/**
	 * Find CognitionComponent from any Actor in the AI hierarchy.
	 * Search order: Actor → Actor's Controller → Controller's Pawn
	 * @param Actor The Pawn, Controller, or any AI-related Actor
	 * @return CognitionComponent if found, nullptr otherwise
	 */
	AINPC_API UCognitionComponent* GetCognitionComponent(AActor* Actor);

	/**
	 * Find GoalComponent from any Actor in the AI hierarchy.
	 * Search order: Actor → Actor's Controller → Controller's Pawn
	 * @param Actor The Pawn, Controller, or any AI-related Actor
	 * @return GoalComponent if found, nullptr otherwise
	 */
	AINPC_API UGoalComponent* GetGoalComponent(AActor* Actor);

	/**
	 * Find PersonalityComponent from any Actor in the AI hierarchy.
	 * Typically lives on UtilityAIController.
	 * @param Actor The Pawn, Controller, or any AI-related Actor
	 * @return PersonalityComponent if found, nullptr otherwise
	 */
	AINPC_API UPersonalityComponent* GetPersonalityComponent(AActor* Actor);

	/**
	 * Find UtilityAIComponent from any Actor in the AI hierarchy.
	 * Typically lives on UtilityAIController.
	 * @param Actor The Pawn, Controller, or any AI-related Actor
	 * @return UtilityAIComponent if found, nullptr otherwise
	 */
	AINPC_API UUtilityAIComponent* GetUtilityAIComponent(AActor* Actor);

	/**
	 * Find NPCDefinitionComponent from any Actor in the AI hierarchy.
	 * Typically lives on the Pawn/Character.
	 * @param Actor The Pawn, Controller, or any AI-related Actor
	 * @return NPCDefinitionComponent if found, nullptr otherwise
	 */
	AINPC_API UNPCDefinitionComponent* GetNPCDefinitionComponent(AActor* Actor);
}
