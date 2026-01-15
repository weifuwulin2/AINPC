// EmotionMatrixConfig.cpp
// 情绪矩阵配置实现 / Emotion Matrix Config Implementation

#include "UtilityAI/EmotionMatrixConfig.h"
#include "Social/SocialGameplayTags.h"

float FEmotionMatrixRow::GetMultiplier(const FGameplayTag& ActivityTag) const
{
	// 根据 Tag 返回对应的乘数
	// Return corresponding multiplier based on tag
	
	if (ActivityTag.MatchesTag(AINPCTags::Activity_Combat))
	{
		return Combat;
	}
	if (ActivityTag.MatchesTag(AINPCTags::Activity_Flee))
	{
		return Flee;
	}
	if (ActivityTag.MatchesTag(AINPCTags::Activity_Social))
	{
		return Social;
	}
	if (ActivityTag.MatchesTag(AINPCTags::Activity_Investigate))
	{
		return Investigate;
	}
	if (ActivityTag.MatchesTag(AINPCTags::Activity_Rest))
	{
		return Rest;
	}
	if (ActivityTag.MatchesTag(AINPCTags::Activity_Avoid))
	{
		return Avoid;
	}
	if (ActivityTag.MatchesTag(AINPCTags::Activity_Work))
	{
		return Work;
	}
	if (ActivityTag.MatchesTag(AINPCTags::Activity_Eat))
	{
		return Eat;
	}
	
	// 默认乘数为 1.0 (无影响)
	// Default multiplier is 1.0 (no effect)
	return 1.0f;
}
