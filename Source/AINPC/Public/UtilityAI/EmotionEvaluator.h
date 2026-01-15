// EmotionEvaluator.h
// 情绪评估系统 - 包含心理学模型和情绪计算
// Emotion Evaluation System - Contains Psychology Model and Emotion Calculation

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UtilityAI/EmotionTypes.h"
#include "EmotionEvaluator.generated.h"

// Forward declarations
class UNPCMentalState;
struct FPersonalityConfig;

// ============================================================================
// FMaslowWeights - 5层马斯洛权重结构体
// ============================================================================

/**
 * Maslow 权重结构体 - 由 OCEAN 计算得出
 * Maslow Weights Struct - Calculated from OCEAN personality
 * 
 * 用于 Utility AI 的 CalculateScore,不用于 EmotionEvaluator
 * Used for Utility AI's CalculateScore, NOT for EmotionEvaluator
 */
USTRUCT(BlueprintType)
struct FMaslowWeights
{
	GENERATED_BODY()

	// 生理层权重 (Hunger/Energy)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maslow Weights")
	float Physiological = 1.0f;

	// 安全层权重 (Perceived_Threat)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maslow Weights")
	float Safety = 1.0f;

	// 社交层权重 (Loneliness)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maslow Weights")
	float Belonging = 1.0f;

	// 尊严层权重 (Anger)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maslow Weights")
	float Esteem = 1.0f;

	// 自我实现层权重 (Curiosity)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maslow Weights")
	float SelfActualization = 1.0f;
};

// ============================================================================
// UPsychologyModel - OCEAN → Maslow 权重转换
// ============================================================================

/**
 * 心理模型 DataAsset - OCEAN → Maslow 权重系数
 * Psychology Model DataAsset - OCEAN → Maslow weight coefficients
 * 
 * 用法: 创建 DA_HumanPsychology, DA_ZombiePsychology 等
 */
UCLASS(BlueprintType)
class AINPC_API UPsychologyModel : public UDataAsset
{
	GENERATED_BODY()

public:
	UPsychologyModel();

	// ========== OCEAN → Maslow 转换系数 ==========
	
	// 生理层: 直接使用此权重值
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physiological",
	          meta = (DisplayName = "生理需求权重", ClampMin = "0.0", ClampMax = "2.0"))
	float PhysiologicalWeight = 1.0f;

	// 安全层: SafetyWeight = 1.0 + (Neuroticism * NeuroticismToSafety)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Safety",
	          meta = (DisplayName = "神经质→安全感系数", ClampMin = "0.0", ClampMax = "3.0"))
	float NeuroticismToSafety = 1.5f;

	// 社交层: BelongingWeight = BelongingBaseWeight + (Extraversion * ExtraversionToBelonging)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belonging",
	          meta = (DisplayName = "外向性→归属感系数", ClampMin = "0.0", ClampMax = "3.0"))
	float ExtraversionToBelonging = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Belonging",
	          meta = (DisplayName = "归属感基础权重", ClampMin = "0.0", ClampMax = "2.0"))
	float BelongingBaseWeight = 0.5f;

	// 尊严层: EsteemWeight = Max(0.2, 1.0 + (N * NeuroticismToEsteem) - (A * AgreeablenessToEsteem))
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Esteem",
	          meta = (DisplayName = "神经质→尊严感系数(+)", ClampMin = "0.0", ClampMax = "2.0"))
	float NeuroticismToEsteem = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Esteem",
	          meta = (DisplayName = "宜人性→尊严感系数(-)", ClampMin = "0.0", ClampMax = "2.0"))
	float AgreeablenessToEsteem = 0.8f;

	// 自我实现层: SelfActualizationWeight = CuriosityBaseWeight + (Openness * OpennessToCuriosity)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self-Actualization",
	          meta = (DisplayName = "开放性→好奇心系数", ClampMin = "0.0", ClampMax = "3.0"))
	float OpennessToCuriosity = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self-Actualization",
	          meta = (DisplayName = "好奇心基础权重", ClampMin = "0.0", ClampMax = "2.0"))
	float CuriosityBaseWeight = 0.5f;

	/** 根据 OCEAN 性格计算 Maslow 权重 (用于 Utility AI) */
	UFUNCTION(BlueprintCallable, Category = "Psychology")
	FMaslowWeights RecalculateWeights(const FPersonalityConfig& Personality) const;
};

// ============================================================================
// UEmotionEvaluator - 情绪计算器
// ============================================================================

/**
 * 情绪计算器 - 根据 Mental State 和性格权重计算情绪状态
 * Emotion Evaluator - Calculates Emotion State using dynamic thresholds
 * 
 * 性格影响阈值:
 * - 高神经质 (High Safety Weight) → 更低的恐惧阈值 → 更容易害怕
 * - 高外向性 (High Belonging Weight) → 更低的孤独阈值 → 更容易孤独
 */
UCLASS()
class AINPC_API UEmotionEvaluator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 从 Mental State 和 性格权重 计算当前情绪
	 * 
	 * @param MentalState - 当前心理状态 (Hunger, Anger, etc.)
	 * @param Weights - 马斯洛权重 (由 OCEAN 性格计算)
	 * 
	 * 动态阈值公式: EffectiveThreshold = BaseThreshold / Weight
	 * 例如: 高神经质 (SafetyWeight=2.0) → FearThreshold = 0.85/2.0 = 0.425
	 */
	UFUNCTION(BlueprintCallable, Category = "Emotion")
	static EEmotionState CalculateEmotion(const UNPCMentalState* MentalState, const FMaslowWeights& Weights);
};

