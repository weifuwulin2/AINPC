#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UtilityAI/BackstoryConfig.h"
#include "NPCDefinitionComponent.generated.h"

class AAIController;

/**
 * Unified NPC Definition Row for DataTables.
 * Allows creating "Templates" like "Guard_Orc", "Civilian_Human".
 */
USTRUCT(BlueprintType)
struct FNPCDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile")
	FName PersonalityID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile")
	FName ProfessionID;

	// Faction Identity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile")
	FName FactionID;

	// The Actor Class to spawn for this template (e.g. BP_Orc_Warrior)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile")
	TSubclassOf<APawn> PawnClass;
};

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

	// --- Template System (Easy Setup) ---
	
	/** 
	 * Preset template ID (e.g., "Orc_Warrior", "Human_Civilian").
	 * If set, auto-loads PersonalityID, ProfessionID, FactionID from DT_NPCDefinitions.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile | Template")
	FName DefinitionTemplateID;

	/** Data Table for NPC Templates. Should be set to DT_NPCDefinitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile | Template")
	UDataTable* DefinitionTable;

	// --- Configuration IDs (Auto-populated from Template or set manually) ---

	/** Personality ID (e.g., "Zombie", "Merchant"). Maps to DT_Personalities. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile")
	FName PersonalityID;

	/** Profession ID (e.g., "Miner", "Guard"). Maps to DT_Professions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile")
	FName ProfessionID;

	/** Faction ID. Maps to DT_Factions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile")
	FName FactionID;

	/** Data Table for Factions. Should be set to DT_Factions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Profile")
	UDataTable* FactionTable;

	// --- Logic ---
	
	/** Loads configuration from DefinitionTemplateID. Called automatically in BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "NPC Profile")
	void LoadFromTemplate();

	/** 
	 * Randomizes Name, Backstory, and Past Event from their respective DataTables.
	 * Called automatically if these IDs are not set.
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Profile")
	void RandomizeModularIdentity();

	/** Applies the definition to the linked Controller's components. */
	UFUNCTION(BlueprintCallable, Category = "NPC Profile")
	void ApplyDefinition(AAIController* Controller);
	void ApplyPersonality(AAIController* Controller);
	void ApplyProfession(AAIController* Controller);
	void ApplyFaction(AActor* Owner);

	/** Retrieves the full Backstory definition based on BackstoryID. */
	UFUNCTION(BlueprintCallable, Category = "NPC Profile")
	
	bool GetSocialProfileDef(FSocialProfileDef& OutDef) const;
	UFUNCTION(BlueprintCallable, Category = "NPC Profile")
	FString GetDisplayName() const;

	// Getters
	UFUNCTION(BlueprintCallable, Category = "NPC Profile | Modular")
	bool GetNameDef(FNPCNameDef& OutDef) const;

	UFUNCTION(BlueprintCallable, Category = "NPC Profile | Modular")
	FName GetNameID() const { return NameID; }
    
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
