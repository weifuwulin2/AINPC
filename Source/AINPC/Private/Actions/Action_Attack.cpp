
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
	
	// ✅ Increase default range to prevent collision blocking
	AttackRange = 250.0f;
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
			// Explicitly remove first to ensure no stale bindings (Fix Bug #2)
			TargetSystem->OnTargetInvalidated.RemoveDynamic(this, &UAction_Attack::OnTargetInvalidated);
			TargetSystem->OnTargetInvalidated.AddDynamic(this, &UAction_Attack::OnTargetInvalidated);

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
	
	// ... (Target Recovery Logic Omitted for Brevity) ...
	
	if (!Target)
	{
		// ... (Recovery Logic) ...
		if (UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
		{
			FTargetSelectionConfig Config = TargetConfigOverride;
			Target = TargetSystem->SelectTarget(Controller, TargetContext, Config);
			if (Target) Controller->SetFocus(Target);
		}

		if (!Target)
		{
			AINPC_LOG_WARNING("Action_Attack: No FocusActor and Recovery Failed - clearing action state");
			Controller->ClearFocus(EAIFocusPriority::Gameplay);
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
	
	// Debug Distance Log
	// AINPC_LOG(Log, "Action_Attack: Execute Tick. Dist: %.1f / Range: %.1f. bIsAttacking: %d", FMath::Sqrt(DistSq), AttackRange, bIsAttacking);

	// Safety timeout: force-reset bIsAttacking if stuck (e.g., anim callback never fired)
	if (bIsAttacking && AttackStartTime > 0.0f)
	{
		float TimeSinceAttack = GetWorld()->GetTimeSeconds() - AttackStartTime;
		if (TimeSinceAttack > 5.0f)
		{
			AINPC_LOG_WARNING("Action_Attack: bIsAttacking stuck for %.1fs - force resetting", TimeSinceAttack);
			bIsAttacking = false;
		}
	}

	if (DistSq <= RangeSq)
	{
		// In range - perform attack
		PerformAttack(Controller);
	}
	else
	{
		// Out of range - pursue target
		FAIMoveRequest MoveReq(Target);
		MoveReq.SetAcceptanceRadius(50.0f);
		MoveReq.SetAllowPartialPath(true); // Allow moving along partial NavMesh for distant targets

		EPathFollowingRequestResult::Type Result = Controller->MoveTo(MoveReq);

		FString ResultStr;
		switch(Result) {
			case EPathFollowingRequestResult::Failed: ResultStr = "Failed"; break;
			case EPathFollowingRequestResult::AlreadyAtGoal: ResultStr = "AlreadyAtGoal"; break;
			case EPathFollowingRequestResult::RequestSuccessful: ResultStr = "RequestSuccessful"; break;
		}

		AINPC_LOG(Log, "Action_Attack: Moving to target (Dist: %.1f). Result: %s",
			FMath::Sqrt(DistSq), *ResultStr);
	}
}

void UAction_Attack::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);

	bIsAttacking = false;
	AttackStartTime = 0.0f;

	// Stop montage and unbind delegate to prevent stale callbacks
	if (Controller)
	{
		if (ACharacter* Character = Cast<ACharacter>(Controller->GetPawn()))
		{
			if (AttackMontage && Character->GetMesh())
			{
				if (UAnimInstance* AnimInst = Character->GetMesh()->GetAnimInstance())
				{
					if (AnimInst->Montage_IsPlaying(AttackMontage))
					{
						Character->StopAnimMontage(AttackMontage);
					}
				}
			}
		}
		Controller->StopMovement();
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}

	OwningController = nullptr;

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
	AttackStartTime = Controller->GetWorld()->GetTimeSeconds();

	// Stop movement and face target
	Controller->StopMovement();
	// Note: Focus is already set, no need to SetFocus again

	// Play attack animation
	if (AttackMontage)
	{
		float Duration = Character->PlayAnimMontage(AttackMontage);
		AINPC_LOG(Log, "Action_Attack: Playing Montage %s (Duration: %.2f)", *AttackMontage->GetName(), Duration);

		if (Duration > 0.f)
		{
			// Setup callback for animation end
			UAnimInstance* AnimInst = Character->GetMesh()->GetAnimInstance();
			if (AnimInst)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UAction_Attack::OnAttackAnimFinished);
				AnimInst->Montage_SetEndDelegate(EndDelegate, AttackMontage);
			}
			
			// Apply damage (in production, this should be triggered by AnimNotify)
			float PreHealth = 0.f; // Debug
			if (ACharacter* TChar = Cast<ACharacter>(Target)) 
			{
				 // PreHealth = TChar->GetHealth(); // If property exists
			}
			
			AINPC_LOG(Log, "Action_Attack: 💥 Attempting ApplyDamage on %s (Amt: %.1f)", *Target->GetName(), DamageAmount);
			
			UGameplayStatics::ApplyDamage(Target, DamageAmount, Controller, Character, UDamageType::StaticClass());
			bHasDealtDamage = true;
			
			AINPC_LOG(Log, "Action_Attack: ✅ Dealt %.1f damage to %s", DamageAmount, *Target->GetName());
		}
		else
		{
			// Montage failed to play! Abort locking state to prevent freeze.
			AINPC_LOG_WARNING("Action_Attack: Montage %s failed to play! Resetting state.", *AttackMontage->GetName());
			
			// Still deal damage or abort? Let's deal damage for now but reset.
			UGameplayStatics::ApplyDamage(Target, DamageAmount, Controller, Character, UDamageType::StaticClass());
			bIsAttacking = false;
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
