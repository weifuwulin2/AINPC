
#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "Action_Attack.generated.h"

class UStateTree;
class UStateTreeAIComponent;

/**
 * Concrete Attack Action.
 * Thin orchestrator: starts/stops the combat StateTree and monitors target validity.
 * All combat behavior (movement, attacks, strafe) is handled by the StateTree.
 */
UCLASS()
class AINPC_API UAction_Attack : public UUtilityActionBase
{
	GENERATED_BODY()

public:
	UAction_Attack();

	// --- Combat StateTree Configuration ---

	/** If true, this action starts a combat StateTree while active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|StateTree")
	bool bUseCombatStateTree = false;

	/** StateTree asset to run when this action enters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|StateTree", meta = (EditCondition = "bUseCombatStateTree", EditConditionHides))
	TObjectPtr<UStateTree> CombatStateTreeAsset;

	/** Stops the combat StateTree when the action exits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|StateTree", meta = (EditCondition = "bUseCombatStateTree", EditConditionHides))
	bool bStopCombatStateTreeOnExit = true;

	/** Allows replacing an already-running StateTree on the same controller. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|StateTree", meta = (EditCondition = "bUseCombatStateTree", EditConditionHides))
	bool bAllowReplacingRunningStateTree = false;

	// --- Lifecycle Overrides ---
	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;

protected:
	UPROPERTY()
	AAIController* OwningController;

	TWeakObjectPtr<UStateTreeAIComponent> ActiveStateTreeComponent;
	bool bCombatStateTreeActivated = false;

	void StartCombatStateTree(AAIController* Controller);
	void StopCombatStateTree();

	/** Callback when TargetSelectionSubsystem detects target is invalid */
	UFUNCTION()
	void OnTargetInvalidated(AAIController* Controller, AActor* OldTarget);
};
