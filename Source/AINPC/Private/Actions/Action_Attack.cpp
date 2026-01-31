
#include "Actions/Action_Attack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Social/SocialGameplayTags.h"
#include "UtilityAI/UNPCMentalState.h"
#include "AINPC.h"

#include "Subsystems/TargetSelectionSubsystem.h" // ✅ Added include

UAction_Attack::UAction_Attack()
{
	ActionName = "Attack";
	bIsAttacking = false;
	bHasDealtDamage = false;
	OwningController = nullptr;
	
	// Default Target Config (Can be overwritten by DataTable)
	bNeedsTarget = true;
	TargetContext = ETargetSelectionContext::Combat;
	
	// ✅ Override Threshold for Combat Actions
	// Ensure Neutrals (50) are safe from attack. Threshold must be < 50.
	TargetConfigOverride.FriendlyReputationThreshold = 45.0f;
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
	bIsAttacking = false;
	bHasDealtDamage = false;
	
	AINPC_LOG(Warning, "🎯 Action_Attack::Enter called - bNeedsTarget: %s, Controller: %s", 
		bNeedsTarget ? TEXT("TRUE") : TEXT("FALSE"), 
		Controller ? *Controller->GetName() : TEXT("NULL"));
	
	// ✅ Target Selection via Subsystem (Single Source of Truth)
	// -----------------------------------------------------
	AActor* SelectedTarget = nullptr;
	
	if (bNeedsTarget)
	{
		if (UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
		{
			// Merge Action config with Override
			FTargetSelectionConfig Config = TargetConfigOverride;
			
			// Select best target (Cached, Rule-Based, or LLM)
			SelectedTarget = TargetSystem->SelectTarget(Controller, TargetContext, Config);
			
			// ✅ Bind to invalidation event
			if (!TargetSystem->OnTargetInvalidated.IsAlreadyBound(this, &UAction_Attack::OnTargetInvalidated))
			{
				TargetSystem->OnTargetInvalidated.AddDynamic(this, &UAction_Attack::OnTargetInvalidated);
			}

			if (SelectedTarget)
			{
				AINPC_LOG(Log, "Action_Attack: ✅ Selected target '%s' via Subsystem (Context: %d)", 
					*SelectedTarget->GetName(), (int32)TargetContext);
			}
			else
			{
				AINPC_LOG_WARNING("Action_Attack: ❌ Target Selection failed! No valid target found");
			}
		}
		else
		{
			AINPC_LOG_ERROR("Action_Attack: TargetSelectionSubsystem not found!");
		}
	}
	
	// Fallback to legacy behavior if Subsystem failed
	if (!SelectedTarget)
	{
		SelectedTarget = Controller->GetFocusActor();
		if (SelectedTarget)
		{
			AINPC_LOG(Log, "Action_Attack: Using fallback FocusActor: %s", *SelectedTarget->GetName());
		}
		else
		{
			AINPC_LOG_WARNING("Action_Attack: Fallback FocusActor is also null!");
		}
	}
	
	if (!SelectedTarget)
	{
		AINPC_LOG_WARNING("Action_Attack: ⚠️ No target available - Action will be inactive!");
		return;
	}
	
	// ✅ SET FOCUS - This is now the ONLY source of truth for target
	Controller->SetFocus(SelectedTarget);

	AINPC_LOG(Log, "⚔️ ATTACK ACTION ENTERED - Target: %s", *SelectedTarget->GetName());
}

void UAction_Attack::Execute_Implementation(AAIController* Controller)
{
	Super::Execute_Implementation(Controller);

	if (!Controller)
	{
		AINPC_LOG_WARNING("Action_Attack: Invalid Controller during Execute");
		return;
	}
	
	OwningController = Controller;
	
	// ✅ Get target from Focus (single source of truth)
	AActor* Target = Controller->GetFocusActor();
	
	if (!Target)
	{
		// ⚠️ Recovery: Focus lost but action still active. Try to recover via Subsystem.
		if (UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
		{
			FTargetSelectionConfig Config = TargetConfigOverride;
			Target = TargetSystem->SelectTarget(Controller, TargetContext, Config);
			
			if (Target)
			{
				// Recovered! Restore focus.
				Controller->SetFocus(Target);
				AINPC_LOG(Log, "Action_Attack: 🔄 Recovered lost focus target: %s", *Target->GetName());
			}
		}
		
		if (!Target)
		{
			AINPC_LOG_WARNING("Action_Attack: No FocusActor and Recovery Failed - clearing action state");
			Controller->ClearFocus(EAIFocusPriority::Gameplay);
			// Force invalidation
			bIsAttacking = false;
			return;
		}
	}

	// Check if target is still valid and alive
	if (!IsValid(Target) || Target->IsPendingKillPending() ||
		Target->ActorHasTag(FName("Dead")) || Target->ActorHasTag(FName("Status.Dead")))
	{
		AINPC_LOG(Log, "Action_Attack: Target is dead or invalid, clearing focus...");
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	// Check distance to target
	APawn* Pawn = Controller->GetPawn();
	if (!Pawn)
	{
		AINPC_LOG_WARNING("Action_Attack: No pawn for controller");
		return;
	}

	float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), Target->GetActorLocation());
	float RangeSq = AttackRange * AttackRange;

	if (DistSq <= RangeSq)
	{
		// In range - perform attack
		PerformAttack(Controller);
	}
	else
	{
		// Out of range - pursue target
		FAIMoveRequest MoveReq(Target);
		MoveReq.SetAcceptanceRadius(AttackRange * 0.8f);
		Controller->MoveTo(MoveReq);
		AINPC_LOG_VERBOSE("Action_Attack: Moving to target (distance: %.1f)", FMath::Sqrt(DistSq));
	}
}

void UAction_Attack::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);

	bIsAttacking = false;
	OwningController = nullptr;

	if (Controller)
	{
		Controller->StopMovement();
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
	
	if (UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
	{
		TargetSystem->OnTargetInvalidated.RemoveDynamic(this, &UAction_Attack::OnTargetInvalidated);
	}

	AINPC_LOG_VERBOSE("Action_Attack: Exited");
}

void UAction_Attack::PerformAttack(AAIController* Controller)
{
	if (bIsAttacking || !Controller)
	{
		return;
	}
	
	// Get target from Focus
	AActor* Target = Controller->GetFocusActor();
	if (!Target)
	{
		AINPC_LOG_WARNING("Action_Attack::PerformAttack - No target in focus!");
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
	// Note: Focus is already set, no need to SetFocus again

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
			UGameplayStatics::ApplyDamage(Target, DamageAmount, Controller, Character, UDamageType::StaticClass());
			bHasDealtDamage = true;
			AINPC_LOG(Log, "Action_Attack: Dealt %.1f damage to %s", DamageAmount, *Target->GetName());
		}
	}
	else
	{
		// No animation - just deal damage instantly
		AINPC_LOG_WARNING("Action_Attack: No AttackMontage assigned, dealing instant damage");
		UGameplayStatics::ApplyDamage(Target, DamageAmount, Controller, Character, UDamageType::StaticClass());
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

void UAction_Attack::OnTargetInvalidated(AAIController* Controller, AActor* OldTarget)
{
	if (Controller != OwningController)
	{
		return;
	}

	AActor* CurrentTarget = Controller ? Controller->GetFocusActor() : nullptr;
	if (CurrentTarget == OldTarget)
	{
		AINPC_LOG(Log, "Action_Attack: Current target %s invalidated externally! Ending action.", *OldTarget->GetName());
		if (Controller)
		{
			Controller->ClearFocus(EAIFocusPriority::Gameplay);
			Controller->StopMovement();
		}
		
		// Force action update/exit
		bIsAttacking = false;
	}
}
