#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "Action_Patrol.generated.h"

/**
 * Action_Patrol: Selects a random waypoint in the navigation system and moves to it.
 * Used when there are no higher priority needs (Hunger/Sleep/Threat).
 */
UCLASS()
class AINPC_API UAction_Patrol : public UUtilityActionBase
{
	GENERATED_BODY()

public:
	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;
	virtual bool ShouldExit(AAIController* Controller) const override;

protected:
	// Find a random navigable point within radius
	void MoveToRandomPoint(AAIController* Controller);

	// Time to wait at each patrol point
	float PatrolWaitTime = 2.0f;
	
	// Radius to search for patrol points
	float PatrolRadius = 1500.0f;

	// Timer handle for waiting at a point
	FTimerHandle WaitTimerHandle;

	// Is currently waiting at a point?
	bool bIsWaiting = false;
};
