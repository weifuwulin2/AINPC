// EmotionEvaluator.cpp
// 情绪计算器实现 / Emotion Evaluator Implementation

#include "UtilityAI/EmotionEvaluator.h"
#include "UtilityAI/UNPCMentalState.h"
#include "UtilityAI/PersonalityConfig.h"

// ========== UPsychologyModel Implementation ==========

UPsychologyModel::UPsychologyModel()
{
	// 默认系数已在头文件中定义
	// Default coefficients are defined in the header file
}

FMaslowWeights UPsychologyModel::RecalculateWeights(const FPersonalityConfig& Personality) const
{
	FMaslowWeights Weights;

	// 生理层: 使用配置的权重值 (僵尸=0, 人类=1, 野兽=1.5)
	Weights.Physiological = PhysiologicalWeight;

	// 安全层: 神经质越高，越怕死
	Weights.Safety = 1.0f + (Personality.Neuroticism * NeuroticismToSafety);

	// 社交层: 外向者不仅爱社交，而且不社交会死
	Weights.Belonging = BelongingBaseWeight + (Personality.Extraversion * ExtraversionToBelonging);

	// 尊严层: 神经质易怒，宜人者难怒
	Weights.Esteem = FMath::Max(0.2f, 1.0f + (Personality.Neuroticism * NeuroticismToEsteem) - (Personality.Agreeableness * AgreeablenessToEsteem));

	// 自我实现层: 只有开放者才会作死探索
	Weights.SelfActualization = CuriosityBaseWeight + (Personality.Openness * OpennessToCuriosity);

	UE_LOG(LogTemp, Log, TEXT("=== RecalculateWeights (5-Layer) ==="));
	UE_LOG(LogTemp, Log, TEXT("  OCEAN: O=%.2f, C=%.2f, E=%.2f, A=%.2f, N=%.2f"),
		Personality.Openness, Personality.Conscientiousness, Personality.Extraversion,
		Personality.Agreeableness, Personality.Neuroticism);
	UE_LOG(LogTemp, Log, TEXT("  Weights: Physio=%.2f, Safety=%.2f, Belong=%.2f, Esteem=%.2f, SelfActual=%.2f"),
		Weights.Physiological, Weights.Safety, Weights.Belonging, Weights.Esteem, Weights.SelfActualization);

	return Weights;
}

// ========== UEmotionEvaluator Implementation ==========

EEmotionState UEmotionEvaluator::CalculateEmotion(const UNPCMentalState* MentalState, const FMaslowWeights& Weights)
{
	if (!MentalState)
	{
		return EEmotionState::Neutral;
	}

	// ========================================================
	// 动态阈值计算 (Dynamic Threshold Calculation)
	// ========================================================
	// 公式: 实际阈值 = 基础阈值 / 性格权重
	// 例如: 高神经质 (SafetyWeight=2.0) → FearThreshold = 0.85/2.0 = 0.425
	
	const float BaseScaredThreshold = 0.85f;
	const float BaseHungerThreshold = 0.80f;
	const float BaseFatigueThreshold = 0.80f;
	const float BaseIndignityThreshold = 0.70f;
	const float BaseBoredomThreshold = 0.70f;
	const float BaseLonelinessHigh = 0.70f;
	const float BaseLonelinessLow = 0.30f;

	// 动态阈值 (权重越高 → 阈值越低 → 越容易触发)
	float ScaredThreshold = FMath::Clamp(BaseScaredThreshold / FMath::Max(0.5f, Weights.Safety), 0.3f, 1.0f);
	float HungerThreshold = FMath::Clamp(BaseHungerThreshold / FMath::Max(0.5f, Weights.Physiological), 0.3f, 1.0f);
	float FatigueThreshold = FMath::Clamp(BaseFatigueThreshold / FMath::Max(0.5f, Weights.Physiological), 0.3f, 1.0f);
	float IndignityThreshold = FMath::Clamp(BaseIndignityThreshold / FMath::Max(0.5f, Weights.Esteem), 0.3f, 1.0f);
	float BoredomThreshold = FMath::Clamp(BaseBoredomThreshold / FMath::Max(0.5f, Weights.SelfActualization), 0.3f, 1.0f);
	float LonelinessHighThreshold = FMath::Clamp(BaseLonelinessHigh / FMath::Max(0.5f, Weights.Belonging), 0.3f, 1.0f);
	float LonelinessLowThreshold = FMath::Clamp(BaseLonelinessLow * Weights.Belonging, 0.05f, 0.5f);

	// ========== 优先级 1: Safety (Survival) ==========
	if (MentalState->Perceived_Threat > ScaredThreshold)
	{
		return EEmotionState::Scared;
	}

	// ========== 优先级 2: Physiological (Body) ==========
	if (MentalState->Hunger > HungerThreshold)
	{
		return EEmotionState::Angry;
	}
	if (MentalState->Fatigue > FatigueThreshold)
	{
		return EEmotionState::Sad;
	}

	// ========== 优先级 3: Esteem (Ego) ==========
	// 被侮辱/挑衅 -> 愤怒
	// 高神经质/低宜人性 = 更容易愤怒
	if (MentalState->Indignity > IndignityThreshold)
	{
		return EEmotionState::Angry;
	}

	// ========== 优先级 4: Self-Actualization ==========
	// 好奇心爆棚/无聊 -> 探索欲
	if (MentalState->Boredom > BoredomThreshold)
	{
		return EEmotionState::Curious;
	}

	// ========== 优先级 5: Belonging (Social) ==========
	if (MentalState->Loneliness < LonelinessLowThreshold)
	{
		return EEmotionState::Happy;
	}
	if (MentalState->Loneliness > LonelinessHighThreshold)
	{
		return EEmotionState::Sad;
	}

	// ========== 默认: Neutral ==========
	return EEmotionState::Neutral;
}


