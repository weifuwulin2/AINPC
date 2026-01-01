// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "UtilityAIController.generated.h"

class UNPCMentalState;
/**
 * 
 */
UCLASS()
class AINPC_API AUtilityAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AUtilityAIController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// === 数据层 ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UNPCMentalState* MentalState;

	// === 动作层 ===
    
	// 动作配置表：在蓝图里把 Action_Attack 填进去
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TArray<TSubclassOf<UUtilityActionBase>> ActionClasses;

	// 运行时实例化的动作列表
	UPROPERTY()
	TArray<UUtilityActionBase*> AvailableActions;

	// 当前正在执行的动作
	UPROPERTY()
	UUtilityActionBase* CurrentAction;

	// === 核心逻辑 ===
	void EvaluateUtilityLogic();
};
