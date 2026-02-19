// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatStateTreeUtility.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeExecutionTypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "CombatEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeAsyncExecutionContext.h"
#include "Navigation/PathFollowingComponent.h"
#include "AINPC.h"

bool FStateTreeCharacterGroundedCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// is the character currently grounded?
	bool bCondition = InstanceData.Character->GetMovementComponent()->IsMovingOnGround();

	return InstanceData.bMustBeOnAir ? !bCondition : bCondition;
}

#if WITH_EDITOR
FText FStateTreeCharacterGroundedCondition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Is Character Grounded</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeComboAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	COMBAT_ST_LOG(Warning, "[ST-DBG] ComboAttackTask::EnterState called. ChangeType=%d", (int32)Transition.ChangeType);

	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		COMBAT_ST_LOG(Warning, "[ST-DBG] ComboAttackTask: Character=%s",
			IsValid(InstanceData.Character) ? *InstanceData.Character->GetName() : TEXT("NULL/INVALID"));

		if (!IsValid(InstanceData.Character))
		{
			COMBAT_ST_LOG(Error, "[ST-DBG] ComboAttackTask: Character is NULL! Task will not execute.");
			return EStateTreeRunStatus::Failed;
		}

		// bind to the on attack completed delegate
		InstanceData.Character->OnAttackCompleted.BindLambda(
			[WeakContext = Context.MakeWeakExecutionContext()]()
			{
				COMBAT_ST_LOG(Warning, "[ST-DBG] ComboAttackTask: OnAttackCompleted fired!");
				WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
			}
		);


		// tell the character to do a combo attack
		InstanceData.Character->DoAIComboAttack();
		COMBAT_ST_LOG(Warning, "[ST-DBG] ComboAttackTask: DoAIComboAttack() called");
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeComboAttackTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// unbind the on attack completed delegate
		InstanceData.Character->OnAttackCompleted.Unbind();
	}
}

#if WITH_EDITOR
FText FStateTreeComboAttackTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Do Combo Attack</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeChargedAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	COMBAT_ST_LOG(Warning, "[ST-DBG] ChargedAttackTask::EnterState called. ChangeType=%d", (int32)Transition.ChangeType);

	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		COMBAT_ST_LOG(Warning, "[ST-DBG] ChargedAttackTask: Character=%s",
			IsValid(InstanceData.Character) ? *InstanceData.Character->GetName() : TEXT("NULL/INVALID"));

		if (!IsValid(InstanceData.Character))
		{
			COMBAT_ST_LOG(Error, "[ST-DBG] ChargedAttackTask: Character is NULL! Task will not execute.");
			return EStateTreeRunStatus::Failed;
		}

		// bind to the on attack completed delegate
		InstanceData.Character->OnAttackCompleted.BindLambda(
			[WeakContext = Context.MakeWeakExecutionContext()]()
			{
				COMBAT_ST_LOG(Warning, "[ST-DBG] ChargedAttackTask: OnAttackCompleted fired!");
				WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
			}
		);

		// tell the character to do a charged attack
		InstanceData.Character->DoAIChargedAttack();
		COMBAT_ST_LOG(Warning, "[ST-DBG] ChargedAttackTask: DoAIChargedAttack() called");
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeChargedAttackTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// unbind the on attack completed delegate
		InstanceData.Character->OnAttackCompleted.Unbind();
	}
}

#if WITH_EDITOR
FText FStateTreeChargedAttackTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Do Charged Attack</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeWaitForLandingTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// bind to the on enemy landed delegate
		InstanceData.Character->OnEnemyLanded.BindLambda(
			[WeakContext = Context.MakeWeakExecutionContext()]()
			{
				WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
			}
		);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeWaitForLandingTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// bind the on enemy landed delegate
		InstanceData.Character->OnEnemyLanded.Unbind();
	}
}

#if WITH_EDITOR
FText FStateTreeWaitForLandingTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Wait for Landing</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeFaceActorTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// set the AI Controller's focus
		InstanceData.Controller->SetFocus(InstanceData.ActorToFaceTowards);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeFaceActorTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned to another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// clear the AI Controller's focus
		InstanceData.Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

#if WITH_EDITOR
FText FStateTreeFaceActorTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Face Towards Actor</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeFaceLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// set the AI Controller's focus
		InstanceData.Controller->SetFocalPoint(InstanceData.FaceLocation);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeFaceLocationTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned to another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// clear the AI Controller's focus
		InstanceData.Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

#if WITH_EDITOR
FText FStateTreeFaceLocationTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Face Towards Location</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeSetCharacterSpeedTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// set the character's max ground speed
		InstanceData.Character->GetCharacterMovement()->MaxWalkSpeed = InstanceData.Speed;
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FStateTreeSetCharacterSpeedTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Set Character Speed</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeGetPlayerInfoTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// get the instance data
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// get the character possessed by the first local player
	InstanceData.TargetPlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(InstanceData.Character, 0));

	// do we have a valid target?
	if (InstanceData.TargetPlayerCharacter)
	{
		// update the last known location
		InstanceData.TargetPlayerLocation = InstanceData.TargetPlayerCharacter->GetActorLocation();
	}

	// update the distance
	InstanceData.DistanceToTarget = FVector::Distance(InstanceData.TargetPlayerLocation, InstanceData.Character->GetActorLocation());

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FStateTreeGetPlayerInfoTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Get Player Info</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

namespace
{
	AActor* ResolveCombatTarget(AAIController* Controller, AActor* ExplicitTarget)
	{
		if (IsValid(ExplicitTarget))
		{
			return ExplicitTarget;
		}

		return IsValid(Controller) ? Controller->GetFocusActor() : nullptr;
	}
}

bool FStateTreeCombatTargetDistanceCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	bool bInRange = false;

	AAIController* Controller = InstanceData.Controller.Get();
	APawn* Pawn = IsValid(Controller) ? Controller->GetPawn() : nullptr;
	AActor* Target = ResolveCombatTarget(Controller, InstanceData.TargetActor.Get());

	if (IsValid(Pawn) && IsValid(Target))
	{
		const float MinDistance = FMath::Min(InstanceData.MinDistance, InstanceData.MaxDistance);
		const float MaxDistance = FMath::Max(InstanceData.MinDistance, InstanceData.MaxDistance);

		const float DistanceSquared = InstanceData.bUse3DDistance
			? FVector::DistSquared(Pawn->GetActorLocation(), Target->GetActorLocation())
			: FVector::DistSquared2D(Pawn->GetActorLocation(), Target->GetActorLocation());

		bInRange = DistanceSquared >= FMath::Square(MinDistance) && DistanceSquared <= FMath::Square(MaxDistance);

		COMBAT_ST_LOG(Log, "[ST-DBG] DistanceCondition: Dist=%.1f, Range=[%.1f, %.1f], InRange=%s, Invert=%s => Result=%s",
			FMath::Sqrt(DistanceSquared), MinDistance, MaxDistance,
			bInRange ? TEXT("Y") : TEXT("N"),
			InstanceData.bInvert ? TEXT("Y") : TEXT("N"),
			(InstanceData.bInvert ? !bInRange : bInRange) ? TEXT("PASS") : TEXT("FAIL"));
	}
	else
	{
		COMBAT_ST_LOG(Warning, "[ST-DBG] DistanceCondition: Pawn=%s, Target=%s - cannot evaluate",
			IsValid(Pawn) ? TEXT("OK") : TEXT("NULL"),
			IsValid(Target) ? TEXT("OK") : TEXT("NULL"));
	}

	return InstanceData.bInvert ? !bInRange : bInRange;
}

#if WITH_EDITOR
FText FStateTreeCombatTargetDistanceCondition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString("<b>Combat Target Distance In Range</b>");
}
#endif

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeCombatStrafeTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	COMBAT_ST_LOG(Warning, "[ST-DBG] CombatStrafeTask::EnterState called. ChangeType=%d", (int32)Transition.ChangeType);

	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		AAIController* Controller = InstanceData.Controller.Get();
		APawn* Pawn = IsValid(Controller) ? Controller->GetPawn() : nullptr;
		AActor* Target = ResolveCombatTarget(Controller, InstanceData.TargetActor.Get());

		COMBAT_ST_LOG(Warning, "[ST-DBG] CombatStrafeTask: Controller=%s, Pawn=%s, Target=%s",
			IsValid(Controller) ? *Controller->GetName() : TEXT("NULL"),
			IsValid(Pawn) ? *Pawn->GetName() : TEXT("NULL"),
			IsValid(Target) ? *Target->GetName() : TEXT("NULL"));

		if (!IsValid(Controller) || !IsValid(Pawn) || !IsValid(Target))
		{
			COMBAT_ST_LOG(Error, "[ST-DBG] CombatStrafeTask: FAILED - missing Controller/Pawn/Target");
			return EStateTreeRunStatus::Failed;
		}

		UWorld* World = Pawn->GetWorld();
		if (!IsValid(World))
		{
			return EStateTreeRunStatus::Failed;
		}

		InstanceData.StartTimeSeconds = World->GetTimeSeconds();

		FVector RadialDirection = Pawn->GetActorLocation() - Target->GetActorLocation();
		RadialDirection.Z = 0.0f;

		if (RadialDirection.IsNearlyZero())
		{
			RadialDirection = Pawn->GetActorForwardVector();
			RadialDirection.Z = 0.0f;
		}

		RadialDirection = RadialDirection.GetSafeNormal();
		if (RadialDirection.IsNearlyZero())
		{
			return EStateTreeRunStatus::Failed;
		}

		const float RadiusMin = FMath::Min(InstanceData.StrafeRadiusMin, InstanceData.StrafeRadiusMax);
		const float RadiusMax = FMath::Max(InstanceData.StrafeRadiusMin, InstanceData.StrafeRadiusMax);
		const float Radius = FMath::RandRange(RadiusMin, RadiusMax);

		const float SideSign = InstanceData.bRandomizeSide
			? (FMath::RandBool() ? 1.0f : -1.0f)
			: (InstanceData.bClockwiseSide ? 1.0f : -1.0f);
		const float AngleMagnitude = FMath::RandRange(15.0f, FMath::Max(15.0f, InstanceData.ArcHalfAngleDeg));
		const FVector StrafeDirection = RadialDirection.RotateAngleAxis(AngleMagnitude * SideSign, FVector::UpVector).GetSafeNormal();

		InstanceData.MoveTargetLocation = Target->GetActorLocation() + StrafeDirection * Radius;

		const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
			InstanceData.MoveTargetLocation,
			InstanceData.AcceptanceRadius,
			true,
			true,
			true,
			true,
			nullptr,
			true
		);

		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			return EStateTreeRunStatus::Failed;
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeCombatStrafeTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AAIController* Controller = InstanceData.Controller.Get();
	APawn* Pawn = IsValid(Controller) ? Controller->GetPawn() : nullptr;
	if (!IsValid(Controller) || !IsValid(Pawn))
	{
		return EStateTreeRunStatus::Failed;
	}

	UWorld* World = Pawn->GetWorld();
	if (!IsValid(World))
	{
		return EStateTreeRunStatus::Failed;
	}

	const float DistanceToGoalSq = FVector::DistSquared2D(Pawn->GetActorLocation(), InstanceData.MoveTargetLocation);
	if (DistanceToGoalSq <= FMath::Square(InstanceData.AcceptanceRadius))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (InstanceData.MoveTimeout > 0.0f && (World->GetTimeSeconds() - InstanceData.StartTimeSeconds) >= InstanceData.MoveTimeout)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeCombatStrafeTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
		AAIController* Controller = InstanceData.Controller.Get();

		if (IsValid(Controller) && InstanceData.bStopMovementOnExit)
		{
			Controller->StopMovement();
		}
	}
}

#if WITH_EDITOR
FText FStateTreeCombatStrafeTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString("<b>Strafe Around Target</b>");
}
#endif

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeCombatAttackPatternTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	COMBAT_ST_LOG(Warning, "[ST-DBG] CombatAttackPatternTask::EnterState called. ChangeType=%d", (int32)Transition.ChangeType);

	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
		ACombatEnemy* Character = InstanceData.Character.Get();

		COMBAT_ST_LOG(Warning, "[ST-DBG] CombatAttackPatternTask: Character=%s",
			IsValid(Character) ? *Character->GetName() : TEXT("NULL/INVALID"));

		if (!IsValid(Character) || !IsValid(Character->GetWorld()))
		{
			COMBAT_ST_LOG(Error, "[ST-DBG] CombatAttackPatternTask: FAILED - Character or World invalid");
			return EStateTreeRunStatus::Failed;
		}

		InstanceData.StartTimeSeconds = Character->GetWorld()->GetTimeSeconds();

		Character->OnAttackCompleted.Unbind();
		Character->OnAttackCompleted.BindLambda(
			[WeakContext = Context.MakeWeakExecutionContext()]()
			{
				COMBAT_ST_LOG(Warning, "[ST-DBG] CombatAttackPatternTask: OnAttackCompleted fired!");
				WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
			}
		);

		const float ChargedChance = FMath::Clamp(InstanceData.ChargedAttackChance, 0.0f, 1.0f);
		const bool bDoCharged = FMath::FRand() <= ChargedChance;
		COMBAT_ST_LOG(Warning, "[ST-DBG] CombatAttackPatternTask: Chose %s (ChargedChance=%.2f)",
			bDoCharged ? TEXT("CHARGED") : TEXT("COMBO"), ChargedChance);

		if (bDoCharged)
		{
			Character->DoAIChargedAttack();
		}
		else
		{
			Character->DoAIComboAttack();
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeCombatAttackPatternTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	ACombatEnemy* Character = InstanceData.Character.Get();

	if (!IsValid(Character) || !IsValid(Character->GetWorld()))
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.AttackTimeout > 0.0f &&
		(Character->GetWorld()->GetTimeSeconds() - InstanceData.StartTimeSeconds) >= InstanceData.AttackTimeout)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeCombatAttackPatternTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
		ACombatEnemy* Character = InstanceData.Character.Get();

		if (IsValid(Character))
		{
			Character->OnAttackCompleted.Unbind();
		}
	}
}

#if WITH_EDITOR
FText FStateTreeCombatAttackPatternTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString("<b>Execute Combat Attack Pattern</b>");
}
#endif

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeCombatRecoverTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	COMBAT_ST_LOG(Warning, "[ST-DBG] CombatRecoverTask::EnterState called. ChangeType=%d", (int32)Transition.ChangeType);

	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
		AAIController* Controller = InstanceData.Controller.Get();

		COMBAT_ST_LOG(Warning, "[ST-DBG] CombatRecoverTask: Controller=%s, Duration=%.2f",
			IsValid(Controller) ? *Controller->GetName() : TEXT("NULL"),
			InstanceData.RecoverDuration);

		if (!IsValid(Controller) || !IsValid(Controller->GetWorld()))
		{
			COMBAT_ST_LOG(Error, "[ST-DBG] CombatRecoverTask: FAILED - Controller or World invalid");
			return EStateTreeRunStatus::Failed;
		}

		if (InstanceData.bStopMovementOnEnter)
		{
			Controller->StopMovement();
		}

		InstanceData.StartTimeSeconds = Controller->GetWorld()->GetTimeSeconds();

		if (InstanceData.RecoverDuration <= 0.0f)
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeCombatRecoverTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAIController* Controller = InstanceData.Controller.Get();

	if (!IsValid(Controller) || !IsValid(Controller->GetWorld()))
	{
		return EStateTreeRunStatus::Failed;
	}

	return (Controller->GetWorld()->GetTimeSeconds() - InstanceData.StartTimeSeconds) >= InstanceData.RecoverDuration
		? EStateTreeRunStatus::Succeeded
		: EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FStateTreeCombatRecoverTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString("<b>Combat Recover Delay</b>");
}
#endif
