#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCDefinitionComponent.generated.h"

class AAIController;

/**
 * Unified component for managing static NPC configuration IDs.
 * Acts as the "Profile" or "Passport" for the NPC, holding IDs for
 * Personality, Profession, Background, etc.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API UNPCDefinitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNPCDefinitionComponent();

	virtual void BeginPlay() override;

	// --- Configuration IDs ---

	/** Personality ID (e.g., "Zombie", "Merchant"). Maps to DT_Personalities. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile")
	FName PersonalityID;

	/** Profession ID (e.g., "Miner", "Guard"). Maps to DT_Professions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile")
	FName ProfessionID;

	// Future IDs can be added here (e.g., BackgroundID, VoiceID)

	// --- Logic ---

	/** Applies the definition to the linked Controller's components. */
	UFUNCTION(BlueprintCallable, Category = "NPC Profile")
	void ApplyDefinition(AAIController* Controller);

protected:
	// Helper to apply specific parts
	void ApplyPersonality(AAIController* Controller);
	void ApplyProfession(AAIController* Controller);
};
