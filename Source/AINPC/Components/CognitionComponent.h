// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MemoryComponent.h"
#include "Components/ActorComponent.h"
#include "CognitionComponent.generated.h"

class ULLMCommunicator;
class USentimentMapper;
class UMentalStateInterpolator;

// 定义广播委托：当大脑想明白后，通知订阅者（Controller）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMentalStateChanged, const FMentalState&, NewState);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API UCognitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:    
	UCognitionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- 外部接口 ---
    
	// 唯一的输入口：接收外界刺激（"我看见了僵尸"）
	UFUNCTION(BlueprintCallable, Category = "AI | Cognition")
	void ProcessStimulus(FString SituationDescription);

	// 委托：Controller 绑定这个来获取结果
	UPROPERTY(BlueprintAssignable, Category = "AI | Cognition")
	FOnMentalStateChanged OnMentalStateChanged;

	UFUNCTION(BlueprintCallable)
	void StartDreaming();
	
	// 角色描述 / Role Description
	// 定义此 NPC 的身份和基本行为倾向，用于 LLM 上下文
	// Defines this NPC's identity and basic behavioral tendencies for LLM context
	// Example: "You are a hostile zombie that attacks humans on sight"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Cognition")
	FString RoleDescription = TEXT("You are a neutral NPC");
	
	// 行为准则 / Behavioral Guidelines
	// 定义此 NPC 的具体行为规则和反应模式
	// Defines this NPC's specific behavioral rules and reaction patterns
	// Example: "Attack any human on sight. Flee if health is below 30%."
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Cognition")
	FString BehavioralGuidelines = TEXT("");
	
protected:
	// 内部持有的 LLM 服务
	UPROPERTY()
	ULLMCommunicator* LLMService;

	UPROPERTY()
	UMemoryComponent* MemoryComp;

	// 语义映射器 / Sentiment Mapper
	// 将 LLM 输出的标签转换为数值
	UPROPERTY()
	USentimentMapper* SentimentMapper;

	// 心理状态插值器 / Mental State Interpolator
	// 平滑过渡情绪变化
	UPROPERTY()
	UMentalStateInterpolator* Interpolator;

	// 内部回调：处理 LLMCommunicator 返回的原始结果
	void OnLLMReply(bool bSuccess, const FMentalState& NewState);

private:
	// 专门的回调：处理总结结果
	void OnDreamingAnalysisComplete(bool bSuccess, const FString& RawResponse);
};
