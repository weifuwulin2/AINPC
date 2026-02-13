#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Stateless NPC discovery utilities for the WorldDirector.
 * Follows the same pattern as AINPCHelpers and FactionHelpers.
 */
namespace NPCQueryHelpers
{
	/**
	 * Get all living NPCs in the world (have CognitionComponent, not dead).
	 * @param World - The world context
	 * @return Array of valid NPC pawn actors
	 */
	AINPC_API TArray<AActor*> GetAllLivingNPCs(UWorld* World);

	/**
	 * Filter NPCs by faction ID.
	 * @param NPCs - Input NPC array
	 * @param FactionID - Faction to match (e.g. "Orc", "Human")
	 * @return Filtered array of NPCs belonging to the faction
	 */
	AINPC_API TArray<AActor*> FilterByFaction(const TArray<AActor*>& NPCs, FName FactionID);

	/**
	 * Filter NPCs by profession ID.
	 * @param NPCs - Input NPC array
	 * @param ProfessionID - Profession to match (e.g. "Merchant", "Guard")
	 * @return Filtered array of NPCs with the profession
	 */
	AINPC_API TArray<AActor*> FilterByProfession(const TArray<AActor*>& NPCs, FName ProfessionID);

	/**
	 * Filter out NPCs that are currently in a narrative scene (Status.InScene).
	 * @param NPCs - Input NPC array
	 * @return Filtered array of available (non-scene) NPCs
	 */
	AINPC_API TArray<AActor*> FilterAvailable(const TArray<AActor*>& NPCs);

	/**
	 * Build a brief description string for an NPC: "Name (Faction, Profession)"
	 * @param NPC - The actor to describe
	 * @return Brief string, or empty if not a valid NPC
	 */
	AINPC_API FString BuildNPCBrief(AActor* NPC);

	/**
	 * Find the best matching NPC from candidates given a natural-language description.
	 * Performs keyword matching against faction + profession strings.
	 * @param Candidates - Available NPCs to search
	 * @param Description - LLM-generated description (e.g. "an orc merchant")
	 * @return Best matching actor, or nullptr if no match
	 */
	AINPC_API AActor* FindNPCByDescription(const TArray<AActor*>& Candidates, const FString& Description);
}
