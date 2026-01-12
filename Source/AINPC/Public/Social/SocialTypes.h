#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SocialTypes.generated.h"

class AActor;

/**
 * Faction Types for AI relationships.
 * Defines "Who is on whose side".
 * Jobs (Guard/Merchant) should be defined by Tags or Roles, not Faction.
 */

UENUM(BlueprintType)
enum class EFactionType : uint8
{
	Neutral     UMETA(DisplayName = "Neutral"),
	Human       UMETA(DisplayName = "Human"),
	Monster     UMETA(DisplayName = "Monster")
};

/**
 * The standard data package flowing through the AI NPC system.
 * Decouples "what happened" from "how to react".
 */
USTRUCT(BlueprintType)
struct AINPC_API FSemanticEvent
{
	GENERATED_BODY()


	// Who performed the action
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social")
	TObjectPtr<AActor> Instigator = nullptr;

	// What was done (Taxonomy Tag, e.g., Social.Conflict.Insult)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social")
	FGameplayTag Verb;

	// Who was the target
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social")
	TObjectPtr<AActor> Target = nullptr;

	// Natural language description (e.g., "Soldier A insulted Recruit B.")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social")
	FString Content;

	// Intensity of the event (0.0 - 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social")
	float Magnitude = 0.0f;
};

/**
 * Data configured on assets (Smart Objects) to describe their social meaning.
 */
USTRUCT(BlueprintType)
struct AINPC_API FSmartObjectSemantics
{
	GENERATED_BODY()

	// Template for generating description (e.g., "{Instigator} sits on {Target}.")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social")
	FString DescriptionTemplate;

	// The social meaning tag (e.g., Activity.Rest)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social")
	FGameplayTag SocialTag;

	// Base intensity of this interaction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social")
	float Intensity = 0.0f;
};

/**
 * A single entry in the NPC's memory stream.
 * Can be an observation, a thought, or a reflection.
 */
USTRUCT(BlueprintType)
struct AINPC_API FMemoryItem
{
	GENERATED_BODY()

	// Unique ID for retrieval/reference
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Memory")
	FGuid MemoryId;

	// When this happened (Game Time or Real Time)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Memory")
	FDateTime Timestamp;

	// The natural language description
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Memory")
	FString Description;

	// Associated tags (for quick filtering)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Memory")
	FGameplayTagContainer Tags;

	// Calculated importance at time of recording (0.0 - 10.0 usually)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Memory")
	float ImportanceScore = 0.0f;

	// Embedding vector for semantic search (reserved for future use)
	// TArray<float> Embedding; 

	FMemoryItem()
	{
		MemoryId = FGuid::NewGuid();
		Timestamp = FDateTime::Now();
	}
};
