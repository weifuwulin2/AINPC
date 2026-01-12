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
	// 动作生命周期
	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;

	// 重写 CalculateScore 以添加目标有效性检查
	// Override CalculateScore to add target validity check
	virtual float CalculateScore(UNPCMentalState* MentalState, AAIController* Controller, bool bLogDebug = false) override;

private:
	// 执行开始时间戳
	float ExecutionTime;
	
	// 最大执行时间（秒）
	UPROPERTY(EditAnywhere, Category = "Test")
	float MaxExecutionTime = 2.0f;
	
	// 是否完成
	bool bIsComplete;
	
	// 攻击范围
	UPROPERTY(EditAnywhere, Category = "Attack")
	float AttackRange = 200.0f;
	
	// 伤害值
	UPROPERTY(EditAnywhere, Category = "Attack")
	float Damage = 10.0f;
	
	// 攻击冷却时间
	UPROPERTY(EditAnywhere, Category = "Attack")
	float AttackCooldown = 1.0f;
	
	// 上次攻击时间
	float LastAttackTime;
	
	// 移动速度
	UPROPERTY(EditAnywhere, Category = "Attack")
	float MoveSpeed = 400.0f;
};
