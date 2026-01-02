// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UtilityAI/MentalStateFields.h"  // ✅ 引入字段配置
#include "UNPCMentalState.generated.h"

// 前置声明：FMentalState 定义在 LLMCommunicator.h
struct FMentalState;

/**
 * NPC 精神状态 (自动生成所有字段)
 */
UCLASS()
class AINPC_API UNPCMentalState : public UObject
{
	GENERATED_BODY()
    
public:
	UNPCMentalState();
	
	// =========================================================
	// 情绪维度 (自动生成)
	// =========================================================
	
	// ✅ 使用宏自动生成所有字段
	#define DECLARE_PROPERTY(Name, DefaultValue, DisplayName, Description) \
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mental State", \
		          meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = DisplayName, ToolTip = Description)) \
		float Name;
	
	MENTAL_STATE_FIELDS(DECLARE_PROPERTY)
	
	#undef DECLARE_PROPERTY
	
	// =========================================================
	// 辅助函数
	// =========================================================
	
	// 辅助函数：重置状态
	UFUNCTION(BlueprintCallable, Category = "Mental State")
	void ResetState();

	// =========================================================
	// 数据结构转换函数 (Data Conversion)
	// =========================================================
	
	/**
	 * 从 FMentalState 结构体更新当前对象
	 * @param NewState - LLM返回的结构体数据
	 */
	UFUNCTION(BlueprintCallable, Category = "Mental State")
	void UpdateFromStruct(const FMentalState& NewState);
	
	/**
	 * 将当前对象转换为 FMentalState 结构体
	 * @return FMentalState 结构体
	 */
	UFUNCTION(BlueprintCallable, Category = "Mental State")
	FMentalState ToStruct() const;
};
