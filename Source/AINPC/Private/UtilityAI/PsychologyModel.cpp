// PsychologyModel.cpp
// 心理学模型实现 / Psychology Model Implementation

#include "UtilityAI/PsychologyModel.h"

UPsychologyModel::UPsychologyModel()
{
	// 默认系数已在头文件中定义
	// Default coefficients are defined in the header file
}

FMaslowWeights UPsychologyModel::RecalculateWeights(const FPersonalityConfig& Personality) const
{
	FMaslowWeights Weights;

	// ========== 生理层 (Physiological) ==========
	
	// 饥饿权重 = 尽责性 × 系数
	// Hunger Weight = Conscientiousness × Coefficient
	Weights.HungerWeight = Personality.Conscientiousness * Conscientiousness_To_Hunger;

	// 精力权重 = 神经质 × 系数
	// Energy Weight = Neuroticism × Coefficient
	Weights.EnergyWeight = Personality.Neuroticism * Neuroticism_To_Energy;

	// ========== 安全层 (Safety) ==========
	
	// 威胁权重 = 神经质 × 系数
	// Threat Weight = Neuroticism × Coefficient
	Weights.ThreatWeight = Personality.Neuroticism * Neuroticism_To_Threat;

	// 资源焦虑权重 = 神经质 × 系数 + 尽责性 × 系数
	// Resource Anxiety Weight = Neuroticism × Coefficient + Conscientiousness × Coefficient
	Weights.ResourceAnxietyWeight = 
		(Personality.Neuroticism * Neuroticism_To_ResourceAnxiety) +
		(Personality.Conscientiousness * Conscientiousness_To_ResourceAnxiety);

	// ========== 社交层 (Social) ==========
	
	// 孤独感权重 = 外向性 × 系数
	// Loneliness Weight = Extraversion × Coefficient
	Weights.LonelinessWeight = Personality.Extraversion * Extraversion_To_Loneliness;

	// 信任度权重 = 宜人性 × 系数 + 神经质 × 负向系数
	// Trust Weight = Agreeableness × Coefficient + Neuroticism × Negative Coefficient
	// 注意：神经质是负向影响，所以用减法
	// Note: Neuroticism has negative impact, so we subtract
	Weights.TrustWeight = 
		(Personality.Agreeableness * Agreeableness_To_Trust) +
		(Personality.Neuroticism * Neuroticism_To_Trust_Negative);  // 这里系数已经是负数
	
	// 确保权重不为负
	// Ensure weight is not negative
	Weights.TrustWeight = FMath::Max(0.1f, Weights.TrustWeight);

	// ========== 尊严层 (Esteem) ==========
	
	// 愤怒权重 = 神经质 × 系数 + 宜人性 × 负向系数
	// Anger Weight = Neuroticism × Coefficient + Agreeableness × Negative Coefficient
	Weights.AngerWeight = 
		(Personality.Neuroticism * Neuroticism_To_Anger) +
		(Personality.Agreeableness * Agreeableness_To_Anger_Negative);  // 这里系数已经是负数
	
	// 确保权重不为负
	// Ensure weight is not negative
	Weights.AngerWeight = FMath::Max(0.1f, Weights.AngerWeight);

	// 地位需求权重 = 外向性 × 系数 + 神经质 × 系数
	// Social Status Weight = Extraversion × Coefficient + Neuroticism × Coefficient
	Weights.SocialStatusWeight = 
		(Personality.Extraversion * Extraversion_To_SocialStatus) +
		(Personality.Neuroticism * Neuroticism_To_SocialStatus);

	// ========== 自我实现层 (Self-Actualization) ==========
	
	// 责任感权重 = 尽责性 × 系数
	// Duty Weight = Conscientiousness × Coefficient
	Weights.DutyWeight = Personality.Conscientiousness * Conscientiousness_To_Duty;

	// 好奇心权重 = 开放性 × 系数
	// Curiosity Weight = Openness × Coefficient
	Weights.CuriosityWeight = Personality.Openness * Openness_To_Curiosity;

	// ========== 调试日志 (Debug Log) ==========
	
	UE_LOG(LogTemp, Log, TEXT("=== RecalculateWeights ==="));
	UE_LOG(LogTemp, Log, TEXT("Input OCEAN: O=%.2f, C=%.2f, E=%.2f, A=%.2f, N=%.2f"),
		Personality.Openness, Personality.Conscientiousness, Personality.Extraversion,
		Personality.Agreeableness, Personality.Neuroticism);
	
	UE_LOG(LogTemp, Log, TEXT("Output Weights:"));
	UE_LOG(LogTemp, Log, TEXT("  Physiological: Hunger=%.2f, Energy=%.2f"),
		Weights.HungerWeight, Weights.EnergyWeight);
	UE_LOG(LogTemp, Log, TEXT("  Safety: Threat=%.2f, ResourceAnxiety=%.2f"),
		Weights.ThreatWeight, Weights.ResourceAnxietyWeight);
	UE_LOG(LogTemp, Log, TEXT("  Social: Loneliness=%.2f, Trust=%.2f"),
		Weights.LonelinessWeight, Weights.TrustWeight);
	UE_LOG(LogTemp, Log, TEXT("  Esteem: Anger=%.2f, SocialStatus=%.2f"),
		Weights.AngerWeight, Weights.SocialStatusWeight);
	UE_LOG(LogTemp, Log, TEXT("  Self-Actualization: Duty=%.2f, Curiosity=%.2f"),
		Weights.DutyWeight, Weights.CuriosityWeight);

	return Weights;
}
