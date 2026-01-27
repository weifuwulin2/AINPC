// PersonalityConfig.h
// OCEAN 大五人格配置 / OCEAN Big Five Personality Configuration
// 
// 这是 NPC 的"出厂设置"，游戏运行时不会改变
// This is the NPC's "factory settings", will not change during runtime

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Social/SocialTypes.h"
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

	// ========== 角色定义 (Role Definition) ==========
	
	// Deprecated: Faction is now handled by NPCDefinitionComponent
	// EFactionType Faction = EFactionType::Neutral;
	
	// 角色描述 / Role Description
	// 定义此 NPC 的身份和基本行为倾向，用于 LLM 上下文
	// Defines this NPC's identity and basic behavioral tendencies for LLM context
	// Example: "You are a hostile zombie that attacks humans on sight"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role Definition",
	          meta = (DisplayName = "Role Description (角色描述)", 
	                  ToolTip = "NPC 的身份和基本行为倾向 / NPC's identity and basic behavioral tendencies",
	                  MultiLine = true))
	FString RoleDescription = TEXT("You are a neutral NPC");
	
	// 行为准则 / Behavioral Guidelines
	// 定义此 NPC 的具体行为规则和反应模式
	// Defines this NPC's specific behavioral rules and reaction patterns
	// Example: "Attack any human on sight. Flee if health is below 30%."
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role Definition",
	          meta = (DisplayName = "Behavioral Guidelines (行为准则)", 
	                  ToolTip = "NPC 的具体行为规则和反应模式 / NPC's specific behavioral rules and reaction patterns",
	                  MultiLine = true))
	FString BehavioralGuidelines = TEXT("");

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
		Config.Openness = 0.9f;      // High Openness = Strong Curiosity
		Config.Conscientiousness = 0.7f;
		Config.Extraversion = 0.2f;  // Low Extraversion = Dislikes socializing
		Config.Agreeableness = 0.5f;
		Config.Neuroticism = 0.5f;
		return Config;
	}

	// 狡诈的政客 / Scheming Politician
	static FPersonalityConfig SchemingPolitician()
	{
		FPersonalityConfig Config;
		Config.Openness = 0.8f;      // Creative in plotting
		Config.Conscientiousness = 0.7f; // Disciplined in ambition
		Config.Extraversion = 0.8f;  // Charming
		Config.Agreeableness = 0.2f; // Low Trust/Empathy
		Config.Neuroticism = 0.6f;   // Paranoid
		return Config;
	}

	// 狂热的教徒 / Fanatic Cultist
	static FPersonalityConfig FanaticCultist()
	{
		FPersonalityConfig Config;
		Config.Openness = 0.2f;      // Closed to new ideas (Dogmatic)
		Config.Conscientiousness = 0.9f; // Obsessively dedicated
		Config.Extraversion = 0.5f;
		Config.Agreeableness = 0.4f; // Only agreeable to 'us'
		Config.Neuroticism = 0.8f;   // Highly volatile
		return Config;
	}

	// 醉鬼 / Drunkard
	static FPersonalityConfig Drunkard()
	{
		FPersonalityConfig Config;
		Config.Openness = 0.7f;      // Inhibitions lowered
		Config.Conscientiousness = 0.1f; // Cannot focus
		Config.Extraversion = 0.6f;  // Loud
		Config.Agreeableness = 0.5f; // Unpredictable
		Config.Neuroticism = 0.7f;   // Emotional swings
		return Config;
	}

	// 天真的孩子 / Innocent Child
	static FPersonalityConfig InnocentChild()
	{
		FPersonalityConfig Config;
		Config.Openness = 0.9f;      // Pure curiosity
		Config.Conscientiousness = 0.3f; // Short attention span
		Config.Extraversion = 0.7f;  // Playful
		Config.Agreeableness = 0.9f; // Trusting
		Config.Neuroticism = 0.6f;   // Easily scared/upset
		return Config;
	}

	// 生成基于 OCEAN 值的文字描述 (5-Tier Granularity)
	// Generate text description based on OCEAN values
	FString GetOCEANDescription() const
	{
		TArray<FString> Traits;

		// Lambda helper for 5-tier mapping
		auto GetTrait = [](float Val, const FString& VeryHigh, const FString& High, const FString& Mid, const FString& Low, const FString& VeryLow) -> FString
		{
			if (Val >= 0.8f) return VeryHigh;
			if (Val >= 0.6f) return High;
			if (Val >= 0.4f) return Mid;
			if (Val >= 0.2f) return Low;
			return VeryLow;
		};

		// 1. Openness (Imagination, Curiosity)
		Traits.Add(GetTrait(Openness, 
			TEXT("Deeply Philosophical & Abstract"), // >0.8
			TEXT("Curious & Inventive"),             // >0.6
			TEXT("Practical but Open-minded"),       // 0.4-0.6
			TEXT("Down-to-earth & Conventional"),    // >0.2
			TEXT("Rigidly Traditional")              // <0.2
		));

		// 2. Conscientiousness (Discipline, Order)
		Traits.Add(GetTrait(Conscientiousness, 
			TEXT("Highly Disciplined & Perfectionist"), 
			TEXT("Organized & Reliable"), 
			TEXT("Reasonably Dependable"), 
			TEXT("Disorganized & Spontaneous"), 
			TEXT("Careless & Impulsive")
		));

		// 3. Extraversion (Social Energy)
		Traits.Add(GetTrait(Extraversion, 
			TEXT("Life of the Party & Thrill-seeker"), 
			TEXT("Outgoing & Sociable"), 
			TEXT("Moderately Social"), 
			TEXT("Reserved & Private"), 
			TEXT("Solitary & Reclusive")
		));

		// 4. Agreeableness (Trust, Empathy)
		Traits.Add(GetTrait(Agreeableness, 
			TEXT("Selfless & Altruistic"), 
			TEXT("Compassionate & Trusting"), 
			TEXT("Negotiator & Pragmatic"), 
			TEXT("Competitive & Skeptical"), 
			TEXT("Hostile & Manipulative")
		));
		
		// 5. Neuroticism (Anxiety, Stability)
		Traits.Add(GetTrait(Neuroticism, 
			TEXT("Extremely Volatile & Anxious"), 
			TEXT("Sensitive & Reactive"), 
			TEXT("Generally Stable"), 
			TEXT("Calm & Composed"), 
			TEXT("Unshakeable & Stoic")
		));

		return FString::Join(Traits, TEXT(", "));
	}
};
