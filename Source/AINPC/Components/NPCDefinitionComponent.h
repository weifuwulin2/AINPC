#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UtilityAI/BackstoryConfig.h"
#include "NPCDefinitionComponent.generated.h"

class AAIController;

/**
 * Unified component for managing static NPC configuration IDs.
 * Acts as the "Profile" or "Passport" for the NPC, holding IDs for
 * Personality, Profession, Background, etc.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
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
	void ApplyPersonality(AAIController* Controller);
	void ApplyProfession(AAIController* Controller);

	/** Retrieves the full Backstory definition based on BackstoryID. */
	UFUNCTION(BlueprintCallable, Category = "NPC Profile")
	
	bool GetSocialProfileDef(FSocialProfileDef& OutDef) const;
	// Getters
	UFUNCTION(BlueprintCallable, Category = "NPC Profile | Modular")
	bool GetNameDef(FNPCNameDef& OutDef) const;
    
	UFUNCTION(BlueprintCallable, Category = "NPC Profile | Modular")
	bool GetPastEventDef(FPastEventDef& OutDef) const;

protected:

	// --- Backstory Support (Legacy) ---
	
	/** Backstory ID (e.g., "WarVeteran"). Maps to DT_Backstories. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile | Modular")
	FName SocialProfileID;

	/** Data Table for Backstories. Should be set to DT_Backstories. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile | Modular")
	UDataTable* SocialProfileTable;

	
	// --- Modular Identity (Phase 2+) ---

	/** Name ID (e.g. "Male01"). Maps to DT_Names. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile | Modular")
	FName NameID;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile | Modular")
	UDataTable* NameTable;

	/** Past Event ID (e.g. "BurningFields"). Maps to DT_PastEvents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile | Modular")
	FName PastEventID;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile | Modular")
	UDataTable* PastEventTable;


};