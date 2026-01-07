#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Curves/CurveFloat.h"
#include "UtilityAI/UNPCMentalState.h" // 确保路径引用正确
#include "UtilityAI/MentalStateFields.h"  // ✅ 引入字段配置
#include "UtilityActionBase.generated.h"

// 前置声明
class AAIController;
class UNPCMentalState;

// =========================================================
// 1. 枚举与配置结构 (自动生成)
// =========================================================


UENUM(BlueprintType)
enum class EUtilityInputType : uint8
{
	// === 马斯洛需求层次 (来自 MentalState) ===
	// Maslow's Hierarchy of Needs (from MentalState)
	// 注意：枚举值使用驼峰命名（无下划线），但对应的字段名可能有下划线
	
	// 生理层 (Physiological)
	Hunger,
	Energy,
	
	// 安全层 (Safety)
	PerceivedThreat,      // 对应 Perceived_Threat
	ResourceAnxiety,      // 对应 Resource_Anxiety
	
	// 社交层 (Love/Belonging)
	Loneliness,
	Trust,
	
	// 尊严层 (Esteem)
	Anger,
	SocialStatus,         // 对应 Social_Status
	
	// 自我实现层 (Self-Actualization)
	DutyUrgency,          // 对应 Duty_Urgency
	Curiosity,
	
	// === 环境感知字段 (不在 MentalState 中) ===
	// Environmental Perception Fields (not in MentalState)
	SelfHealth,           // 生理：自身血量百分比
	TargetHealth,         // 环境：目标血量百分比
	DistanceToTarget,     // 环境：与目标的距离 (归一化)
	AmmoCount,            // 环境：弹药量百分比
	HasCover,             // 环境：附近是否有掩体 (0 或 1)
	IsTargetPlayer        // 环境：目标是否是玩家
};

// 考量类型：动机 vs 必要条件
// Consideration Type: Motivation vs Context
UENUM(BlueprintType)
enum class EConsiderationType : uint8
{
	Motivation,  // 动机：使用加法求和 Σ(Weight × Input)
	Context      // 必要条件：使用乘法 ∏(Context)
};

USTRUCT(BlueprintType)
struct FUtilityConsideration
{
    GENERATED_BODY()

    // 考量类型：动机还是必要条件？
    // Consideration Type: Motivation or Context?
    // 
    // Motivation: 加法求和，表示"想做这件事的动机强度"
    // Context: 乘法，表示"能否做这件事的必要条件"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consideration",
              meta = (DisplayName = "Consideration Type",
                      ToolTip = "Motivation=动机(加法), Context=必要条件(乘法)"))
    EConsiderationType ConsiderationType = EConsiderationType::Motivation;

    // 输入源：我们要考量什么变量？
    // Input Source: Which variable should we consider?
    // 
    // 权重由 PersonalityComponent 提供（仅对 Motivation 有效）
    // Weights are provided by PersonalityComponent (only for Motivation)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consideration",
              meta = (DisplayName = "Variable to Consider",
                      ToolTip = "选择要考量的马斯洛变量或环境变量 / Select Maslow or environment variable to consider"))
    EUtilityInputType InputType;
};

USTRUCT(BlueprintType)
struct FUtilityActionConfig : public FTableRowBase
{
    GENERATED_BODY()
    
    // 逻辑类：具体的行为逻辑 (比如 BP_Attack)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<UUtilityActionBase> ActionClass;

    // 考量列表：多因子判断核心
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FUtilityConsideration> Considerations; 

    // 基础奖励：这个动作本身的价值
    // Base Reward: The intrinsic value of this action
    // 例如：吃饭的饱腹感、攻击的伤害、逃跑的安全感
    UPROPERTY(EditAnywhere, BlueprintReadOnly, 
              meta = (DisplayName = "Base Reward",
                      ToolTip = "这个动作的基础奖励值 / Base reward value of this action"))
    float BaseReward = 1.0f;

    // 冷却时间 (秒)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float CooldownTime = 0.0f;
    
    // 惯性奖励：如果正在做这个动作，额外加多少分？
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float InertiaBonus = 0.0f;
};

// =========================================================
// 2. Action 基类定义
// =========================================================

UCLASS(Abstract, Blueprintable, BlueprintType)
class AINPC_API UUtilityActionBase : public UObject
{
    GENERATED_BODY()

public:
    UUtilityActionBase();

    // --- 配置数据 (由 Controller 注入) ---
    UPROPERTY(Transient)
    TArray<FUtilityConsideration> Considerations;

    UPROPERTY(Transient)
    float BaseReward = 1.0f;

    UPROPERTY(Transient)
    float CooldownTime = 0.0f;

    UPROPERTY(Transient)
    float InertiaBonus = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Action Config")
    FString ActionName;

    // --- 运行时状态 ---
    // 上次成功执行的时间 (用于计算冷却)
    float LastExecutedTime = -9999.0f;

    // --- 核心函数 ---

    // 初始化：从配置表填充数据
    void InitFromConfig(const FUtilityActionConfig& Config);

    // 计算分数：核心逻辑 (包含冷却、惯性、多因子乘法)
    virtual float CalculateScore(UNPCMentalState* MentalState, AAIController* Controller);

    // 标记执行：当动作被选中并执行时调用，用于刷新冷却时间
    void MarkExecutionTime(float CurrentTime);

protected:
    // 辅助函数：统一获取输入值
    float GetConsiderationValue(EUtilityInputType InputType, UNPCMentalState* State, AAIController* Controller);

    // 辅助函数：将枚举值转换为变量名字符串
    // Helper function: Convert enum value to variable name string
    FString GetVariableNameFromInputType(EUtilityInputType InputType) const;


public:
    // --- 动作执行生命周期 (蓝图可重写) ---
    
    UFUNCTION(BlueprintNativeEvent)
    void Enter(AAIController* Controller);
    virtual void Enter_Implementation(AAIController* Controller) {}
    
    UFUNCTION(BlueprintNativeEvent)
    void Execute(AAIController* Controller);
    virtual void Execute_Implementation(AAIController* Controller) {}
    
    UFUNCTION(BlueprintNativeEvent)
    void Exit(AAIController* Controller);
    virtual void Exit_Implementation(AAIController* Controller) {}
};