#include "Actions/Action_Attack.h"
#include "AIController.h"
#include "Components/StateTreeAIComponent.h"
#include "GameFramework/Character.h"
#include "Social/SocialGameplayTags.h"
#include "StateTree.h"
#include "AINPC.h"

#include "Subsystems/TargetSelectionSubsystem.h"

UAction_Attack::UAction_Attack()
{
	ActionName = "Attack";
	OwningController = nullptr;
	ActiveStateTreeComponent.Reset();
	bCombatStateTreeActivated = false;

	// Default Target Config (Can be overwritten by DataTable)
	bNeedsTarget = true;
	TargetContext = ETargetSelectionContext::Combat;

	// Override threshold for combat actions
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

	AINPC_LOG(Warning, "Action_Attack::Enter called - bNeedsTarget: %s, Controller: %s",
		bNeedsTarget ? TEXT("TRUE") : TEXT("FALSE"),
		Controller ? *Controller->GetName() : TEXT("NULL"));

	// Target Selection via Subsystem (single source of truth)
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

			// Bind to invalidation event
			// Explicitly remove first to ensure no stale bindings (Fix Bug #2)
			TargetSystem->OnTargetInvalidated.RemoveDynamic(this, &UAction_Attack::OnTargetInvalidated);
			TargetSystem->OnTargetInvalidated.AddDynamic(this, &UAction_Attack::OnTargetInvalidated);

			if (SelectedTarget)
			{
				AINPC_LOG(Log, "Action_Attack: Selected target '%s' via Subsystem (Context: %d)",
					*SelectedTarget->GetName(), (int32)TargetContext);
			}
			else
			{
				AINPC_LOG_WARNING("Action_Attack: Target selection failed, no valid target found");
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
		AINPC_LOG_WARNING("Action_Attack: No target available - action will be inactive");
		return;
	}

	// Set focus as the single source of truth for target
	Controller->SetFocus(SelectedTarget);

	AINPC_LOG(Log, "ATTACK ACTION ENTERED - Target: %s", *SelectedTarget->GetName());
	StartCombatStateTree(Controller);
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

	// Get target from focus (single source of truth)
	AActor* Target = Controller->GetFocusActor();

	// Target recovery: if focus lost, try to reacquire via subsystem
	if (!Target)
	{
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

	// StateTree handles all movement and combat — nothing else to do here
}

void UAction_Attack::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);

	StopCombatStateTree();

	if (Controller)
	{
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

void UAction_Attack::StartCombatStateTree(AAIController* Controller)
{
	AINPC_LOG(Warning, "[ST-DBG] StartCombatStateTree called. bUseCombatStateTree=%s, Controller=%s",
		bUseCombatStateTree ? TEXT("TRUE") : TEXT("FALSE"),
		IsValid(Controller) ? *Controller->GetName() : TEXT("INVALID"));

	if (!bUseCombatStateTree || !IsValid(Controller))
	{
		return;
	}

	UStateTreeAIComponent* StateTreeComponent = Controller->FindComponentByClass<UStateTreeAIComponent>();
	if (!IsValid(StateTreeComponent))
	{
		AINPC_LOG(Error, "[ST-DBG] No UStateTreeAIComponent on %s! It should be created in AUtilityAIController constructor.", *Controller->GetName());
		return;
	}

	AINPC_LOG(Warning, "[ST-DBG] Found StateTreeAIComponent: %s, IsRunning=%s, HasBegunPlay=%s",
		*StateTreeComponent->GetName(),
		StateTreeComponent->IsRunning() ? TEXT("YES") : TEXT("NO"),
		StateTreeComponent->HasBegunPlay() ? TEXT("YES") : TEXT("NO"));

	// Stop existing StateTree if running
	if (StateTreeComponent->IsRunning())
	{
		if (!bAllowReplacingRunningStateTree)
		{
			AINPC_LOG(Warning, "[ST-DBG] StateTree already running, replacement disabled.");
			return;
		}
		StateTreeComponent->StopLogic(TEXT("Action_Attack switching to combat StateTree"));
	}

	// Set the combat StateTree asset
	if (!IsValid(CombatStateTreeAsset.Get()))
	{
		AINPC_LOG(Error, "[ST-DBG] CombatStateTreeAsset is NULL! Cannot start combat StateTree.");
		return;
	}

	StateTreeComponent->SetStateTree(CombatStateTreeAsset.Get());
	AINPC_LOG(Warning, "[ST-DBG] SetStateTree: %s", *CombatStateTreeAsset->GetName());

	// Start the StateTree
	StateTreeComponent->StartLogic();
	AINPC_LOG(Warning, "[ST-DBG] After StartLogic(): IsRunning=%s",
		StateTreeComponent->IsRunning() ? TEXT("YES") : TEXT("NO"));

	ActiveStateTreeComponent = StateTreeComponent;
	bCombatStateTreeActivated = StateTreeComponent->IsRunning();

	AINPC_LOG(Warning, "[ST-DBG] === RESULT: bCombatStateTreeActivated=%s, ActiveComp=%s ===",
		bCombatStateTreeActivated ? TEXT("YES") : TEXT("NO"),
		ActiveStateTreeComponent.IsValid() ? TEXT("VALID") : TEXT("INVALID"));
}

void UAction_Attack::StopCombatStateTree()
{
	AINPC_LOG(Warning, "[ST-DBG] StopCombatStateTree called. bUseCombatStateTree=%s, bCombatStateTreeActivated=%s, bStopOnExit=%s",
		bUseCombatStateTree ? TEXT("TRUE") : TEXT("FALSE"),
		bCombatStateTreeActivated ? TEXT("TRUE") : TEXT("FALSE"),
		bStopCombatStateTreeOnExit ? TEXT("TRUE") : TEXT("FALSE"));

	if (!bUseCombatStateTree)
	{
		ActiveStateTreeComponent.Reset();
		bCombatStateTreeActivated = false;
		return;
	}

	if (bStopCombatStateTreeOnExit && bCombatStateTreeActivated)
	{
		if (UStateTreeAIComponent* StateTreeComponent = ActiveStateTreeComponent.Get())
		{
			AINPC_LOG(Warning, "[ST-DBG] Stopping combat StateTree. IsRunning=%s",
				StateTreeComponent->IsRunning() ? TEXT("YES") : TEXT("NO"));
			if (StateTreeComponent->IsRunning())
			{
				StateTreeComponent->StopLogic(TEXT("Action_Attack exited"));
				AINPC_LOG(Warning, "[ST-DBG] StopLogic called. IsRunning after=%s",
					StateTreeComponent->IsRunning() ? TEXT("YES") : TEXT("NO"));
			}
		}
		else
		{
			AINPC_LOG(Warning, "[ST-DBG] ActiveStateTreeComponent is NULL/stale during stop!");
		}
	}

	ActiveStateTreeComponent.Reset();
	bCombatStateTreeActivated = false;
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
		AINPC_LOG(Log, "Action_Attack: Current target %s invalidated externally! Clearing focus.", *OldTarget->GetName());
		if (Controller)
		{
			Controller->ClearFocus(EAIFocusPriority::Gameplay);
			Controller->StopMovement();
		}
	}
}
