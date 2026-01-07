// PersonalityConfig.h
// OCEAN 大五人格配置 / OCEAN Big Five Personality Configuration
// 
// 这是 NPC 的"出厂设置"，游戏运行时不会改变
// This is the NPC's "factory settings", will not change during runtime

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PersonalityConfig.generated.h"

/**
 * OCEAN 大五人格模型 / OCEAN Big Five Personality Model
 * 
 * 每个值范围 0.0 - 1.0
 * Each value ranges from 0.0 to 1.0
 * 
 * - Openness (开放性): 好奇心、想象力、创造力
 * - Conscientiousness (尽责性): 责任感、自律、目标导向
 * - Extraversion (外向性): 社交热情、活力、主动性
 * - Agreeableness (宜人性): 合作、信任、同理心
 * - Neuroticism (神经质): 情绪不稳定、焦虑、敏感
 */
USTRUCT(BlueprintType)
struct FPersonalityConfig : public FTableRowBase
{
	GENERATED_BODY()

	// 开放性 (Openness): 影响好奇心、探索欲望
	// Openness: Affects curiosity and exploration desire
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCEAN Personality",
	          meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Openness (开放性)", 
	                  ToolTip = "好奇心、想象力、创造力 / Curiosity, imagination, creativity"))
	float Openness = 0.5f;

	// 尽责性 (Conscientiousness): 影响责任感、执行任务的动力
	// Conscientiousness: Affects sense of duty and task execution motivation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCEAN Personality",
	          meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Conscientiousness (尽责性)", 
	                  ToolTip = "责任感、自律、目标导向 / Responsibility, self-discipline, goal-oriented"))
	float Conscientiousness = 0.5f;

	// 外向性 (Extraversion): 影响社交需求、主动性
	// Extraversion: Affects social needs and proactivity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCEAN Personality",
	          meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Extraversion (外向性)", 
	                  ToolTip = "社交热情、活力、主动性 / Social enthusiasm, energy, initiative"))
	float Extraversion = 0.5f;

	// 宜人性 (Agreeableness): 影响信任度、合作倾向
	// Agreeableness: Affects trust and cooperation tendency
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCEAN Personality",
	          meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Agreeableness (宜人性)", 
	                  ToolTip = "合作、信任、同理心 / Cooperation, trust, empathy"))
	float Agreeableness = 0.5f;

	// 神经质 (Neuroticism): 影响威胁感知、焦虑水平
	// Neuroticism: Affects threat perception and anxiety level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCEAN Personality",
	          meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Neuroticism (神经质)", 
	                  ToolTip = "情绪不稳定、焦虑、敏感 / Emotional instability, anxiety, sensitivity"))
	float Neuroticism = 0.5f;

	// 构造函数：默认为"平衡型"人格
	// Constructor: Default to "balanced" personality
	FPersonalityConfig()
		: Openness(0.5f)
		, Conscientiousness(0.5f)
		, Extraversion(0.5f)
		, Agreeableness(0.5f)
		, Neuroticism(0.5f)
	{}

	// 预设人格模板 / Preset Personality Templates
	
	// 勇敢的战士 / Brave Warrior
	static FPersonalityConfig BraveWarrior()
	{
		FPersonalityConfig Config;
		Config.Openness = 0.4f;
		Config.Conscientiousness = 0.8f;
		Config.Extraversion = 0.7f;
		Config.Agreeableness = 0.5f;
		Config.Neuroticism = 0.2f;  // 低神经质 = 不容易害怕
		return Config;
	}

	// 谨慎的守卫 / Cautious Guard
	static FPersonalityConfig CautiousGuard()
	{
		FPersonalityConfig Config;
		Config.Openness = 0.3f;
		Config.Conscientiousness = 0.9f;
		Config.Extraversion = 0.4f;
		Config.Agreeableness = 0.6f;
		Config.Neuroticism = 0.6f;  // 高神经质 = 更警惕
		return Config;
	}

	// 友好的商人 / Friendly Merchant
	static FPersonalityConfig FriendlyMerchant()
	{
		FPersonalityConfig Config;
		Config.Openness = 0.6f;
		Config.Conscientiousness = 0.7f;
		Config.Extraversion = 0.9f;  // 高外向性 = 喜欢社交
		Config.Agreeableness = 0.8f;
		Config.Neuroticism = 0.3f;
		return Config;
	}

	// 孤僻的学者 / Reclusive Scholar
	static FPersonalityConfig ReclusiveScholar()
	{
		FPersonalityConfig Config;
		Config.Openness = 0.9f;      // 高开放性 = 好奇心强
		Config.Conscientiousness = 0.7f;
		Config.Extraversion = 0.2f;  // 低外向性 = 不喜欢社交
		Config.Agreeableness = 0.5f;
		Config.Neuroticism = 0.5f;
		return Config;
	}
};
