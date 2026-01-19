// BackstoryConfig.h
// Definition of NPC Backstory data structure
// 
// Phase 1: The Soul Layer
// Defines past history, traumas, and social standing for LLM context injection.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "BackstoryConfig.generated.h"

/**
 * Social Status Enum
 * Defines the NPC's standing in society (affects speech tone)
 */
UENUM(BlueprintType)
enum class EBackstorySocialStatus : uint8
{
    Low           UMETA(DisplayName = "Low / Outcast"),
    Middle        UMETA(DisplayName = "Middle / Commoner"),
    High          UMETA(DisplayName = "High / Noble"),
    Unknown       UMETA(DisplayName = "Unknown")
};

/**
 * NPC Name Definition
 * Simple table for random or specific names.
 */
USTRUCT(BlueprintType)
struct FNPCNameDef : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FString FirstName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FString Surname;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FString Gender; // "Male", "Female", "Neutral"
};

/**
 * Past Event Definition (Trauma/History)
 * Specific events that shaped the NPC's psyche.
 */
USTRUCT(BlueprintType)
struct FPastEventDef : public FTableRowBase
{
    GENERATED_BODY()

    // Description of the event (e.g., "Survived the Battle of Burning Fields")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    FString EventDescription;

    // Phobias resulting from this event (e.g., "Fear.Fire", "Fear.LoudNoises")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History", meta = (Categories = "Phobia"))
    TArray<FGameplayTag> ResultingPhobias;

    // Mental Scar / Lasting Impact (e.g., "Freezes when smelling smoke")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    FString MentalScar;
    
    // Impact on OCEAN (Optional modifier, e.g. Neuroticism +0.2)
    // could be added later
};

/**
 * Social Profile Definition Structure (formerly BackstoryDef)
 * Row struct for DT_SocialProfiles. Defines Social Class, Values, and Motivation.
 */
USTRUCT(BlueprintType)
struct FSocialProfileDef : public FTableRowBase
{
    GENERATED_BODY()

    // Social standing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Profile")
    EBackstorySocialStatus SocialStatus = EBackstorySocialStatus::Middle;

    // Key values/beliefs (e.g., "Honor", "Survival", "Greed")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Profile")
    TArray<FString> KeyValues;

    // Core Drives
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Profile")
    FString Motivation;
};
