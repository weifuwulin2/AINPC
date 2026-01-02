// TestAction_Idle.h
// 测试用的待机动作

#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "TestAction_Idle.generated.h"

/**
 * 测试用待机动作
 * 用作默认/备选动作
 */
UCLASS(Blueprintable)
class AINPC_API UTestAction_Idle : public UUtilityActionBase
{
	GENERATED_BODY()

public:
	UTestAction_Idle();

	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;

private:
	float ExecutionTime;
	int32 IdleCounter;
};
