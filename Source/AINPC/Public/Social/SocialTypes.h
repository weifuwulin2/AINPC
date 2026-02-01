#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Events/EventTypes.h"
#include "SocialTypes.generated.h"

class AActor;

/**
 * OCEAN Personality Traits (Big Five)
 * Used for personality data-driven logic
 */
UENUM(BlueprintType)
enum class EOCEANTrait : uint8
{
    Openness            UMETA(DisplayName = "Openness"),
    Conscientiousness   UMETA(DisplayName = "Conscientiousness"),
    Extraversion        UMETA(DisplayName = "Extraversion"),
    Agreeableness       UMETA(DisplayName = "Agreeableness"),
    Neuroticism         UMETA(DisplayName = "Neuroticism")
};

// ============================================================================
// REMOVED: EFactionType (Legacy 3-value enum)
// ============================================================================
// Faction system now uses flexible FName-based FactionIDs via FactionReputationComponent.
// Use FactionReputationComponent::GetFactionID() instead.
//
// Migration:
// - Old: if (GetFaction(Actor) == EFactionType::Monster)
// - New: if (GetFactionID(Actor) == "Monster" || GetFactionID(Actor) == "Orcs")
//
// Removed: 2026-02-01
// ============================================================================

// FSemanticEvent moved to Events/EventTypes.h for centralized management
// FSemanticEvent 已移至 Events/EventTypes.h 进行集中管理

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

	// The social meaning tag (e.g., Interaction.Rest)
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
	// Note: Initialized in constructor since FGuid::NewGuid() is not a compile-time constant
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Memory", meta = (IgnoreForMemberInitializationTest))
	FGuid MemoryId;

	// When this happened (Game Time or Real Time)
	// Note: Initialized in constructor since FDateTime::Now() is not a compile-time constant
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Memory", meta = (IgnoreForMemberInitializationTest))
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

	// ✅ Phase 4 P1: Resolution state - marks event as "handled"
	// 解决状态标记 - 表示该事件已被处理
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Memory")
	bool bIsResolved = false;
	
	// ✅ Phase 4 P1: Game time for precise decay calculation
	// 游戏时间戳（秒）- 用于精确衰减计算
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Memory")
	float GameTimeSeconds = 0.0f;

	// Embedding vector for semantic search (reserved for future use)
	// TArray<float> Embedding; 

	// Constructor: Initialize runtime-dependent fields
	FMemoryItem()
		: MemoryId(FGuid::NewGuid())
		, Timestamp(FDateTime::Now())
	{
	}
};
