// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
// 引入 HTTP 头文件
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "LLM/LLMCommunicator.h"
#include "Perception/AIPerceptionTypes.h"
#include "UtilityAIController.generated.h"


class UAISenseConfig_Sight;
class UCognitionComponent;
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

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- 核心数据 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UNPCMentalState* MentalState;

	// --- 组件 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UCognitionComponent* CognitionComp;

	// --- 行为配置 ---
	UPROPERTY(EditDefaultsOnly, Category = "AI Config")
	UDataTable* ActionDataTable;

	// --- 接口 ---
	UFUNCTION(BlueprintCallable, Category = "AI")
	void UpdateMind(FString SituationDescription);

protected:
	// 内部函数：评估所有行为得分
	void EvaluateUtilityLogic();

	// 回调函数：接收来自 CognitionComponent 的数据
	UFUNCTION()
	void OnMindUpdated(const FMentalState& NewState);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* AIPerception;

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	// 感知更新的回调函数
	UFUNCTION()
	void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

private:
	UPROPERTY()
	TArray<UUtilityActionBase*> AvailableActions;

	UPROPERTY()
	UUtilityActionBase* CurrentAction;
};
