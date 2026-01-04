// TestCheatManager.h
// 测试用作弊管理器，提供调试和测试功能

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "TestCheatManager.generated.h"

/**
 * 测试用作弊管理器
 * 提供各种调试和测试功能
 */
UCLASS()
class AINPC_API UTestCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/**
	 * 给所有 NPC 造成指定伤害
	 * @param Damage 伤害值
	 */
	UFUNCTION(Exec, Category = "Test|NPC")
	void DamageAllNPCs(float Damage = 10.0f);

	/**
	 * 杀死所有 NPC
	 */
	UFUNCTION(Exec, Category = "Test|NPC")
	void KillAllNPCs();

	/**
	 * 治疗所有 NPC 到满血
	 */
	UFUNCTION(Exec, Category = "Test|NPC")
	void HealAllNPCs();

	/**
	 * 列出所有 NPC 的状态
	 */
	UFUNCTION(Exec, Category = "Test|NPC")
	void ListAllNPCs();

	/**
	 * 给指定 NPC 造成伤害
	 * @param NPCName NPC 名称（部分匹配）
	 * @param Damage 伤害值
	 */
	UFUNCTION(Exec, Category = "Test|NPC")
	void DamageNPC(const FString& NPCName, float Damage = 10.0f);

	/**
	 * 测试 LLM 连接
	 */
	UFUNCTION(Exec, Category = "Test|LLM")
	void TestLLMConnection();

	/**
	 * 触发指定 NPC 的情绪测试
	 * @param NPCName NPC 名称（部分匹配）
	 * @param EmotionType 情绪类型 (anger, fear, joy 等)
	 */
	UFUNCTION(Exec, Category = "Test|NPC")
	void TriggerEmotion(const FString& NPCName, const FString& EmotionType);
};
