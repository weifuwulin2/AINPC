#pragma once

#include "CoreMinimal.h"

class UFactionReputationComponent;
class UFactionSubsystem;

/**
 * Faction Utility Functions
 * 阵营工具函数
 *
 * Provides centralized utilities for faction identification, hostility checks,
 * and reputation queries. These functions are stateless and can be called
 * from any system without requiring component ownership.
 *
 * 提供集中的阵营识别、敌对检查和声望查询工具。
 * 这些函数无状态，可以从任何系统调用而不需要拥有组件。
 */
namespace FactionHelpers
{
	/**
	 * Standard hostility threshold (0-100 attitude scale)
	 * Values below this threshold are considered hostile
	 * 标准敌对阈值（0-100 态度量表）
	 * 低于此阈值的值被视为敌对
	 */
	AINPC_API constexpr float DEFAULT_HOSTILITY_THRESHOLD = 25.0f;

	/**
	 * Convert attitude value (0-100) to boolean hostility status
	 * 将态度值（0-100）转换为布尔敌对状态
	 *
	 * @param Attitude - Reputation value from 0 (hostile) to 100 (friendly)
	 * @param Threshold - Custom hostility threshold (default: 25.0f)
	 * @return true if attitude is below threshold (hostile), false otherwise
	 *
	 * Example:
	 *   IsAttitudeHostile(20.0f) -> true  (hostile)
	 *   IsAttitudeHostile(50.0f) -> false (neutral/friendly)
	 */
	AINPC_API bool IsAttitudeHostile(float Attitude, float Threshold = DEFAULT_HOSTILITY_THRESHOLD);

	/**
	 * Get the faction ID of an actor
	 * 获取 Actor 的阵营 ID
	 *
	 * Resolves faction through multiple fallback strategies:
	 * 1. Scene status override (Status.InScene -> "Neutral")
	 * 2. FactionReputationComponent.FactionID
	 * 3. Actor tags (Faction.*)
	 * 4. NPCDefinitionComponent.FactionID
	 * 5. Player tag check
	 * 6. Default: "Neutral"
	 *
	 * Handles Controller/Pawn duality automatically.
	 *
	 * @param Actor - The actor to query (can be Pawn, Controller, or any Actor)
	 * @return Faction ID name (e.g., "Player", "Orc", "Neutral")
	 *
	 * Example:
	 *   GetFactionID(OrcWarrior) -> "Orc"
	 *   GetFactionID(PlayerController) -> "Player"
	 */
	AINPC_API FName GetFactionID(AActor* Actor);

	/**
	 * Check if two actors are hostile to each other
	 * 检查两个 Actor 是否互相敌对
	 *
	 * Performs bidirectional hostility check using FactionReputationComponent
	 * and FactionSubsystem. Checks both A->B and B->A relationships.
	 *
	 * 使用 FactionReputationComponent 和 FactionSubsystem 执行双向敌对检查。
	 * 检查 A->B 和 B->A 的关系。
	 *
	 * @param ActorA - First actor
	 * @param ActorB - Second actor
	 * @return true if either actor considers the other hostile
	 *
	 * Example:
	 *   AreActorsHostile(Orc, Player) -> true (if Orc faction is hostile to Player)
	 *   AreActorsHostile(Guard1, Guard2) -> false (same faction)
	 */
	AINPC_API bool AreActorsHostile(AActor* ActorA, AActor* ActorB);

	/**
	 * Convert attitude value (0-100) to a human-readable semantic description for LLM context.
	 * 将态度值转换为 LLM 可理解的语义描述。
	 *
	 * @param Attitude - Reputation value from 0 to 100
	 * @return Descriptive string (e.g. "Sworn Enemy", "Hostile", "Distrustful", "Neutral", "Friendly", "Trusted Ally")
	 */
	AINPC_API FString GetAttitudeDescription(float Attitude);
}
