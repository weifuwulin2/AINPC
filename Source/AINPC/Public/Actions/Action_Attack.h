
#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "Action_Attack.generated.h"

class UAnimMontage;

/**
 * Concrete Attack Action.
 * Engages in combat with hostile targets.
 * Uses the standard Enter/Execute/Exit lifecycle from UUtilityActionBase.
 */
UCLASS()
class AINPC_API UAction_Attack : public UUtilityActionBase
{
	GENERATED_BODY()

public:
	UAction_Attack();

	// --- Configuration ---
	
	/** Range to start attacking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackRange = 150.0f;

	/** Damage to deal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DamageAmount = 10.0f;

	/** Animation to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* AttackMontage;

	// --- Lifecycle Overrides ---
	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;

protected:
	// Runtime state
	// NOTE: TargetActor removed - use Controller->GetFocusActor() as single source of truth
	
	UPROPERTY()
	AAIController* OwningController;

	bool bIsAttacking;
	bool bHasDealtDamage;

	void PerformAttack(AAIController* Controller);
	
	UFUNCTION()
	void OnAttackAnimFinished(UAnimMontage* Montage, bool bInterrupted);

	/** Callback when TargetSelectionSubsystem detects target is invalid */
	UFUNCTION()
	void OnTargetInvalidated(AAIController* Controller, AActor* OldTarget);
};