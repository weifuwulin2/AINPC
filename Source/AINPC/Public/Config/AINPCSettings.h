#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AINPCSettings.generated.h"

/**
 * AINPC System Settings - Editable in Project Settings → Plugins → AINPC
 * 
 * These settings control AI behavior tuning parameters that were previously
 * hardcoded as magic numbers throughout the codebase.
 */
UCLASS(config=Game, defaultconfig, meta=(DisplayName="AINPC Settings"))
class AINPC_API UAINPCSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAINPCSettings();

	// ==================== Utility AI Tuning ====================

	/** Additive bonus when action's IntentionTag matches LLM's current Intention.
	 *  Higher values give LLM more agency over action selection. */
	UPROPERTY(config, EditAnywhere, Category = "Utility AI", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float IntentionMatchBonus = 2.5f;

	/** Multiplier when action's DirectiveTag matches current GoalComponent Directive.
	 *  Applied to final score. Default 1.5x encourages matching. */
	UPROPERTY(config, EditAnywhere, Category = "Utility AI", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float DirectiveMatchMultiplier = 1.5f;

	/** Multiplier when action's DirectiveTag does NOT match current Directive.
	 *  Lower values discourage non-matching actions. */
	UPROPERTY(config, EditAnywhere, Category = "Utility AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirectiveMismatchMultiplier = 0.5f;

	/** Inertia bonus for currently executing action.
	 *  Prevents rapid action switching (flickering). */
	UPROPERTY(config, EditAnywhere, Category = "Utility AI", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float InertiaBonus = 0.5f;

	// ==================== Narrative Integration ====================

	/** Additional multiplier applied to DirectiveMatchMultiplier when NPC is in a Narrative Scene.
	 *  Total multiplier = DirectiveMatchMultiplier * NarrativeDirectiveBoost.
	 *  Higher values enforce stricter role adherence during scenes. */
	UPROPERTY(config, EditAnywhere, Category = "Utility AI|Narrative", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float NarrativeDirectiveBoost = 2.0f;

	// ==================== Needs Thresholds ====================

	/** Hunger/Fatigue threshold that triggers Survival Directive.
	 *  When Hunger OR Fatigue exceeds this, NPC prioritizes eating/sleeping. */
	UPROPERTY(config, EditAnywhere, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalHungerThreshold = 0.6f;

	/** Health threshold that triggers Survival Directive. */
	UPROPERTY(config, EditAnywhere, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalHealthThreshold = 0.3f;

	/** Loneliness threshold that triggers Social Directive.
	 *  NPC will seek social interaction when Loneliness exceeds this. */
	UPROPERTY(config, EditAnywhere, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SocialLonelinessThreshold = 0.7f;

	// ==================== Personality (PAM) ====================

	/** Minimum multiplier for Personality Action Modifier.
	 *  Prevents any action from being completely suppressed by personality. */
	UPROPERTY(config, EditAnywhere, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PAMModifierMin = 0.2f;

	/** Maximum multiplier for Personality Action Modifier.
	 *  Caps how much personality can boost an action. */
	UPROPERTY(config, EditAnywhere, Category = "Personality", meta = (ClampMin = "1.0", ClampMax = "5.0"))
	float PAMModifierMax = 2.0f;

	// ==================== Static Accessor ====================

	/** Get the AINPC Settings singleton (safe to call anytime). */
	UFUNCTION(BlueprintPure, Category = "AINPC", meta = (DisplayName = "Get AINPC Settings"))
	static const UAINPCSettings* Get();

	// UDeveloperSettings interface
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
