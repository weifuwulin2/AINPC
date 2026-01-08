// EmojiConfigDataAsset.h
// Emoji 配置数据资产 / Emoji Configuration Data Asset
//
// 存储所有 emoji 的纹理映射
// Stores all emoji texture mappings

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EmojiConfigDataAsset.generated.h"

/**
 * Emoji 配置条目 / Emoji Config Entry
 * 
 * 单个 emoji 的配置
 * Configuration for a single emoji
 */
USTRUCT(BlueprintType)
struct FEmojiConfig
{
	GENERATED_BODY()

	// 情绪标签 / Emotion Tag
	// 例如: "Happy", "Angry", "Scared"
	// Example: "Happy", "Angry", "Scared"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emoji")
	FString EmotionTag;

	// Emoji 纹理 / Emoji Texture
	// 对应的图片资源
	// Corresponding image asset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emoji")
	UTexture2D* Texture;

	FEmojiConfig()
		: EmotionTag(TEXT("Neutral"))
		, Texture(nullptr)
	{
	}
};

/**
 * Emoji 配置数据资产 / Emoji Configuration Data Asset
 * 
 * 存储所有 emoji 的纹理映射
 * Stores all emoji texture mappings
 * 
 * 使用方法 / Usage:
 * 1. 在编辑器中创建此 DataAsset
 * 2. 添加所有 emoji 配置（Happy, Angry, Scared 等）
 * 3. 在 EmotionDisplayComponent 中引用此 DataAsset
 */
UCLASS(BlueprintType)
class AINPC_API UEmojiConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Emoji 配置列表 / Emoji Config List
	 * 
	 * 所有可用的 emoji 配置
	 * All available emoji configurations
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emoji")
	TArray<FEmojiConfig> EmojiConfigs;

	/**
	 * 根据情绪标签获取纹理 / Get Texture by Emotion Tag
	 * 
	 * @param EmotionTag - 情绪标签 / Emotion tag
	 * @return 对应的纹理，如果没找到返回 nullptr / Corresponding texture, or nullptr if not found
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Emoji")
	UTexture2D* GetTextureForEmotion(const FString& EmotionTag) const;

	/**
	 * 获取默认纹理（Neutral）/ Get Default Texture (Neutral)
	 * 
	 * @return Neutral 情绪的纹理 / Texture for Neutral emotion
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Emoji")
	UTexture2D* GetDefaultTexture() const;
};
