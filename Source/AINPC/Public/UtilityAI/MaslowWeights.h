// MaslowWeights.h
// 马斯洛需求层次权重 / Maslow's Hierarchy of Needs Weights
//
// 这些权重由 OCEAN 性格计算得出，游戏运行时只读不改
// These weights are calculated from OCEAN personality, read-only during runtime

#pragma once

#include "CoreMinimal.h"
#include "MaslowWeights.generated.h"

/**
 * 马斯洛需求层次权重结构 / Maslow's Hierarchy Weights Structure
 * 
 * 这些权重决定了 NPC 对各个马斯洛变量的"敏感度"
 * These weights determine the NPC's "sensitivity" to each Maslow variable
 * 
 * 计算公式：Weight = OCEAN_Value × Coefficient
 * Calculation: Weight = OCEAN_Value × Coefficient
 */
USTRUCT(BlueprintType)
struct FMaslowWeights
{
	GENERATED_BODY()

	// ========== 生理层 (Physiological) ==========
	
	// 饥饿权重 / Hunger Weight
	UPROPERTY(BlueprintReadOnly, Category = "Maslow Weights | Physiological")
	float HungerWeight = 1.0f;

	// 精力权重 / Energy Weight
	UPROPERTY(BlueprintReadOnly, Category = "Maslow Weights | Physiological")
	float EnergyWeight = 1.0f;

	// ========== 安全层 (Safety) ==========
	
	// 威胁感知权重 / Threat Perception Weight
	UPROPERTY(BlueprintReadOnly, Category = "Maslow Weights | Safety")
	float ThreatWeight = 1.0f;

	// 资源焦虑权重 / Resource Anxiety Weight
	UPROPERTY(BlueprintReadOnly, Category = "Maslow Weights | Safety")
	float ResourceAnxietyWeight = 1.0f;

	// ========== 社交层 (Love/Belonging) ==========
	
	// 孤独感权重 / Loneliness Weight
	UPROPERTY(BlueprintReadOnly, Category = "Maslow Weights | Social")
	float LonelinessWeight = 1.0f;

	// 信任度权重 / Trust Weight
	UPROPERTY(BlueprintReadOnly, Category = "Maslow Weights | Social")
	float TrustWeight = 1.0f;

	// ========== 尊严层 (Esteem) ==========
	
	// 愤怒权重 / Anger Weight
	UPROPERTY(BlueprintReadOnly, Category = "Maslow Weights | Esteem")
	float AngerWeight = 1.0f;

	// 地位需求权重 / Social Status Weight
	UPROPERTY(BlueprintReadOnly, Category = "Maslow Weights | Esteem")
	float SocialStatusWeight = 1.0f;

	// ========== 自我实现层 (Self-Actualization) ==========
	
	// 责任感权重 / Duty Weight
	UPROPERTY(BlueprintReadOnly, Category = "Maslow Weights | Self-Actualization")
	float DutyWeight = 1.0f;

	// 好奇心权重 / Curiosity Weight
	UPROPERTY(BlueprintReadOnly, Category = "Maslow Weights | Self-Actualization")
	float CuriosityWeight = 1.0f;

	// 默认构造函数
	FMaslowWeights()
		: HungerWeight(1.0f)
		, EnergyWeight(1.0f)
		, ThreatWeight(1.0f)
		, ResourceAnxietyWeight(1.0f)
		, LonelinessWeight(1.0f)
		, TrustWeight(1.0f)
		, AngerWeight(1.0f)
		, SocialStatusWeight(1.0f)
		, DutyWeight(1.0f)
		, CuriosityWeight(1.0f)
	{}

	// 调试输出 / Debug Output
	FString ToString() const
	{
		return FString::Printf(
			TEXT("MaslowWeights:\n")
			TEXT("  Physiological: Hunger=%.2f, Energy=%.2f\n")
			TEXT("  Safety: Threat=%.2f, ResourceAnxiety=%.2f\n")
			TEXT("  Social: Loneliness=%.2f, Trust=%.2f\n")
			TEXT("  Esteem: Anger=%.2f, SocialStatus=%.2f\n")
			TEXT("  Self-Actualization: Duty=%.2f, Curiosity=%.2f"),
			HungerWeight, EnergyWeight,
			ThreatWeight, ResourceAnxietyWeight,
			LonelinessWeight, TrustWeight,
			AngerWeight, SocialStatusWeight,
			DutyWeight, CuriosityWeight
		);
	}
};
