#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Events/EventTypes.h"
#include "ReputationReactionSubsystem.generated.h"

/**
 * One rule: "when event X fires, modify attitude by Y."
 * Can be used as a DataTable row (FTableRowBase) for designer-friendly tuning.
 *
 * TargetDeltaTowardInstigator  – applied to the TARGET actor's FactionReputationComponent
 *                                toward the INSTIGATOR (e.g. "I was insulted" → -15 attitude)
 * InstigatorDeltaTowardTarget  – applied to the INSTIGATOR's component toward the TARGET
 *                                (optional, e.g. insulter grows a little contempt)
 * WitnessDeltaTowardInstigator – applied to NPCs within WitnessRadius toward the INSTIGATOR
 * bScaleByMagnitude            – multiply all deltas by FSemanticEvent.Magnitude [0,1]
 */
USTRUCT(BlueprintType)
struct AINPC_API FReputationReactionRule : public FTableRowBase
{
	GENERATED_BODY()

	/** Event GameplayTag that triggers this rule (e.g. Social.Conflict.Insult) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Reaction")
	FGameplayTag EventTag;

	/** Attitude delta for TARGET's component toward INSTIGATOR. Negative = more hostile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Reaction")
	float TargetDeltaTowardInstigator = 0.0f;

	/** Attitude delta for INSTIGATOR's component toward TARGET. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Reaction")
	float InstigatorDeltaTowardTarget = 0.0f;

	/** Attitude delta applied to nearby witnesses' components toward the INSTIGATOR. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Reaction")
	float WitnessDeltaTowardInstigator = 0.0f;

	/** Sphere radius (cm) used to find witnesses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Reaction",
	          meta = (ClampMin = 0, Units = "cm"))
	float WitnessRadius = 1500.0f;

	/** Multiply all deltas by FSemanticEvent.Magnitude [0,1]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Reaction")
	bool bScaleByMagnitude = false;
};

/**
 * Listens to EventBus and adjusts NPC attitudes (via FactionReputationComponent::ModifyReputation)
 * whenever social or combat events occur.
 *
 * Rules can be tuned via DataTable (ReactionRuleTable) or fall back to built-in defaults.
 * Built-in defaults cover the most common cases:
 *   Combat.Damage          → victim hates attacker more
 *   Event.Death.Witnessed  → witnesses hate killer
 *   Social.Conflict.Insult → target and witnesses dislike insulter
 *   Social.Interact.*      → compliment/threat/gift/beg effects
 */
UCLASS(Config=Game)
class AINPC_API UReputationReactionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Optional DataTable override (rows must be FReputationReactionRule).
	 * If set, built-in defaults are replaced by the table contents.
	 * Configure in DefaultGame.ini:
	 *   [/Script/AINPC.ReputationReactionSubsystem]
	 *   ReactionRuleTable=/Game/Data/DT_ReputationReactions.DT_ReputationReactions
	 */
	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> ReactionRuleTable;

	/** Called by EventBus. Must be UFUNCTION for dynamic delegate binding. */
	UFUNCTION()
	void OnEventReceived(const FEventMetadata& EventData);

private:
	/** Loaded rules keyed by EventTag. */
	TMap<FGameplayTag, FReputationReactionRule> Rules;

	FGuid SubscriptionId;

	void LoadRules();
	void RegisterDefaultRules();

	void ApplyReactionRule(const FSemanticEvent& Event, const FReputationReactionRule& Rule);
	void ApplyWitnessReaction(const FSemanticEvent& Event, float WitnessDelta, float Radius);
};
