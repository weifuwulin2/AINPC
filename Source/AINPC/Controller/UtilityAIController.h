#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h" 
#include "UtilityAIController.generated.h"

// 前置声明 (Forward Declarations)
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class USensoryComponent;
class UCognitionComponent;
class UUtilityAIComponent;
class UNPCMentalState;
struct FMentalState; // 假设这是你的结构体

UCLASS()
class AINPC_API AUtilityAIController : public AAIController
{
    GENERATED_BODY()

public:
    AUtilityAIController();

    // =========================================================
    // 1. 组件声明 (Components)
    // =========================================================
    
    // 物理感知组件 (引擎自带，负责看和听)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UAIPerceptionComponent* AIPerception;

    // 感官翻译组件 (负责把物理信号翻译成文字)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    USensoryComponent* SensoryComp;

    // 认知组件 (负责处理情绪和记忆)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UCognitionComponent* CognitionComp;

    // 决策组件 (负责 Utility 算分和执行)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UUtilityAIComponent* UtilityComp;

    // =========================================================
    // 2. 感知配置 (Config)
    // =========================================================
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Configuration")
    UAISenseConfig_Sight* SightConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Configuration")
    UAISenseConfig_Hearing* HearingConfig;

    // =========================================================
    // 3. 共享数据 (Shared Data)
    // =========================================================
    
    // 情绪状态对象 (Blackboard) - Utility组件会读取这里
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Data")
    UNPCMentalState* MentalState;

    // =========================================================
    // 4. 对外接口 (External Interface)
    // =========================================================

    // 供聊天系统或剧情系统调用
    UFUNCTION(BlueprintCallable, Category = "AI Communication")
    void ReceiveSpeech(AActor* Speaker, FString Message);

protected:
    virtual void BeginPlay() override;

    // =========================================================
    // 5. 内部回调 (Callbacks)
    // =========================================================

    // 回调：当 Sensory 组件翻译完信号后，中转给认知组件
    UFUNCTION()
    void RelaySensoryToCognition(const FString& StimulusDescription);

    // 回调：当 Cognition 组件更新了情绪后，同步到本地 MentalState
    UFUNCTION()
    void OnMindUpdated(const FMentalState& NewState);

private:
    // =========================================================
    // 6. 定时器 (Timers)
    // =========================================================
    
    // Dreaming 定时器：定期整理记忆，提取长期洞察
    FTimerHandle DreamingTimerHandle;
};