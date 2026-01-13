// EngineManagedValues.h
// Engine 管理的马斯洛变量值 / Engine-Managed Maslow Values
//
// 这个结构体存储由游戏引擎计算的基础值
// This struct stores base values calculated by the game engine

#pragma once

#include "CoreMinimal.h"
#include "EngineManagedValues.generated.h"

/**
 * Engine 管理的马斯洛变量值 / Engine-Managed Maslow Values
 * 
 * 这些值由游戏引擎在 Tick() 中计算，用于：
 * These values are calculated by the game engine in Tick(), used for:
 * 
 * 1. Engine 独裁区：直接作为最终值
 *    Engine Exclusive: Directly used as final values
 * 
 * 2. 混合管辖区：作为基础值，与 LLM 的值合并
 *    Hybrid: Used as base values, merged with LLM values
 * 
 * 详见: docs/design/Maslow_Variables_Jurisdiction.md
 */
USTRUCT(BlueprintType)
struct FEngineManagedValues
{
	GENERATED_BODY()

	// ========== Engine 独裁区 (Engine Exclusive) ==========
	
	/**
	 * 饥饿感 / Hunger
	 * 
	 * 更新逻辑 / Update Logic:
	 * - Hunger += DeltaTime × MetabolismRate
	 * - 吃东西时 / When eating: Hunger -= FoodValue
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Engine Exclusive")
	float Hunger = 0.0f;

	/**
	 * 疲劳度 / Fatigue
	 * 
	 * 更新逻辑 / Update Logic:
	 * - 活动时 / When active: Fatigue += FatigueRate × DeltaTime
	 * - 休息时 / When resting: Fatigue -= RestoreRate × DeltaTime
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Engine Exclusive")
	float Fatigue = 0.0f;

	// ========== 混合管辖区 - Engine 部分 (Hybrid - Engine Part) ==========
	
	/**
	 * 感知威胁 (Engine 部分) / Perceived Threat (Engine Part)
	 * 
	 * 更新逻辑 / Update Logic:
	 * - 检测视野内的敌人数量 / Detect enemies in sight
	 * - 听到枪声 / Hear gunshots
	 * - 检测血量低于阈值 / Detect low health
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Hybrid - Engine Part")
	float Perceived_Threat = 0.0f;

	/**
	 * 资源焦虑 (Engine 部分) / Resource Anxiety (Engine Part)
	 * 
	 * 更新逻辑 / Update Logic:
	 * - 检测弹药百分比 / Check ammo percentage
	 * - 检测金钱数量 / Check money amount
	 * - 检测背包空间 / Check inventory space
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Hybrid - Engine Part")
	float Resource_Anxiety = 0.0f;

	/**
	 * 孤独感 (Engine 部分) / Loneliness (Engine Part)
	 * 
	 * 更新逻辑 / Update Logic:
	 * - 计时器：TimeSinceLastTalk / Timer: Time since last talk
	 * - 线性增长 / Linear growth
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Hybrid - Engine Part")
	float Loneliness = 0.0f;

	/**
	 * 责任紧迫感 (Engine 部分) / Duty Urgency (Engine Part)
	 * 
	 * 更新逻辑 / Update Logic:
	 * - 检查当前时间是否到了换岗时间 / Check if it's shift change time
	 * - 检查任务截止时间 / Check task deadline
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Hybrid - Engine Part")
	float Duty_Urgency = 0.0f;

	// ========== 辅助函数 (Helper Functions) ==========
	
	/**
	 * 重置所有值为默认值 / Reset all values to defaults
	 */
	void Reset()
	{
		Hunger = 0.0f;
		Fatigue = 0.0f;
		Perceived_Threat = 0.0f;
		Resource_Anxiety = 0.0f;
		Loneliness = 0.0f;
		Duty_Urgency = 0.0f;
	}

	/**
	 * 调试输出 / Debug Output
	 */
	FString ToString() const
	{
		return FString::Printf(
			TEXT("EngineManagedValues:\n")
			TEXT("  [ENGINE] Hunger: %.2f\n")
			TEXT("  [ENGINE] Fatigue: %.2f\n")
			TEXT("  [HYBRID] Perceived_Threat: %.2f\n")
			TEXT("  [HYBRID] Resource_Anxiety: %.2f\n")
			TEXT("  [HYBRID] Loneliness: %.2f\n")
			TEXT("  [HYBRID] Duty_Urgency: %.2f"),
			Hunger, Fatigue,
			Perceived_Threat, Resource_Anxiety,
			Loneliness, Duty_Urgency
		);
	}
};
