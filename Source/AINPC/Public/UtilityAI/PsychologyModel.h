// PsychologyModel.h
// 心理学模型数据资产 / Psychology Model Data Asset
//
// 存储 OCEAN → Maslow 的转化系数
// Stores OCEAN → Maslow transformation coefficients

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PersonalityConfig.h"
#include "MaslowWeights.h"
#include "PsychologyModel.generated.h"

/**
 * 心理学转化系数 / Psychology Transformation Coefficients
 * 
 * 定义了 OCEAN 性格如何影响马斯洛需求的敏感度
 * Defines how OCEAN personality affects sensitivity to Maslow's needs
 * 
 * 公式 / Formula:
 * Weight = OCEAN_Value × Coefficient
 * 
 * 例如 / Example:
 * ThreatWeight = Neuroticism × Neuroticism_To_Threat
 * 如果 Neuroticism = 0.8, Neuroticism_To_Threat = 3.0
 * 则 ThreatWeight = 0.8 × 3.0 = 2.4
 */
UCLASS(BlueprintType)
class AINPC_API UPsychologyModel : public UDataAsset
{
	GENERATED_BODY()

public:
	// ========== 生理层系数 (Physiological Coefficients) ==========
	
	// 尽责性 → 饥饿敏感度 (高尽责性的人更注意自己的生理需求)
	// Conscientiousness → Hunger Sensitivity
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physiological",
	          meta = (DisplayName = "Conscientiousness → Hunger", 
	                  ToolTip = "高尽责性的人更注意饥饿 / High conscientiousness people pay more attention to hunger"))
	float Conscientiousness_To_Hunger = 1.5f;

	// 神经质 → 精力敏感度 (高神经质的人更容易感到疲惫)
	// Neuroticism → Energy Sensitivity
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physiological",
	          meta = (DisplayName = "Neuroticism → Energy", 
	                  ToolTip = "高神经质的人更容易感到疲惫 / High neuroticism people tire more easily"))
	float Neuroticism_To_Energy = 2.0f;

	// ========== 安全层系数 (Safety Coefficients) ==========
	
	// 神经质 → 威胁感知 (高神经质的人更容易感到威胁)
	// Neuroticism → Threat Perception
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Safety",
	          meta = (DisplayName = "Neuroticism → Threat", 
	                  ToolTip = "高神经质的人更容易感到威胁 / High neuroticism people perceive more threats"))
	float Neuroticism_To_Threat = 3.0f;

	// 神经质 → 资源焦虑 (高神经质的人更担心资源不足)
	// Neuroticism → Resource Anxiety
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Safety",
	          meta = (DisplayName = "Neuroticism → Resource Anxiety", 
	                  ToolTip = "高神经质的人更担心资源不足 / High neuroticism people worry more about resources"))
	float Neuroticism_To_ResourceAnxiety = 2.5f;

	// 尽责性 → 资源焦虑 (高尽责性的人会提前规划资源)
	// Conscientiousness → Resource Anxiety
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Safety",
	          meta = (DisplayName = "Conscientiousness → Resource Anxiety", 
	                  ToolTip = "高尽责性的人会提前规划资源 / High conscientiousness people plan resources ahead"))
	float Conscientiousness_To_ResourceAnxiety = 1.5f;

	// ========== 社交层系数 (Social Coefficients) ==========
	
	// 外向性 → 孤独感 (高外向性的人更需要社交)
	// Extraversion → Loneliness
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social",
	          meta = (DisplayName = "Extraversion → Loneliness", 
	                  ToolTip = "高外向性的人更需要社交 / High extraversion people need more social interaction"))
	float Extraversion_To_Loneliness = 2.5f;

	// 宜人性 → 信任度 (高宜人性的人更容易信任他人)
	// Agreeableness → Trust
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social",
	          meta = (DisplayName = "Agreeableness → Trust", 
	                  ToolTip = "高宜人性的人更容易信任他人 / High agreeableness people trust others more easily"))
	float Agreeableness_To_Trust = 2.0f;

	// 神经质 → 信任度 (负向：高神经质的人更难信任他人)
	// Neuroticism → Trust (Negative)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social",
	          meta = (DisplayName = "Neuroticism → Trust (Negative)", 
	                  ToolTip = "高神经质的人更难信任他人 / High neuroticism people trust less"))
	float Neuroticism_To_Trust_Negative = -1.5f;

	// ========== 尊严层系数 (Esteem Coefficients) ==========
	
	// 神经质 → 愤怒 (高神经质的人更容易愤怒)
	// Neuroticism → Anger
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Esteem",
	          meta = (DisplayName = "Neuroticism → Anger", 
	                  ToolTip = "高神经质的人更容易愤怒 / High neuroticism people anger more easily"))
	float Neuroticism_To_Anger = 2.5f;

	// 宜人性 → 愤怒 (负向：高宜人性的人不容易愤怒)
	// Agreeableness → Anger (Negative)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Esteem",
	          meta = (DisplayName = "Agreeableness → Anger (Negative)", 
	                  ToolTip = "高宜人性的人不容易愤怒 / High agreeableness people anger less"))
	float Agreeableness_To_Anger_Negative = -1.5f;

	// 外向性 → 地位需求 (高外向性的人更在意社交地位)
	// Extraversion → Social Status
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Esteem",
	          meta = (DisplayName = "Extraversion → Social Status", 
	                  ToolTip = "高外向性的人更在意社交地位 / High extraversion people care more about social status"))
	float Extraversion_To_SocialStatus = 2.0f;

	// 神经质 → 地位需求 (高神经质的人更敏感于地位威胁)
	// Neuroticism → Social Status
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Esteem",
	          meta = (DisplayName = "Neuroticism → Social Status", 
	                  ToolTip = "高神经质的人更敏感于地位威胁 / High neuroticism people are more sensitive to status threats"))
	float Neuroticism_To_SocialStatus = 1.5f;

	// ========== 自我实现层系数 (Self-Actualization Coefficients) ==========
	
	// 尽责性 → 责任感 (高尽责性的人更重视责任)
	// Conscientiousness → Duty
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self-Actualization",
	          meta = (DisplayName = "Conscientiousness → Duty", 
	                  ToolTip = "高尽责性的人更重视责任 / High conscientiousness people value duty more"))
	float Conscientiousness_To_Duty = 2.5f;

	// 开放性 → 好奇心 (高开放性的人更好奇)
	// Openness → Curiosity
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self-Actualization",
	          meta = (DisplayName = "Openness → Curiosity", 
	                  ToolTip = "高开放性的人更好奇 / High openness people are more curious"))
	float Openness_To_Curiosity = 3.0f;

	// ========== 核心函数 (Core Function) ==========
	
	/**
	 * 根据 OCEAN 性格计算马斯洛权重
	 * Calculate Maslow weights from OCEAN personality
	 * 
	 * @param Personality - OCEAN 性格配置 / OCEAN personality configuration
	 * @return 计算出的马斯洛权重 / Calculated Maslow weights
	 */
	UFUNCTION(BlueprintCallable, Category = "Psychology")
	FMaslowWeights RecalculateWeights(const FPersonalityConfig& Personality) const;

	// 默认构造函数
	UPsychologyModel();
};
