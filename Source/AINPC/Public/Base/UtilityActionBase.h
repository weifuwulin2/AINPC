#pragma once

#include "CoreMinimal.h"
#include "UtilityActionBase.generated.h"

class UNPCMentalState;
class AAIController;
/**
 * 
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class AINPC_API UUtilityActionBase : public UObject
{
	GENERATED_BODY()

public:
	// 动作名称，方便调试
	UPROPERTY(EditDefaultsOnly, Category = "Action Config")
	FString ActionName;

	// === 核心虚函数 (Virtual Functions) ===

	// 1. 打分函数：返回 0.0 - 1.0 的分数
	// 传入 MentalState 指针，让动作根据情绪计算分数
	virtual float CalculateScore(const UNPCMentalState* State);

	// 2. 执行函数：真正干活的地方
	virtual void Execute(AAIController* Controller);

	// 3. 退出函数：切换动作时清理现场（比如停止动画）
	virtual void Exit(AAIController* Controller);
};
