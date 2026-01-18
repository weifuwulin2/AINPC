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

	// Helper lambda for score calculation (Winner Takes All)
	// 辅助Lambda：分数计算 (赢家通吃)
	// Formula: Score = (Value - AdjustedThreshold) * Weight
	// 只有当 Value > Threshold 时才有分 (Score > 0)，否则为 0
	auto GetScore = [](float Value, float Threshold, float Weight) -> float
	{
		return FMath::Max(0.0f, (Value - Threshold) * Weight);
	};

	float MaxScore = 0.0f;
	EEmotionState BestEmotion = EEmotionState::Neutral;

	// ========== 1. Safety (Survival) -> Scared ==========
	float ScaredScore = GetScore(MentalState->Perceived_Threat, ScaredThreshold, Weights.Safety);
	if (ScaredScore > MaxScore)
	{
		MaxScore = ScaredScore;
		BestEmotion = EEmotionState::Scared;
	}

	// ========== 2. Physiological (Body) -> Angry / Sad ==========
	// Hunger -> Angry
	float HungerScore = GetScore(MentalState->Hunger, HungerThreshold, Weights.Physiological);
	if (HungerScore > MaxScore)
	{
		MaxScore = HungerScore;
		BestEmotion = EEmotionState::Angry;
	}

	// Fatigue -> Sad
	float FatigueScore = GetScore(MentalState->Fatigue, FatigueThreshold, Weights.Physiological);
	if (FatigueScore > MaxScore)
	{
		MaxScore = FatigueScore;
		BestEmotion = EEmotionState::Sad;
	}

	// ========== 3. Esteem (Ego) -> Angry ==========
	// Indignity -> Angry
	float IndignityScore = GetScore(MentalState->Indignity, IndignityThreshold, Weights.Esteem);
	if (IndignityScore > MaxScore)
	{
		MaxScore = IndignityScore;
		BestEmotion = EEmotionState::Angry;
	}

	// ========== 4. Self-Actualization -> Curious ==========
	// Boredom -> Curious
	float BoredomScore = GetScore(MentalState->Boredom, BoredomThreshold, Weights.SelfActualization);
	if (BoredomScore > MaxScore)
	{
		MaxScore = BoredomScore;
		BestEmotion = EEmotionState::Curious;
	}

	// ========== 5. Belonging (Social) -> Happy / Sad ==========
	// Loneliness High -> Sad
	float LonelinessHighScore = GetScore(MentalState->Loneliness, LonelinessHighThreshold, Weights.Belonging);
	if (LonelinessHighScore > MaxScore)
	{
		MaxScore = LonelinessHighScore;
		BestEmotion = EEmotionState::Sad;
	}

	// Loneliness Low -> Happy (Inverted Logic)
	// 越孤独越不开心，越不孤独(Value < Threshold)越开心
	// Score = (Threshold - Value) * Weight
	if (MentalState->Loneliness < LonelinessLowThreshold)
	{
		float HappyScore = (LonelinessLowThreshold - MentalState->Loneliness) * Weights.Belonging;
		if (HappyScore > MaxScore)
		{
			MaxScore = HappyScore;
			BestEmotion = EEmotionState::Happy;
		}
	}

	// ✅ 更新 MentalState 的情绪分数 (用于后续衰减)
	// Update MentalState's emotion score (for decay)
	if (UNPCMentalState* WritableState = const_cast<UNPCMentalState*>(MentalState))
	{
		WritableState->CurrentEmotionScore = MaxScore;
	}

	// ✅ 如果情绪分数太低，返回 Neutral（需求基本满足）
	// If emotion score is too low, return Neutral (needs are mostly satisfied)
	const float MinEmotionThreshold = 0.3f;
	if (MaxScore < MinEmotionThreshold)
	{
		return EEmotionState::Neutral;
	}

	return BestEmotion;
}


