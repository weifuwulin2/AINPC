#include "Actions/Action_Patrol.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Controller/UtilityAIController.h"
#include "Components/SensoryComponent.h"

void UAction_Patrol::Enter_Implementation(AAIController* Controller)
{
	if (!Controller) return;
	
	ActionStartTime = Controller->GetWorld()->GetTimeSeconds();
	bIsWaiting = false;

	// Start moving immediately
	MoveToRandomPoint(Controller);
	
	UE_LOG(LogTemp, Log, TEXT("[%s] Started Patrolling"), *ActionName);
}

void UAction_Patrol::Execute_Implementation(AAIController* Controller)
{
	if (!Controller) return;

	// Check if reached destination
	if (Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		if (!bIsWaiting)
		{
			// Arrived at point, start waiting
			bIsWaiting = true;

			// Set timer to move to next point
			TWeakObjectPtr<AAIController> WeakController(Controller);
			TWeakObjectPtr<UAction_Patrol> WeakSelf(this);
			Controller->GetWorld()->GetTimerManager().SetTimer(
				WaitTimerHandle,
				[WeakSelf, WeakController]()
				{
					if (UAction_Patrol* Self = WeakSelf.Get())
					{
						if (AAIController* Ctrl = WeakController.Get())
						{
							Self->MoveToRandomPoint(Ctrl);
						}
					}
				},
				PatrolWaitTime,
				false
			);
			
			UE_LOG(LogTemp, Log, TEXT("[%s] Arrived. Waiting for %.1fs..."), *ActionName, PatrolWaitTime);
		}
	}
}

void UAction_Patrol::Exit_Implementation(AAIController* Controller)
{
	if (Controller)
	{
		Controller->StopMovement();
		if (Controller->GetWorld())
		{
			Controller->GetWorld()->GetTimerManager().ClearTimer(WaitTimerHandle);
		}
	}
	bIsWaiting = false;
	UE_LOG(LogTemp, Log, TEXT("[%s] Stopped Patrolling"), *ActionName);
}

bool UAction_Patrol::ShouldExit(AAIController* Controller) const
{
	// ✅ Patrol is a low-priority filler action.
	// It should ALWAYS be interruptible by anything else (Hunger, Threat, etc.)
	// UNLESS we just started moving (jitter protection)
	
	float ElapsedTime = Controller->GetWorld()->GetTimeSeconds() - ActionStartTime;
	if (ElapsedTime < 1.0f) 
	{
		// Minimum 1s commitment to prevent micro-stuttering
		return false; 
	}

	// Always allow exit otherwise
	return true;
}

void UAction_Patrol::MoveToRandomPoint(AAIController* Controller)
{
	if (!Controller || !Controller->GetPawn()) return;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Controller->GetWorld());
	if (NavSys)
	{
		FNavLocation RandomPt;
		if (NavSys->GetRandomReachablePointInRadius(Controller->GetPawn()->GetActorLocation(), PatrolRadius, RandomPt))
		{
			Controller->MoveToLocation(RandomPt.Location);
			bIsWaiting = false;
			UE_LOG(LogTemp, Verbose, TEXT("[%s] Moving to %s"), *ActionName, *RandomPt.Location.ToString());
		}
	}
}
