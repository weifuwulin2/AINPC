// EmotionMatrixConfig.h
// 情绪矩阵配置 - 定义 Emotion x Activity -> Multiplier 映射
// Emotion Matrix Config - Defines Emotion x Activity -> Multiplier mapping

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "UtilityAI/EmotionTypes.h"
#include "EmotionMatrixConfig.generated.h"

/**
 * 情绪矩阵行 - 每种情绪对不同活动的分数乘数
 * Emotion Matrix Row - Score multipliers for each emotion per activity type
 * 
 * 使用方法:
 * 1. 创建 DataTable，行类型选择 FEmotionMatrixRow
 * 2. RowName 设置为 Emotion 名称 (e.g., "Angry", "Scared")
 * 3. 填写每种 Activity 的乘数
 */
USTRUCT(BlueprintType)
struct FEmotionMatrixRow : public FTableRowBase
{
	GENERATED_BODY()

	// Combat 活动乘数 (Attack, Shoot, Chase)
	// Angry=2.0, Scared=0.1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multipliers",
	          meta = (DisplayName = "Combat (战斗)", ClampMin = "0.0", ClampMax = "5.0"))
	float Combat = 1.0f;

	// Flee 活动乘数 (RunAway, Hide, Beg)
	// Scared=5.0, Angry=0.2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multipliers",
	          meta = (DisplayName = "Flee (逃跑)", ClampMin = "0.0", ClampMax = "5.0"))
	float Flee = 1.0f;

	// Social 活动乘数 (Chat, Greet, Trade)
	// Happy=2.0, Scared=0.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multipliers",
	          meta = (DisplayName = "Social (社交)", ClampMin = "0.0", ClampMax = "5.0"))
	float Social = 1.0f;

	// Investigate 活动乘数 (LookAt, Approach, Inspect)
	// Curious=3.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multipliers",
	          meta = (DisplayName = "Investigate (探索)", ClampMin = "0.0", ClampMax = "5.0"))
	float Investigate = 1.0f;

	// Rest 活动乘数 (Sleep, Sit, Idle)
	// Sad=2.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multipliers",
	          meta = (DisplayName = "Rest (休息)", ClampMin = "0.0", ClampMax = "5.0"))
	float Rest = 1.0f;

	// Avoid 活动乘数 (Reject, WalkAway, Vomit)
	// Disgust=3.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multipliers",
	          meta = (DisplayName = "Avoid (回避)", ClampMin = "0.0", ClampMax = "5.0"))
	float Avoid = 1.0f;

	// Work 活动乘数 (Patrol, Craft, Guard)
	// Neutral=1.2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multipliers",
	          meta = (DisplayName = "Work (工作)", ClampMin = "0.0", ClampMax = "5.0"))
	float Work = 1.0f;

	// Eat 活动乘数 (Eat, Drink)
	// 所有情绪默认=1.0，但 Hungry 时由 Drive 直接驱动
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multipliers",
	          meta = (DisplayName = "Eat (进食)", ClampMin = "0.0", ClampMax = "5.0"))
	float Eat = 1.0f;

	/**
	 * 根据 ActivityTag 获取对应乘数
	 * Get multiplier by Activity tag
	 */
	float GetMultiplier(const FGameplayTag& ActivityTag) const;
};
