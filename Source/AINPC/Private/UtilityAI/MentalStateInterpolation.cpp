// MentalStateInterpolation.cpp
// 心理状态平滑插值实现 / Mental State Smooth Interpolation Implementation

#include "UtilityAI/MentalStateInterpolation.h"
#include "UtilityAI/MentalStateNames.h" // ✅ Use Constants
#include "UtilityAI/MentalStateFields.h"

UMentalStateInterpolator::UMentalStateInterpolator()
{
    // 初始化默认配置
    // Initialize default configuration
    DefaultConfig.InterpSpeed = 2.0f;
    DefaultConfig.SnapThreshold = 0.01f;
    DefaultConfig.bEnableRandomPerturbation = true;
    DefaultConfig.PerturbationRange = 0.05f;
}

void UMentalStateInterpolator::SetTargetValue(const FString& VariableName, float TargetValue)
{
    // ✅ CRITICAL: Skip Engine-exclusive fields (Hunger, Fatigue)
    // These are managed by MetabolismComponent and should NOT have target values set by LLM
    // Note: Perceived_Threat is allowed here as LLM can trigger reactions.
    if (VariableName == MentalStateNames::Hunger || 
        VariableName == MentalStateNames::Fatigue)
    {
        // Skip - Engine manages these fields exclusively
        return;
    }

    // 获取配置
    // Get configuration
    const FMentalStateInterpConfig* ConfigPtr = InterpConfigs.Find(VariableName);
    const FMentalStateInterpConfig& Config = ConfigPtr ? *ConfigPtr : DefaultConfig;

    // 应用随机扰动（可选）
    // Apply random perturbation (optional)
    float FinalTarget = TargetValue;
    if (Config.bEnableRandomPerturbation)
    {
        FinalTarget = ApplyPerturbation(TargetValue, Config);
    }

    // 限制范围
    // Clamp range
    FinalTarget = FMath::Clamp(FinalTarget, 0.0f, 1.0f);

    // 存储目标值
    // Store target value
    TargetValues.Add(VariableName, FinalTarget);

    UE_LOG(LogTemp, Log, TEXT("[MentalStateInterpolator] Set target for %s: %.3f (original: %.3f)"), 
           *VariableName, FinalTarget, TargetValue);
}

void UMentalStateInterpolator::UpdateInterpolation(UNPCMentalState* MentalState, float DeltaTime)
{
    // ✅ NEW ARCHITECTURE:
    // The Interpolator now ONLY manages the "Intention" (TargetValues).
    // It NO LONGER writes directly to the MentalState fields.
    // This allows MetabolismComponent to be the sole master of the NPC's actual physical state.

    // 遍历所有目标值，只更新内部 TargetValues 字典
    // Iterate through target values and only update the internal dictionary
    for (auto& Pair : TargetValues)
    {
        const FString& VariableName = Pair.Key;
        float& TargetValue = Pair.Value;

        // ✅ Step 0: Target Decay
        // All LLM intentions (threat, boredom, etc.) should naturally fade over time.
        // This prevents an NPC from being stuck in "Scared" mode forever if the LLM stops responding.
        if (VariableName != TEXT("Hunger") && VariableName != TEXT("Fatigue"))
        {
            // The Target itself drifts back to zero.
            TargetValue = FMath::FInterpTo(TargetValue, 0.0f, DeltaTime, 0.2f); // Slow drift
        }
    }
}

void UMentalStateInterpolator::SetInterpConfig(const FString& VariableName, const FMentalStateInterpConfig& Config)
{
    InterpConfigs.Add(VariableName, Config);
    UE_LOG(LogTemp, Log, TEXT("[MentalStateInterpolator] Set config for %s: Speed=%.2f, Threshold=%.3f"), 
           *VariableName, Config.InterpSpeed, Config.SnapThreshold);
}

FMentalStateInterpConfig UMentalStateInterpolator::GetInterpConfig(const FString& VariableName) const
{
    const FMentalStateInterpConfig* ConfigPtr = InterpConfigs.Find(VariableName);
    return ConfigPtr ? *ConfigPtr : DefaultConfig;
}

void UMentalStateInterpolator::SetValueImmediate(UNPCMentalState* MentalState, const FString& VariableName, float Value)
{
    if (!MentalState)
    {
        UE_LOG(LogTemp, Error, TEXT("[MentalStateInterpolator] MentalState is null!"));
        return;
    }

    // 限制范围
    // Clamp range
    Value = FMath::Clamp(Value, 0.0f, 1.0f);

    // 立即设置当前值和目标值
    // Immediately set both current and target value
    SetCurrentValue(MentalState, VariableName, Value);
    TargetValues.Add(VariableName, Value);

    UE_LOG(LogTemp, Log, TEXT("[MentalStateInterpolator] Set %s immediately to %.3f"), *VariableName, Value);
}

void UMentalStateInterpolator::ResetAllTargets()
{
    TargetValues.Empty();
    UE_LOG(LogTemp, Log, TEXT("[MentalStateInterpolator] Reset all target values"));
}

float UMentalStateInterpolator::GetTargetValue(const FString& VariableName) const
{
    // 查找目标值
    // Find target value
    const float* ValuePtr = TargetValues.Find(VariableName);
    
    if (ValuePtr)
    {
        return *ValuePtr;
    }
    else
    {
        // 如果没有设置目标值，返回 0.0
        // If no target value is set, return 0.0
        return 0.0f;
    }
}

float UMentalStateInterpolator::GetCurrentValue(UNPCMentalState* MentalState, const FString& VariableName) const
{
    // 使用宏自动生成的 getter
    // Use macro-generated getter
    #define GET_FIELD_VALUE(Name, DefaultValue, DisplayName, Description) \
        if (VariableName == TEXT(#Name)) return MentalState->Name;
    
    MENTAL_STATE_FIELDS(GET_FIELD_VALUE)
    
    #undef GET_FIELD_VALUE

    UE_LOG(LogTemp, Warning, TEXT("[MentalStateInterpolator] Unknown variable: %s"), *VariableName);
    return 0.0f;
}

void UMentalStateInterpolator::SetCurrentValue(UNPCMentalState* MentalState, const FString& VariableName, float Value)
{
    // ✅ CRITICAL: Skip Engine-exclusive fields (Hunger, Fatigue)
    // These are managed by MetabolismComponent and should NOT be overwritten by Interpolator
    // Perceived_Threat is now managed by both: LLM can "jump" it, and Metabolism can "decay" it.
    if (VariableName == TEXT("Hunger") || 
        VariableName == TEXT("Fatigue"))
    {
        // Skip - these are Engine-exclusive fields
        return;
    }

    // 限制范围
    // Clamp range
    Value = FMath::Clamp(Value, 0.0f, 1.0f);

    // 使用宏自动生成的 setter
    // Use macro-generated setter
    #define SET_FIELD_VALUE(Name, DefaultValue, DisplayName, Description) \
        if (VariableName == TEXT(#Name)) { MentalState->Name = Value; return; }
    
    MENTAL_STATE_FIELDS(SET_FIELD_VALUE)
    
    #undef SET_FIELD_VALUE

    UE_LOG(LogTemp, Warning, TEXT("[MentalStateInterpolator] Unknown variable: %s"), *VariableName);
}

float UMentalStateInterpolator::ApplyPerturbation(float Value, const FMentalStateInterpConfig& Config) const
{
    // 添加随机扰动
    // Add random perturbation
    float Perturbation = FMath::RandRange(-Config.PerturbationRange, Config.PerturbationRange);
    return Value + Perturbation;
}
