// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MemoryComponent.h"
#include "Components/ActorComponent.h"
#include "CognitionComponent.generated.h"
class ULLMCommunicator;
// 定义广播委托：当大脑想明白后，通知订阅者（Controller）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMentalStateChanged, const FMentalState&, NewState);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API UCognitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:    
	UCognitionComponent();

	virtual void BeginPlay() override;

	// --- 外部接口 ---
    
	// 唯一的输入口：接收外界刺激（"我看见了僵尸"）
	UFUNCTION(BlueprintCallable, Category = "AI | Cognition")
	void ProcessStimulus(FString SituationDescription);

	// 委托：Controller 绑定这个来获取结果
	UPROPERTY(BlueprintAssignable, Category = "AI | Cognition")
	FOnMentalStateChanged OnMentalStateChanged;

	UFUNCTION(BlueprintCallable)
	void StartDreaming();
	
protected:
	// 内部持有的 LLM 服务
	UPROPERTY()
	ULLMCommunicator* LLMService;

	UPROPERTY()
	UMemoryComponent* MemoryComp;

	// 内部回调：处理 LLMCommunicator 返回的原始结果
	void OnLLMReply(bool bSuccess, const FMentalState& NewState);

private:
	// 专门的回调：处理总结结果
	void OnDreamingAnalysisComplete(bool bSuccess, const FString& RawResponse);
};
