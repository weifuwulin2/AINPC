#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Curves/CurveFloat.h"
#include "UtilityAI/UNPCMentalState.h" // 确保路径引用正确
#include "UtilityActionBase.generated.h"

// 前置声明
class AAIController;
class UNPCMentalState;

// =========================================================
// 1. 枚举与配置结构
// =========================================================

UENUM(BlueprintType)
enum class EUtilityInputType : uint8
{
    Anger,           // 情绪：愤怒
    Fear,            // 情绪：恐惧
    Confidence,      // 情绪：自信
    SocialBattery,   // 情绪：社交电量
    Hunger,          // 生理：饥饿
    SelfHealth,      // 生理：自身血量百分比
    TargetHealth,    // 环境：目标血量百分比
    DistanceToTarget,// 环境：与目标的距离 (归一化)
    AmmoCount,       // 环境：弹药量百分比
    HasCover,        // 环境：附近是否有掩体 (0 或 1)
    IsTargetPlayer   // 环境：目标是否是玩家
};

USTRUCT(BlueprintType)
struct FUtilityConsideration
{
    GENERATED_BODY()

    // 输入源：我们要考量什么？
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EUtilityInputType InputType;

    // 反应曲线：数值如何映射为分数？(0~1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
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

    // 基础权重：全局优先级修正
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Weight = 1.0f;

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
    float BaseWeight = 1.0f;

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