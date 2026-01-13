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
	
	// 只更新 LLM 管辖的字段 / Only update LLM-controlled fields
	Perceived_Threat = FMath::Clamp(NewState.Perceived_Threat, 0.0f, 1.0f);
	Resource_Anxiety = FMath::Clamp(NewState.Resource_Anxiety, 0.0f, 1.0f);
	Loneliness = FMath::Clamp(NewState.Loneliness, 0.0f, 1.0f);
	Trust = FMath::Clamp(NewState.Trust, 0.0f, 1.0f);
	Anger = FMath::Clamp(NewState.Anger, 0.0f, 1.0f);
	Social_Status = FMath::Clamp(NewState.Social_Status, 0.0f, 1.0f);
	Duty_Urgency = FMath::Clamp(NewState.Duty_Urgency, 0.0f, 1.0f);
	Curiosity = FMath::Clamp(NewState.Curiosity, 0.0f, 1.0f);
	
	// 可选：打印日志用于调试
	UE_LOG(LogTemp, Verbose, TEXT("[MentalState] Updated from struct (skipped Engine fields: Hunger, Energy)"));
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