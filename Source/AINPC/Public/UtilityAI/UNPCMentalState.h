// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UNPCMentalState.generated.h"

/**
 * 
 */
UCLASS()
class AINPC_API UNPCMentalState : public UObject
{
	GENERATED_BODY()
    
    public:
    	UNPCMentalState();
    
    	// 使用 Category 分类，方便在编辑器调试
    	// EditAnywhere: 可以在蓝图或实例中修改
    	// BlueprintReadWrite: 蓝图逻辑可以读写
    	// meta=(ClampMin, ClampMax): 限制数值在 0-1 之间，防止 LLM 输出非法值
    
    	// === 情绪维度 (Emotional Dimensions) ===
    	
    	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mental State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    	float Anger;
    
    	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mental State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    	float Fear;
    
    	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mental State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    	float Confidence;
    
    	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mental State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    	float SocialBattery;
    
    	// === 生理维度 (Physical Dimensions) ===
    	
    	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    	float Hunger;
    
    	// 辅助函数：重置状态
    	UFUNCTION(BlueprintCallable, Category = "Mental State")
    	void ResetState();
};
