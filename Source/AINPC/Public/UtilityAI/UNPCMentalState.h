// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UtilityAI/MentalStateFields.h"  // ✅ 引入字段配置
#include "UtilityAI/EmotionTypes.h"        // ✅ 引入情绪枚举
#include "UNPCMentalState.generated.h"

// 前置声明：FMentalState 定义在 LLMCommunicator.h
// Forward declaration: FMentalState is defined in LLMCommunicator.h
struct FMentalState;

/**
 * NPC 精神状态 (自动生成所有字段)
 * NPC Mental State (all fields auto-generated)
 */
UCLASS()
class AINPC_API UNPCMentalState : public UObject
{
	GENERATED_BODY()
    
public:
	UNPCMentalState();
	
	// =========================================================
	// 情绪维度 (自动生成)
	// Emotion Dimensions (auto-generated)
	// =========================================================
	
	// ✅ 使用宏自动生成所有字段
	// ✅ Use macro to auto-generate all fields
	#define DECLARE_PROPERTY(Name, DefaultValue, DisplayName, Description) \
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mental State", \
		          meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = DisplayName, ToolTip = Description)) \
		float Name;
	
	MENTAL_STATE_FIELDS(DECLARE_PROPERTY)
	
	#undef DECLARE_PROPERTY

	// =========================================================
	// Emotion State (Engine-Driven)
	// 由 EmotionEvaluator 每帧计算
	// =========================================================
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Emotion State")
	EEmotionState CurrentEmotion = EEmotionState::Neutral;

	// =========================================================
	// Intention (LLM-Driven Strategy Bias)
	// 由 CognitionComponent 从 LLM 获取
	// =========================================================
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LLM State")
	FString Intention;
	
	// =========================================================
	// 辅助函数
	// Helper Functions
	// =========================================================
	
	// 辅助函数：重置状态
	// Helper function: Reset state
	UFUNCTION(BlueprintCallable, Category = "Mental State")
	void ResetState();

	// =========================================================
	// 数据结构转换函数 (Data Conversion)
	// Data Structure Conversion Functions
	// =========================================================
	
	/**
	 * 从 FMentalState 结构体更新当前对象
	 * Update current object from FMentalState struct
	 * @param NewState - LLM返回的结构体数据 / Struct data returned by LLM
	 */
	UFUNCTION(BlueprintCallable, Category = "Mental State")
	void UpdateFromStruct(const FMentalState& NewState);
	
	/**
	 * 将当前对象转换为 FMentalState 结构体
	 * Convert current object to FMentalState struct
	 * @return FMentalState 结构体 / FMentalState struct
	 */
	UFUNCTION(BlueprintCallable, Category = "Mental State")
	FMentalState ToStruct() const;
};
