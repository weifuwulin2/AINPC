#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProfessionTypes.generated.h"

/**
 * A specific task in a daily schedule.
 */
USTRUCT(BlueprintType)
struct FScheduleTask
{
	GENERATED_BODY()

	// Start time (0.0 - 24.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartTime = 9.0f;

	// The semantic directive tag (e.g., "Directive.Work", "Directive.Rest")
	// This tag will be passed to GoalComponent to filter/drive Utility AI.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag TaskTag;

	// Optional: Specific activity tag to find smart objects (e.g., "Interaction.Work.Mine")
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ActivityTag;

	bool IsValid() const { return TaskTag.IsValid(); }
};

/**
 * Configuration for a specific profession (Job).
 */
USTRUCT(BlueprintType)
struct FProfessionConfig : public FTableRowBase
{
	GENERATED_BODY()

	// Display Name
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ProfessionName;

    // Job Description (LLM Context)
    // E.g. "You are a merchant looking for customers."
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

	// Daily Schedule
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FScheduleTask> Schedule;

	// Icon or other metadata can go here
};
