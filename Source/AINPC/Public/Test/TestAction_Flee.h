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

	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;

private:
	float ExecutionTime;
	
	UPROPERTY(EditAnywhere, Category = "Test")
	float MaxExecutionTime = 1.5f;
	
	// 是否完成
	bool bIsComplete;
	
	// 逃跑速度
	UPROPERTY(EditAnywhere, Category = "Flee")
	float FleeSpeed = 600.0f;
	
	// 安全距离
	UPROPERTY(EditAnywhere, Category = "Flee")
	float SafeDistance = 1000.0f;
};
