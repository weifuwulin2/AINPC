
#include "Actions/Action_Attack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Social/SocialGameplayTags.h"
#include "UtilityAI/UNPCMentalState.h"
#include "AINPC.h"

UAction_Attack::UAction_Attack()
{
	ActionName = "Attack";
	bIsAttacking = false;
	bHasDealtDamage = false;
	TargetActor = nullptr;
	OwningController = nullptr;
	
	// No need to set InputWeights, Priority, etc. here - they come from DataTable via InitFromConfig
}

void UAction_Attack::Enter_Implementation(AAIController* Controller)
{
	Super::Enter_Implementation(Controller);
	
	if (!Controller)
	{
		AINPC_LOG_ERROR("Action_Attack: Controller is null!");
		return;
	}

	OwningController = Controller;
	TargetActor = Controller->GetFocusActor();
	bIsAttacking = false;
	bHasDealtDamage = false;
	
	if (!TargetActor)
	{
		AINPC_LOG_WARNING("Action_Attack: No target focused!");
		return;
	}

	AINPC_LOG(Log, "⚔️ ATTACK ACTION ENTERED - Target: %s", *TargetActor->GetName());
}

void UAction_Attack::Execute_Implementation(AAIController* Controller)
{
	Super::Execute_Implementation(Controller);

	if (!Controller || !TargetActor)
	{
		AINPC_LOG_WARNING("Action_Attack: Invalid Controller or Target during Execute");
		return;
	}

	OwningController = Controller;

	// Check if target is still valid and alive
	if (TargetActor->ActorHasTag("Dead") || TargetActor->ActorHasTag(FName("Dead")))
	{
		AINPC_LOG(Log, "Action_Attack: Target is dead, exiting combat");
		return;
	}

	// Check distance to target
	APawn* Pawn = Controller->GetPawn();
	if (!Pawn)
	{
		AINPC_LOG_WARNING("Action_Attack: No pawn for controller");
		return;
	}

	float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), TargetActor->GetActorLocation());
	float RangeSq = AttackRange * AttackRange;

	if (DistSq <= RangeSq)
	{
		// In range - perform attack
		PerformAttack(Controller);
	}
	else
	{
		// Out of range - pursue target
		FAIMoveRequest MoveReq(TargetActor);
		MoveReq.SetAcceptanceRadius(AttackRange * 0.8f);
		Controller->MoveTo(MoveReq);
		AINPC_LOG_VERBOSE("Action_Attack: Moving to target (distance: %.1f)", FMath::Sqrt(DistSq));
	}
}

void UAction_Attack::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);

	bIsAttacking = false;
	TargetActor = nullptr;
	OwningController = nullptr;

	if (Controller)
	{
		Controller->StopMovement();
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}

	AINPC_LOG_VERBOSE("Action_Attack: Exited");
}

void UAction_Attack::PerformAttack(AAIController* Controller)
{
	if (bIsAttacking || !Controller || !TargetActor)
	{
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
	if (!Character)
	{
		AINPC_LOG_WARNING("Action_Attack: Pawn is not a Character!");
		return;
	}

	bIsAttacking = true;
	bHasDealtDamage = false;

	// Stop movement and face target
	Controller->StopMovement();
	Controller->SetFocus(TargetActor);

	// Play attack animation
	if (AttackMontage)
	{
		float Duration = Character->PlayAnimMontage(AttackMontage);
		
		// Setup callback for animation end
		UAnimInstance* AnimInst = Character->GetMesh()->GetAnimInstance();
		if (AnimInst)
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &UAction_Attack::OnAttackAnimFinished);
			AnimInst->Montage_SetEndDelegate(EndDelegate, AttackMontage);
		}
		
		// Apply damage (in production, this should be triggered by AnimNotify)
		if (!bHasDealtDamage)
		{
			UGameplayStatics::ApplyDamage(TargetActor, DamageAmount, Controller, Character, UDamageType::StaticClass());
			bHasDealtDamage = true;
			AINPC_LOG(Log, "Action_Attack: Dealt %.1f damage to %s", DamageAmount, *TargetActor->GetName());
		}
	}
	else
	{
		// No animation - just deal damage instantly
		AINPC_LOG_WARNING("Action_Attack: No AttackMontage assigned, dealing instant damage");
		UGameplayStatics::ApplyDamage(TargetActor, DamageAmount, Controller, Character, UDamageType::StaticClass());
		bIsAttacking = false;
		bHasDealtDamage = true;
	}
}

void UAction_Attack::OnAttackAnimFinished(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == AttackMontage)
	{
		bIsAttacking = false;
		AINPC_LOG_VERBOSE("Action_Attack: Animation finished (Interrupted: %s)", bInterrupted ? TEXT("Yes") : TEXT("No"));
		// Action can now be re-evaluated by Utility AI system
	}
}
