#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h" 
#include "Social/SocialTypes.h" // Add this
#include "UtilityAI/EmotionTypes.h" // Add this
#include "UtilityAIController.generated.h"

class UMetabolismComponent;
// 前置声明 (Forward Declarations)
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class USensoryComponent;
class UMemoryComponent; // Add this
class UCognitionComponent;
class UUtilityAIComponent;
class UPersonalityComponent;
class UEmotionDisplayComponent;
class UNPCMentalState;
struct FMentalState; 

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

    // The Nervous System (Sensory)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    USensoryComponent* SensoryComp;

    // The Brain (Memory)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UMemoryComponent* MemoryComp;

    // 认知组件 (负责处理情绪)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UCognitionComponent* CognitionComp;

    // 决策组件 (负责 Utility 算分和执行)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UUtilityAIComponent* UtilityComp;

    // 性格组件 (负责 OCEAN 性格和马斯洛权重)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UPersonalityComponent* PersonalityComp;
    
    // 情绪显示组件 (负责显示 emoji 和对话泡泡)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UEmotionDisplayComponent* EmotionDisplayComp;

    // 新陈代谢组件 / Metabolism Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UMetabolismComponent* MetabolismComp;

    // 目标仲裁组件 (Goal/LOD)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    class UGoalComponent* GoalComp;

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

    // 当前情绪状态 (Cached)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Data")
    EEmotionState CurrentEmotion = EEmotionState::Neutral;

    // 情绪矩阵配置表 (Emotion Matrix)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Configuration")
    UDataTable* EmotionMatrixTable;

    // =========================================================
    // 4. 对外接口 (External Interface)
    // =========================================================

    // 供聊天系统或剧情系统调用 (Deprecated? Replaced by Sensory?)
    UFUNCTION(BlueprintCallable, Category = "AI Communication")
    void ReceiveSpeech(AActor* Speaker, FString Message);

protected:
    virtual void BeginPlay() override;

    // =========================================================
    // 5. 内部回调 (Callbacks)
    // =========================================================

    // Callback: Connects Sensory Output (Nerves) to Memory Input (Brain)
    UFUNCTION()
    void OnSemanticEventReceived(const FSemanticEvent& Event);

    // 回调：当 Sensory 组件翻译完信号后，中转给认知组件 (Legacy?)
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