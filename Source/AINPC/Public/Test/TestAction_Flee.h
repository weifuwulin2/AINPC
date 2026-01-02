// TestAction_Flee.h
// 测试用的逃跑动作

#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "TestAction_Flee.generated.h"

/**
 * 测试用逃跑动作
 * 用于验证高优先级动作的切换
 */
UCLASS(Blueprintable)
class AINPC_API UTestAction_Flee : public UUtilityActionBase
{
	GENERATED_BODY()

public:
	UTestAction_Flee();

	virtual void OnEnter_Implementation(AAIController* Controller) override;
	virtual void OnExecute_Implementation(AAIController* Controller, float DeltaTime) override;
	virtual void OnExit_Implementation(AAIController* Controller) override;

private:
	float ExecutionTime;
	
	UPROPERTY(EditAnywhere, Category = "Test")
	float MaxExecutionTime = 1.5f;
};
