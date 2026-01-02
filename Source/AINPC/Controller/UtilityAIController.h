// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
// 引入 HTTP 头文件
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "LLM/LLMCommunicator.h"
#include "UtilityAIController.generated.h"


class UUtilityActionBase;
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
	UPROPERTY(EditDefaultsOnly, Category = "AI Config")
	UDataTable* ActionDataTable;

	// 运行时实例化的动作列表
	UPROPERTY()
	TArray<UUtilityActionBase*> AvailableActions;

	// 当前正在执行的动作
	UPROPERTY()
	UUtilityActionBase* CurrentAction;

	// === 核心逻辑 ===
	void EvaluateUtilityLogic();

	// 1. 你的“黑板” (AI 的大脑状态)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	FMentalState CurrentMentalState;

	// 2. 对外接口：让 AI 更新大脑
	UFUNCTION(BlueprintCallable, Category = "AI")
	void UpdateMind(FString SituationDescription);


private:
	// 3. 你的秘书 (通信器实例)
	UPROPERTY()
	ULLMCommunicator* LLMService;

	// 4. 回调函数：这就是你用来“接收具体数值”的地方！
	// 签名必须匹配 FOnLLMResponse: (bool, const FMentalState&)
	void OnMindUpdated(bool bSuccess, const FMentalState& NewState);
};
