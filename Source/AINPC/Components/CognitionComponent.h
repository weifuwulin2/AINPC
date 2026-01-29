// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MemoryComponent.h"
#include "Components/ActorComponent.h"
#include "AI/ContextLODTypes.h"
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
	virtual void BeginDestroy() override; // ✅ P3 Fix: Timer cleanup

	// --- 外部接口 ---
    
	// Set Context LOD (Called by GoalComponent)
	UFUNCTION(BlueprintCallable, Category = "AI | Cognition")
	void SetLOD(EContextLOD NewLOD);

	UFUNCTION(BlueprintPure, Category = "AI | Cognition")
	EContextLOD GetCurrentLOD() const { return CurrentLOD; }

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

	/** 
	 * Whether this brain uses LLM reasoning.
	 * Disable for "Mindless" NPCs (Zombies, Animals) to save performance and cost.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Cognition")
	bool bEnableReasoning = true;

	// 心理状态插值器 / Mental State Interpolator
	// 平滑过渡情绪变化
	UPROPERTY()
	UMentalStateInterpolator* Interpolator;

	// --- Decision Context ---
	// Updates the Cognition layer with the current Utility AI conflict status
	UFUNCTION(BlueprintCallable, Category = "AI | Cognition")
	void ReportDecisionContext(const FString& WinnerName, const FString& RunnerUpName, float ConflictLevel);

	// Context string describing the last significant decision conflict
	UPROPERTY(VisibleAnywhere, Category = "AI | Cognition")
	FString CurrentDecisionContext;

protected:
	UPROPERTY(VisibleAnywhere, Category = "AI | Cognition")
	EContextLOD CurrentLOD = EContextLOD::Standard;

	// 内部持有的 LLM 服务
	UPROPERTY()
	ULLMCommunicator* LLMService;

	UPROPERTY()
	UMemoryComponent* MemoryComp;

	// 语义映射器 / Sentiment Mapper
	// 将 LLM 输出的标签转换为数值
	UPROPERTY()
	USentimentMapper* SentimentMapper;

	
	// 内部回调：处理 LLMCommunicator 返回的原始结果
	void OnLLMReply(bool bSuccess, const FMentalState& NewState);

private:
	// 专门的回调：处理总结结果
	void OnDreamingAnalysisComplete(bool bSuccess, const FString& RawResponse);

	// 重试定时器 / Retry timer
	FTimerHandle RetryStimulusTimerHandle;
	FString PendingStimulus;
	
	// ✅ Rate Limiting
	float LastLLMRequestTime = -999.0f;
	float MinRequestInterval = 2.0f; // Minimum seconds between LLM calls

	// --- Refactor Helpers ---
	bool CheckAmygdalaHijack(const FString& SituationDescription);
	bool IsDataReady(const FString& PersonalityID, const FString& FactionStr, const FString& SituationDescription);
	
	FString BuildIdentityBlock(const FString& RoleDesc, const FString& PersonalityID, const FString& FactionStr);
	FString BuildWorldviewBlock(const FString& FactionStr);
	FString BuildContextBlock(const FString& ProfessionName, const FString& ProfessionDesc);
	FString BuildVolatileBlock(const FString& Situation, const FString& Memories, const FString& GlobalHistory);
};
