#include "Actions/Action_Attack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "AINPC.h"

#include "CombatEnemy.h"
#include "Subsystems/TargetSelectionSubsystem.h"
#include "Navigation/PathFollowingComponent.h"

UAction_Attack::UAction_Attack()
{
	ActionName = "Attack";
	OwningController = nullptr;

	bNeedsTarget = true;
	TargetContext = ETargetSelectionContext::Combat;

	// Only attack enemies (reputation < 45), not neutrals (50)
	TargetConfigOverride.FriendlyReputationThreshold = 45.0f;
}

// ============================================================
// Enter
// ============================================================

void UAction_Attack::Enter_Implementation(AAIController* Controller)
{
	Super::Enter_Implementation(Controller); // Sets ActionStartTime + CommitmentStartTime

	if (!Controller)
	{
		AINPC_LOG_ERROR("Action_Attack: Controller is null!");
		return;
	}

	OwningController = Controller;
	ResetCombatState();

	// --- Target selection ---
	AActor* SelectedTarget = nullptr;

	if (UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
	{
		FTargetSelectionConfig Config = TargetConfigOverride;
		SelectedTarget = TargetSystem->SelectTarget(Controller, TargetContext, Config);

		TargetSystem->OnTargetInvalidated.RemoveDynamic(this, &UAction_Attack::OnTargetInvalidated);
		TargetSystem->OnTargetInvalidated.AddDynamic(this, &UAction_Attack::OnTargetInvalidated);
	}

	if (!SelectedTarget)
	{
		SelectedTarget = Controller->GetFocusActor();
	}

	if (!SelectedTarget)
	{
		AINPC_LOG_WARNING("Action_Attack: No target found on Enter — action will idle");
		return;
	}

	Controller->SetFocus(SelectedTarget);
	AINPC_LOG(Log, "Action_Attack: Entered, target=%s", *SelectedTarget->GetName());
}

// ============================================================
// Execute  (ticked every 0.2 s by UtilityAIComponent)
// ============================================================

void UAction_Attack::Execute_Implementation(AAIController* Controller)
{
	Super::Execute_Implementation(Controller);

	if (!Controller) return;

	// --- Keep target fresh ---
	AActor* Target = Controller->GetFocusActor();
	if (!Target)
	{
		if (UTargetSelectionSubsystem* TS = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
		{
			Target = TS->SelectTarget(Controller, TargetContext, TargetConfigOverride);
			if (Target) Controller->SetFocus(Target);
		}
		if (!Target) return;
	}

	if (!IsValid(Target) || Target->ActorHasTag(FName("Dead")) || Target->ActorHasTag(FName("Status.Dead")))
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	const float Now = Controller->GetWorld()->GetTimeSeconds();

	// --- State machine ---
	switch (CombatPhase)
	{
	case ECombatPhase::ChoosingAttack:
		LaunchAttack(Controller);
		CombatPhase = ECombatPhase::Attacking;
		PhaseStartTime = Now;
		break;

	case ECombatPhase::Attacking:
	{
		const bool bTimedOut = (AttackTimeout > 0.0f) && (Now - PhaseStartTime >= AttackTimeout);
		if (bAttackCallbackReceived || bTimedOut)
		{
			if (bTimedOut && !bAttackCallbackReceived)
			{
				AINPC_LOG_WARNING("Action_Attack: Attack timed out — moving to reposition");
			}
			bAttackCallbackReceived = false;
			BeginRepositioning(Controller);
			CombatPhase = ECombatPhase::Repositioning;
			PhaseStartTime = Now;
		}
		break;
	}

	case ECombatPhase::Repositioning:
	{
		const bool bArrived  = HasReachedReposition(Controller);
		const bool bTimedOut = (StrafeTimeout > 0.0f) && (Now - PhaseStartTime >= StrafeTimeout);
		if (bArrived || bTimedOut)
		{
			Controller->StopMovement();
			CombatPhase = ECombatPhase::ChoosingAttack;
		}
		break;
	}
	}
}

// ============================================================
// Exit
// ============================================================

void UAction_Attack::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);

	// Clean up movement and focus
	if (Controller)
	{
		Controller->StopMovement();
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}

	// Unbind attack callback if still bound
	if (ACombatEnemy* CombatChar = Cast<ACombatEnemy>(Controller ? Controller->GetPawn() : nullptr))
	{
		CombatChar->OnAttackCompleted.Unbind();
	}

	// Unbind target invalidation
	if (UTargetSelectionSubsystem* TS = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
	{
		TS->OnTargetInvalidated.RemoveDynamic(this, &UAction_Attack::OnTargetInvalidated);
	}

	ResetCombatState();
	OwningController = nullptr;

	AINPC_LOG(Log, "Action_Attack: Exited");
}

// ============================================================
// LaunchAttack
// ============================================================

void UAction_Attack::LaunchAttack(AAIController* Controller)
{
	ACombatEnemy* CombatChar = Controller ? Cast<ACombatEnemy>(Controller->GetPawn()) : nullptr;
	if (!IsValid(CombatChar))
	{
		// Pawn is not a CombatEnemy — skip attack, move straight to reposition
		AINPC_LOG_WARNING("Action_Attack: Pawn is not ACombatEnemy, skipping attack");
		bAttackCallbackReceived = true;
		return;
	}

	// Unbind any stale callback first
	CombatChar->OnAttackCompleted.Unbind();

	CombatChar->OnAttackCompleted.BindLambda(
		[WeakThis = TWeakObjectPtr<UAction_Attack>(this)]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->bAttackCallbackReceived = true;
				AINPC_LOG(Log, "Action_Attack: OnAttackCompleted received");
			}
		}
	);

	const bool bCharged = FMath::FRand() < FMath::Clamp(ChargedAttackChance, 0.0f, 1.0f);
	if (bCharged)
	{
		CombatChar->DoAIChargedAttack();
		AINPC_LOG(Log, "Action_Attack: DoAIChargedAttack() fired");
	}
	else
	{
		CombatChar->DoAIComboAttack();
		AINPC_LOG(Log, "Action_Attack: DoAIComboAttack() fired");
	}
}

// ============================================================
// BeginRepositioning
// ============================================================

void UAction_Attack::BeginRepositioning(AAIController* Controller)
{
	APawn*  Pawn   = Controller ? Controller->GetPawn()        : nullptr;
	AActor* Target = Controller ? Controller->GetFocusActor()  : nullptr;

	if (!IsValid(Pawn) || !IsValid(Target))
	{
		RepositionTarget = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
		return;
	}

	// Radial direction from target to pawn, flattened
	FVector RadialDir = (Pawn->GetActorLocation() - Target->GetActorLocation());
	RadialDir.Z = 0.0f;
	if (RadialDir.IsNearlyZero()) RadialDir = Pawn->GetActorForwardVector();
	RadialDir = RadialDir.GetSafeNormal();

	// Pick a random arc offset and side
	const float SideSign  = FMath::RandBool() ? 1.0f : -1.0f;
	const float ArcAngle  = FMath::RandRange(15.0f, FMath::Max(15.0f, StrafeArcHalfAngleDeg));
	const FVector Dir     = RadialDir.RotateAngleAxis(ArcAngle * SideSign, FVector::UpVector).GetSafeNormal();
	const float   Radius  = FMath::RandRange(
		FMath::Min(StrafeRadiusMin, StrafeRadiusMax),
		FMath::Max(StrafeRadiusMin, StrafeRadiusMax));

	RepositionTarget = Target->GetActorLocation() + Dir * Radius;

	const EPathFollowingRequestResult::Type Result = Controller->MoveToLocation(
		RepositionTarget, StrafeAcceptanceRadius,
		/*bStopOnOverlap*/true, /*bUsePathfinding*/true,
		/*bProjectDestToNavigation*/true, /*bCanStrafe*/true);

	if (Result == EPathFollowingRequestResult::Failed)
	{
		AINPC_LOG_WARNING("Action_Attack: MoveToLocation failed — will timeout");
	}
	else
	{
		AINPC_LOG(Log, "Action_Attack: Repositioning to (%.0f, %.0f, %.0f)",
			RepositionTarget.X, RepositionTarget.Y, RepositionTarget.Z);
	}
}

// ============================================================
// HasReachedReposition
// ============================================================

bool UAction_Attack::HasReachedReposition(AAIController* Controller) const
{
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!IsValid(Pawn)) return true;

	if (FVector::DistSquared2D(Pawn->GetActorLocation(), RepositionTarget) <= FMath::Square(StrafeAcceptanceRadius))
		return true;

	if (Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
		return true;

	return false;
}

// ============================================================
// ResetCombatState
// ============================================================

void UAction_Attack::ResetCombatState()
{
	CombatPhase             = ECombatPhase::ChoosingAttack;
	bAttackCallbackReceived = false;
	PhaseStartTime          = 0.0f;
	RepositionTarget        = FVector::ZeroVector;
}

// ============================================================
// OnTargetInvalidated
// ============================================================

void UAction_Attack::OnTargetInvalidated(AAIController* Controller, AActor* OldTarget)
{
	if (Controller != OwningController) return;

	if (Controller->GetFocusActor() == OldTarget)
	{
		AINPC_LOG(Log, "Action_Attack: Target %s invalidated — resetting combat state", *OldTarget->GetName());
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
		Controller->StopMovement();
		ResetCombatState();
	}
}
