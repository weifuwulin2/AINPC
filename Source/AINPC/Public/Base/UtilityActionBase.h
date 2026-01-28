#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "Curves/CurveFloat.h"
#include "UtilityAI/UNPCMentalState.h" // 确保路径引用正确
#include "UtilityAI/MentalStateFields.h"  // ✅ 引入字段配置
#include "Social/SocialTypes.h"           // ✅ 引入 EOCEANTrait 定义
#include "UtilityActionBase.generated.h"

class UUtilityActionBase;
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
	
	// 生理层 (Physiological)
	Hunger,
	Fatigue,
	
	// 安全层 (Safety)
	PerceivedThreat,      // 对应 Perceived_Threat
	
	// 社交层 (Belonging)
	Loneliness,
	
	// 尊严层 (Esteem)
	Indignity,
	
	// 自我实现层 (Self-Actualization)
	Boredom,
	
	// === 环境感知字段 (不在 MentalState 中) ===
	// Environmental Perception Fields (not in MentalState)
	SelfHealth,           // 生理：自身血量百分比
	TargetHealth,         // 环境：目标血量百分比
	DistanceToTarget,     // 环境：与目标的距离 (归一化)
	AmmoCount,            // 环境：弹药量百分比
	HasCover,             // 环境：附近是否有掩体 (0 或 1)
	IsTargetPlayer,       // 环境：目标是否是玩家
	HasAttackTarget,      // 环境：是否有攻击目标 (FocusActor 是否存在)
	HasEnemyNearby,       // 环境：附近是否有敌人 (不依赖 FocusActor)
	HasFriendlyNearby,    // 环境：附近是否有友军/中立单位 (非 Enemy, 非 Self)
	HasFoodNearby,        // 环境：附近是否有食物 (Activity.Eat)
	HasBedNearby,         // 环境：附近是否有床 (Activity.Rest)
	GoalDirectiveMatch    // 目标系统：当前指令是否匹配 (GoalComponent.CurrentDirective)
};

// 考量类型：动机 vs 必要条件
// Consideration Type: Motivation vs Context
UENUM(BlueprintType)
enum class EConsiderationType : uint8
{
	Motivation,  // 动机：使用加法求和 Σ(Weight × Input)
	Context      // 必要条件：使用乘法 ∏(Context)
};

// 预设的响应曲线类型
// Preset response curve types
UENUM(BlueprintType)
enum class EUtilityCurveType : uint8
{
	Linear,           // 线性 (y = x)
	Quadratic,        // 平方 (y = x * x) - 只有很高时才有用
	InverseQuadratic, // 反平方 (y = 1 - (1-x)^2) - 一点点就很有用
	Logistic,         // S形 (Sigmoid) - 只有中间变化大
	Step,             // 阶梯 (x > 0.5 ? 1 : 0)
    TargetThreshold,  // 目标阈值 (x > Threshold ? 1 : 0) (New)
    Inverse           // 反向 (y = 1 - x) - 用于反向 Context，例如"没有敌人时才能聊天"
};

// =========================================================
// Action Priority Levels (for Transition System)
// =========================================================
UENUM(BlueprintType)
enum class EActionPriority : uint8
{
    None = 0,
    Idle = 10,       // Can always be interrupted (Wander, Patrol)
    Ambient = 20,    // Random acts (Looking around)
    Work = 30,       // Job tasks (Mining, Crafting)
    Social = 35,     // Dialogue/Interaction
    Needs = 40,      // Eating/Sleeping - High Inertia
    Threat = 50,     // Combat/Fleeing - Hard to interrupt
    Critical = 60    // Death/Stun - Uninterruptible
};

// =========================================================
// Comparison Operators (for Exit Conditions)
// =========================================================
UENUM(BlueprintType)
enum class EComparisonOperator : uint8
{
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    Equals
};

// =========================================================
// Data-Driven Exit Condition
// =========================================================
USTRUCT(BlueprintType)
struct FActionExitCondition
{
    GENERATED_BODY()
    
    // Variable to check (e.g., Hunger, Fatigue)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EUtilityInputType Variable = EUtilityInputType::Hunger;
    
    // Comparison operator (e.g., LessThan)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EComparisonOperator Operator = EComparisonOperator::LessThan;
    
    // Threshold value (e.g., 0.2)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Threshold = 0.2f;

    // If true, only check this condition after ActionDuration has passed
    // Useful for "Eat until full, but at least chew for 5 seconds"
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bWaitForDuration = false;
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

    // 曲线类型：选择预设的数学曲线，避免必须创建 CurveAsset
    // Curve Type: Select preset math curve to avoid creating CurveAssets
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Response Curve")
    EUtilityCurveType CurveType = EUtilityCurveType::Linear;

    // (可选) 自定义响应曲线：如果设置了此项，将覆盖 CurveType (高级用法)
    // (Optional) Custom Response Curve: If set, overrides CurveType (Advanced usage)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Response Curve")
    UCurveFloat* ResponseCurve;
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

    // 智能对象标签：指定该动作针对哪种类型的智能对象
    // Smart Object Tag: Specifies which type of smart object this action targets
    // Example: "Interaction.Eat", "Interaction.Rest" (Fixed comment)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smart Object", meta = (Categories = "Interaction"))
    FGameplayTag SmartObjectTag;

    // === 动画配置 (Animation Configuration) ===
    
    // 交互动画 Montage (例如：吃饭、睡觉动画)
    // Interaction animation montage (e.g., eating, sleeping animation)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    UAnimMontage* InteractionMontage = nullptr;

    // 是否循环播放动画 / Whether to loop the animation
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bLoopAnimation = true;

    // 动作持续时长（秒）/ Action duration in seconds
    // 0 = 无限持续，直到 Utility AI 切换到其他动作
    // 0 = Infinite duration, until Utility AI switches to another action
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    float ActionDuration = 0.0f;

    // === Emotion Matrix 配置 ===

    // Interaction Tag: Used for Emotion Matrix lookup
    // Example: "Interaction.Combat", "Interaction.Flee", "Interaction.Social"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emotion Matrix",
              meta = (DisplayName = "Activity Tag for Emotion Matrix", Categories = "Interaction"))
    FGameplayTag ActivityTag;

    // LLM Intention Tag
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LLM Integration",
              meta = (DisplayName = "Matching Intention Tag", Categories = "Intention"))
    FGameplayTag IntentionTag;

	// 指令标签：用于 GoalComponent 仲裁匹配
	// Directive Tag: Used for GoalComponent arbitration matching
	// Example: "Directive.Work", "Directive.Survival"
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal Integration",
			  meta = (DisplayName = "Required Directive Tag", Categories = "Directive"))
	FGameplayTag DirectiveTag;
    
    UPROPERTY(EditDefaultsOnly, Category = "Action Config")
    FString ActionName;

	// 职业过滤：如果非空，则只有匹配这些 Profession ID 的 NPC 才能拥有此动作
	// Profession Filter: If not empty, only NPCs with these Profession IDs can possess this action
	// Example: ["Miner", "Slave", "Laborer"]
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Filtering")
	TArray<FName> RequiredProfessionIDs;

	// 性格修正系数 (Data-Driven PAM)
	// Key: OCEAN 特质 / OCEAN Trait
	// Value: 影响系数 (+1.0 正相关, -1.0 负相关) / Influence Factor (+1.0 positive correlation, -1.0 negative)
	// Example: Attack { Neuroticism: -1.0 } -> Low N (Brave) gets bonus
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	TMap<EOCEANTrait, float> PersonalityInfluence;

    // =========================================================
    // Transition System Configuration
    // =========================================================
    
    // Action Priority Level (determines interrupt rules)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition",
              meta = (DisplayName = "Priority Level"))
    EActionPriority Priority = EActionPriority::Idle;

    // Commitment Time: After Enter(), this action cannot be interrupted by lower-priority actions for X seconds
    // (Distinct from Cooldown, which is post-Exit lockout)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition",
              meta = (DisplayName = "Commitment Time (seconds)"))
    float CommitmentTime = 0.0f;

    // Data-Driven Exit Conditions (replaces hardcoded ShouldExit logic)
    // Example: Eat exits when Hunger < 0.2
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition",
              meta = (DisplayName = "Exit Conditions"))
    TArray<FActionExitCondition> ExitConditions;
};

// =========================================================
// 2. Action 基类定义
// =========================================================

UCLASS(Abstract, Blueprintable, BlueprintType)
class AINPC_API UUtilityActionBase : public UObject
{
    GENERATED_BODY()

public:
    // Game-wide Constants for Scoring
    static const float IntentionMatchBonus;        // Default: 0.3f
    static const float DirectiveMatchMultiplier;   // Default: 1.5f
    static const float DirectiveMismatchMultiplier; // Default: 0.5f

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

    UPROPERTY(Transient)
    FGameplayTag SmartObjectTag;

    // Activity Tag: 用于 Emotion Matrix 查表
    UPROPERTY(Transient)
    FGameplayTag ActivityTag;

    // LLM Intention Tag
    UPROPERTY(Transient)
    FGameplayTag IntentionTag;

	// Goal Directive Tag
	UPROPERTY(Transient)
	FGameplayTag DirectiveTag;

    // Personality Influence Map (Data-Driven PAM)
    UPROPERTY(Transient)
    TMap<EOCEANTrait, float> PersonalityInfluence;

    UPROPERTY(Transient)
    FString ActionName;

    // =========================================================
    // Transition System Runtime Fields
    // =========================================================
    
    UPROPERTY(Transient)
    EActionPriority Priority = EActionPriority::Idle;

    UPROPERTY(Transient)
    float CommitmentTime = 0.0f;

    UPROPERTY(Transient)
    TArray<FActionExitCondition> ExitConditions;

    // Runtime: When did the Commitment period start?
    float CommitmentStartTime = 0.0f;

    // Runtime: Last calculated score (for inertia comparison)
    float LastScore = 0.0f;

    // --- 运行时状态 ---
    // 上次成功执行的时间 (用于计算冷却)
    float LastExecutedTime = -9999.0f;

    // Action start time (for duration tracking)
    float ActionStartTime = 0.0f;

    // --- 核心函数 ---

    // 初始化：从配置表填充数据
    void InitFromConfig(const FUtilityActionConfig& Config);

    // 计算分数：核心逻辑 (包含冷却、惯性、多因子乘法)
    virtual float CalculateScore(UNPCMentalState* MentalState, AAIController* Controller, bool bLogDebug = false);

    // 标记执行：当动作被选中并执行时调用，用于刷新冷却时间
    void MarkExecutionTime(float CurrentTime);

    // =========================================================
    // Transition System Helpers
    // =========================================================
    
    // Is this action currently in its Commitment period?
    bool IsCommitted(float CurrentTime) const;

    // Check data-driven exit conditions (returns true if should exit)
    bool CheckExitConditions(UNPCMentalState* MentalState, AAIController* Controller) const;

protected:
    // 辅助函数：统一获取输入值
    float GetConsiderationValue(EUtilityInputType InputType, UNPCMentalState* State, AAIController* Controller, bool bLogDebug = false);

    // 辅助函数：将枚举值转换为变量名字符串
    // Helper function: Convert enum value to variable name string
    FString GetVariableNameFromInputType(EUtilityInputType InputType) const;


public:
    // --- 动作执行生命周期 (蓝图可重写) ---
    
    UFUNCTION(BlueprintNativeEvent)
    void Enter(AAIController* Controller);
    virtual void Enter_Implementation(AAIController* Controller);
    
    UFUNCTION(BlueprintNativeEvent)
    void Execute(AAIController* Controller);
    virtual void Execute_Implementation(AAIController* Controller) {}
    
    UFUNCTION(BlueprintNativeEvent)
    void Exit(AAIController* Controller);
    virtual void Exit_Implementation(AAIController* Controller) {}

    // ✅ 检查动作是否应该退出（允许切换到其他动作）
    // Check if action should exit (allow switching to other actions)
    // 默认返回 true（允许切换），SmartObject 等有持续时长的动作会返回 false 阻止切换
    // Default returns true (allow switching), SmartObject with Duration returns false to block
    UFUNCTION(BlueprintCallable, Category = "Utility Action")
    virtual bool ShouldExit(AAIController* Controller) const;
};