
#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "Action_Attack.generated.h"

class ACombatEnemy;

/** Internal phases of the combat loop driven by Action_Attack. */
UENUM(BlueprintType)
enum class ECombatPhase : uint8
{
	ChoosingAttack,   // About to launch the next attack
	Attacking,        // Waiting for attack animation callback (or timeout)
	Repositioning     // Moving to a strafe / fallback position
};

/**
 * Attack Action — self-contained combat loop.
 * Drives the cycle: attack → reposition (strafe/fallback) → attack → ...
 * No StateTree required; all sub-behaviour is managed here.
 */
UCLASS()
class AINPC_API UAction_Attack : public UUtilityActionBase
{
	GENERATED_BODY()

public:
	UAction_Attack();

	// -------------------------------------------------------
	// Combat Loop Configuration
	// -------------------------------------------------------

	/** Probability [0,1] of using charged attack instead of combo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat",
		meta = (ClampMin = 0, ClampMax = 1))
	float ChargedAttackChance = 0.35f;

	/** Minimum radius around target when picking a strafe destination. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat",
		meta = (ClampMin = 0, Units = "cm"))
	float StrafeRadiusMin = 200.0f;

	/** Maximum radius around target when picking a strafe destination. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat",
		meta = (ClampMin = 0, Units = "cm"))
	float StrafeRadiusMax = 350.0f;

	/** Half-angle of the arc used to randomise the strafe direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat",
		meta = (ClampMin = 0, ClampMax = 180, Units = "deg"))
	float StrafeArcHalfAngleDeg = 60.0f;

	/** Arrival acceptance radius for the repositioning move. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat",
		meta = (ClampMin = 0, Units = "cm"))
	float StrafeAcceptanceRadius = 50.0f;

	/** Max time allowed for repositioning before giving up and attacking again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat",
		meta = (ClampMin = 0, Units = "s"))
	float StrafeTimeout = 2.5f;

	/** Safety timeout: if the attack callback never fires, move on after this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat",
		meta = (ClampMin = 0, Units = "s"))
	float AttackTimeout = 5.0f;

	// -------------------------------------------------------
	// Lifecycle Overrides
	// -------------------------------------------------------
	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;

protected:
	UPROPERTY()
	AAIController* OwningController;

	// --- Runtime combat state ---
	ECombatPhase CombatPhase = ECombatPhase::ChoosingAttack;
	bool bAttackCallbackReceived = false;
	float PhaseStartTime = 0.0f;
	FVector RepositionTarget = FVector::ZeroVector;

	// --- Helpers ---
	void LaunchAttack(AAIController* Controller);
	void BeginRepositioning(AAIController* Controller);
	bool HasReachedReposition(AAIController* Controller) const;
	void ResetCombatState();

	/** Callback when TargetSelectionSubsystem reports the current target is invalid. */
	UFUNCTION()
	void OnTargetInvalidated(AAIController* Controller, AActor* OldTarget);
};
