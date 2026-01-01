// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
// 引入 HTTP 头文件
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "UtilityAIController.generated.h"


class UUtilityActionBase;
// 定义一个结构体来匹配 JSON 格式
USTRUCT()
struct FMentalStateJson
{
	GENERATED_BODY()

	UPROPERTY()
	float Anger = 0.0f;

	UPROPERTY()
	float Fear = 0.0f;

	UPROPERTY()
	float Confidence = 0.0f;

	UPROPERTY()
	float SocialBattery = 0.0f;

	UPROPERTY()
	float Hunger = 0.0f;
};
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
	
	// 触发 HTTP 请求
	UFUNCTION(BlueprintCallable, Category = "AI|Brain")
	void RequestUpdateFromLLM(FString Prompt);

private:
	// 回调函数
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
