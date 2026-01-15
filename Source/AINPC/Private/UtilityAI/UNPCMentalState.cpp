// Fill out your copyright notice in the Description page of Project Settings.

#include "UtilityAI/UNPCMentalState.h"
#include "LLM/LLMCommunicator.h" // 引入 FMentalState 定义

UNPCMentalState::UNPCMentalState()
{
	// ✅ 使用宏自动初始化所有字段
	#define INIT_FIELD(Name, DefaultValue, DisplayName, Description) \
		Name = DefaultValue;
	
	MENTAL_STATE_FIELDS(INIT_FIELD)
	
	#undef INIT_FIELD
}

void UNPCMentalState::ResetState()
{
	// ✅ 使用宏自动重置所有字段
	#define RESET_FIELD(Name, DefaultValue, DisplayName, Description) \
		Name = DefaultValue;
	
	MENTAL_STATE_FIELDS(RESET_FIELD)
	
	#undef RESET_FIELD
}

// =========================================================
// 数据结构转换函数实现 (自动生成)
// =========================================================

void UNPCMentalState::UpdateFromStruct(const FMentalState& NewState)
{
	// ✅ CRITICAL: Skip Engine-exclusive fields (Hunger, Fatigue)
	// These are managed by MetabolismComponent and should NOT be overwritten by LLM
	// 
	// ❌ 不要更新 Hunger 和 Fatigue！它们由 MetabolismComponent 管理
	// ❌ DO NOT update Hunger and Fatigue! They are managed by MetabolismComponent
	
	// 🔍 调试：显示 LLM 返回的 Hunger/Fatigue 值（但不使用它们）
	UE_LOG(LogTemp, Warning, TEXT("[MentalState] LLM returned Hunger=%.3f, Fatigue=%.3f (SKIPPED - using Engine values: Hunger=%.3f, Fatigue=%.3f)"),
	       NewState.Hunger, NewState.Fatigue, Hunger, Fatigue);
	
	// 只更新 LLM 管辖的 4 个字段 (简化后的马斯洛模型)
	// Only update LLM-controlled 4 fields (simplified Maslow model)
	Perceived_Threat = FMath::Clamp(NewState.Perceived_Threat, 0.0f, 1.0f);  // 安全层
	Loneliness = FMath::Clamp(NewState.Loneliness, 0.0f, 1.0f);              // 社交层
	Indignity = FMath::Clamp(NewState.Indignity, 0.0f, 1.0f);            // 尊严层
	Boredom = FMath::Clamp(NewState.Boredom, 0.0f, 1.0f);                // 自我实现层
	
	// ✅ 复制 Intention (LLM 的策略建议)
	// Copy Intention (LLM's strategic suggestion)
	Intention = NewState.Intention;
	
	UE_LOG(LogTemp, Log, TEXT("[MentalState] Updated: Intention='%s'"), *Intention);
}

FMentalState UNPCMentalState::ToStruct() const
{
	// ✅ 使用宏自动生成所有字段的转换代码
	FMentalState Result;
	
	#define COPY_FIELD(Name, DefaultValue, DisplayName, Description) \
		Result.Name = Name;
	
	MENTAL_STATE_FIELDS(COPY_FIELD)
	
	#undef COPY_FIELD
	
	return Result;
}