// EmojiConfigDataAsset.cpp
// Emoji 配置数据资产实现 / Emoji Configuration Data Asset Implementation

#include "Config/EmojiConfigDataAsset.h"

UTexture2D* UEmojiConfigDataAsset::GetTextureForEmotion(const FString& EmotionTag) const
{
	// 遍历所有配置，查找匹配的情绪标签
	// Iterate through all configs to find matching emotion tag
	for (const FEmojiConfig& Config : EmojiConfigs)
	{
		if (Config.EmotionTag.Equals(EmotionTag, ESearchCase::IgnoreCase))
		{
			return Config.Texture;
		}
	}
	
	// 如果没找到，返回 nullptr
	// If not found, return nullptr
	return nullptr;
}

UTexture2D* UEmojiConfigDataAsset::GetDefaultTexture() const
{
	// 尝试获取 "Neutral" 情绪的纹理
	// Try to get texture for "Neutral" emotion
	return GetTextureForEmotion(TEXT("Neutral"));
}
