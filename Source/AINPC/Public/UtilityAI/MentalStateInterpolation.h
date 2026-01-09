// MentalStateInterpolation.h
// 心理状态平滑插值 / Mental State Smooth Interpolation
// 
// 将 LLM 输出的目标值平滑过渡到当前值
// Smoothly transitions LLM output target values to current values

#pragma once

#include "CoreMinimal.h"
#include "UtilityAI/UNPCMentalState.h"
#include "MentalStateInterpolation.generated.h"

/**
 * 插值配置 / Interpolation Configuration
 * 
 * 控制不同情绪的变化速度
 * Controls the change speed of different emotions
 */
USTRUCT(BlueprintType)
struct FMentalStateInterpConfig
{
    GENERATED_BODY()

    // 插值速度 / Interpolation Speed
    // 数值越大，变化越快
    // Higher value = faster change
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interpolation",
              meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float InterpSpeed = 2.0f;

    // 最小变化阈值 / Minimum Change Threshold
    // 如果当前值和目标值的差距小于这个值，直接设置为目标值
    // If difference < threshold, directly set to target value
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interpolation",
              meta = (ClampMin = "0.001", ClampMax = "0.1"))
    float SnapThreshold = 0.01f;

    // 是否启用随机扰动 / Enable Random Perturbation
    // 让每个 NPC 的情绪变化略有不同
    // Makes each NPC's emotion change slightly different
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interpolation")
    bool bEnableRandomPerturbation = true;

    // 随机扰动范围 / Random Perturbation Range
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interpolation",
              meta = (ClampMin = "0.0", ClampMax = "0.2"))
    float PerturbationRange = 0.05f;
};

/**
 * 心理状态插值器 / Mental State Interpolator
 * 
 * 负责平滑过渡 LLM 输出的目标值
 * Responsible for smoothly transitioning LLM output target values
 */
UCLASS()
class AINPC_API UMentalStateInterpolator : public UObject
{
    GENERATED_BODY()

public:
    UMentalStateInterpolator();

    /**
     * 设置目标值 / Set Target Value
     * 
     * LLM 返回新的情绪值时调用
     * Called when LLM returns new emotion value
     * 
     * @param VariableName - 变量名 (如 "Anger")
     * @param TargetValue - 目标值 (0.0 - 1.0)
     */
    UFUNCTION(BlueprintCallable, Category = "Mental State Interpolation")
    void SetTargetValue(const FString& VariableName, float TargetValue);

    /**
     * 更新插值 / Update Interpolation
     * 
     * 在 Tick 中调用，平滑过渡当前值到目标值
     * Called in Tick to smoothly transition current value to target value
     * 
     * @param MentalState - 要更新的心理状态
     * @param DeltaTime - 帧时间
     */
    UFUNCTION(BlueprintCallable, Category = "Mental State Interpolation")
    void UpdateInterpolation(UNPCMentalState* MentalState, float DeltaTime);

    /**
     * 设置插值配置 / Set Interpolation Configuration
     * 
     * @param VariableName - 变量名
     * @param Config - 插值配置
     */
    UFUNCTION(BlueprintCallable, Category = "Mental State Interpolation")
    void SetInterpConfig(const FString& VariableName, const FMentalStateInterpConfig& Config);

    /**
     * 获取插值配置 / Get Interpolation Configuration
     * 
     * @param VariableName - 变量名
     * @return 插值配置
     */
    UFUNCTION(BlueprintCallable, Category = "Mental State Interpolation")
    FMentalStateInterpConfig GetInterpConfig(const FString& VariableName) const;

    /**
     * 立即设置值（跳过插值）/ Immediately Set Value (Skip Interpolation)
     * 
     * 用于需要瞬间变化的情况（如被攻击时的愤怒）
     * Used for instant changes (e.g., anger when attacked)
     * 
     * @param MentalState - 心理状态
     * @param VariableName - 变量名
     * @param Value - 值
     */
    UFUNCTION(BlueprintCallable, Category = "Mental State Interpolation")
    void SetValueImmediate(UNPCMentalState* MentalState, const FString& VariableName, float Value);

    /**
     * 重置所有目标值 / Reset All Target Values
     */
    UFUNCTION(BlueprintCallable, Category = "Mental State Interpolation")
    void ResetAllTargets();

    /**
     * 获取目标值 / Get Target Value
     * 
     * 用于 Utility AI 获取 LLM 的意图值（而不是插值后的当前值）
     * Used by Utility AI to get LLM's intended value (instead of interpolated current value)
     * 
     * @param VariableName - 变量名
     * @return 目标值，如果不存在则返回 0.0
     */
    UFUNCTION(BlueprintCallable, Category = "Mental State Interpolation")
    float GetTargetValue(const FString& VariableName) const;

private:
    // 目标值映射 / Target Value Mapping
    // Key: 变量名 (如 "Anger")
    // Value: 目标值 (0.0 - 1.0)
    TMap<FString, float> TargetValues;

    // 插值配置映射 / Interpolation Configuration Mapping
    // Key: 变量名
    // Value: 插值配置
    TMap<FString, FMentalStateInterpConfig> InterpConfigs;

    // 默认插值配置 / Default Interpolation Configuration
    FMentalStateInterpConfig DefaultConfig;

    // 辅助函数：获取当前值 / Helper: Get Current Value
    float GetCurrentValue(UNPCMentalState* MentalState, const FString& VariableName) const;

    // 辅助函数：设置当前值 / Helper: Set Current Value
    void SetCurrentValue(UNPCMentalState* MentalState, const FString& VariableName, float Value);

    // 辅助函数：应用随机扰动 / Helper: Apply Random Perturbation
    float ApplyPerturbation(float Value, const FMentalStateInterpConfig& Config) const;
};
