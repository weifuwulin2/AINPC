// TestAction_Attack.h
// 测试用的攻击动作

#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "TestAction_Attack.generated.h"

/**
 * 测试用攻击动作
 * 用于验证 Utility AI 框架和 LLM 集成
 */
UCLASS(Blueprintable)
class AINPC_API UTestAction_Attack : public UUtilityActionBase
{
	GENERATED_BODY()

public:
	UTestAction_Attack();

	// 动作生命周期
	virtual void OnEnter_Implementation(AAIController* Controller) override;
	virtual void OnExecute_Implementation(AAIController* Controller, float DeltaTime) override;
	virtual void OnExit_Implementation(AAIController* Controller) override;

private:
	// 执行时间计数器
	float ExecutionTime;
	
	// 最大执行时间（秒）
	UPROPERTY(EditAnywhere, Category = "Test")
	float MaxExecutionTime = 2.0f;
};
