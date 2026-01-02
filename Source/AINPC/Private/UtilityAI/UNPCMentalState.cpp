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
	// ✅ 使用宏自动生成所有字段的转换代码
	#define UPDATE_FIELD(Name, DefaultValue, DisplayName, Description) \
		Name = FMath::Clamp(NewState.Name, 0.0f, 1.0f);
	
	MENTAL_STATE_FIELDS(UPDATE_FIELD)
	
	#undef UPDATE_FIELD
	
	// 可选：打印日志用于调试
	UE_LOG(LogTemp, Verbose, TEXT("[MentalState] Updated from struct"));
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