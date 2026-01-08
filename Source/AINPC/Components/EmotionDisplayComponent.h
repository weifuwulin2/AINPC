// EmotionDisplayComponent.h
// 情绪显示组件 / Emotion Display Component
//
// 负责在 NPC 头顶显示 emoji 和对话泡泡
// Responsible for displaying emoji and speech bubbles above NPC's head

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EmotionDisplayComponent.generated.h"

// 前置声明 / Forward declarations
class UWidgetComponent;
class UUserWidget;

/**
 * 情绪显示组件 / Emotion Display Component
 * 
 * 监听 CognitionComponent 的情绪变化，显示对应的 emoji 和对话泡泡
 * Listens to CognitionComponent's emotion changes and displays corresponding emoji and speech bubbles
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AINPC_API UEmotionDisplayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEmotionDisplayComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ========== 配置 (Configuration) ==========
	
	/**
	 * Emoji Widget 类 / Emoji Widget Class
	 * 
	 * 用于显示 emoji 的 UMG Widget
	 * UMG Widget used to display emoji
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion Display")
	TSubclassOf<UUserWidget> EmojiWidgetClass;
	
	/**
	 * 对话泡泡 Widget 类 / Speech Bubble Widget Class
	 * 
	 * 用于显示对话泡泡的 UMG Widget
	 * UMG Widget used to display speech bubble
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion Display")
	TSubclassOf<UUserWidget> SpeechBubbleWidgetClass;
	
	/**
	 * Emoji 显示时长 / Emoji Display Duration
	 * 
	 * Emoji 显示多久后自动隐藏（秒）
	 * How long the emoji is displayed before auto-hiding (seconds)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion Display")
	float EmojiDisplayDuration = 3.0f;
	
	/**
	 * 对话泡泡显示时长 / Speech Bubble Display Duration
	 * 
	 * 对话泡泡显示多久后自动隐藏（秒）
	 * How long the speech bubble is displayed before auto-hiding (seconds)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion Display")
	float SpeechBubbleDisplayDuration = 5.0f;
	
	/**
	 * Widget 显示高度偏移 / Widget Height Offset
	 * 
	 * Widget 在角色头顶的高度偏移（厘米）
	 * Height offset above character's head (centimeters)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion Display")
	float HeightOffset = 100.0f;
	
	// ========== 公共接口 (Public Interface) ==========
	
	/**
	 * 显示情绪 emoji / Display Emotion Emoji
	 * 
	 * @param Emotion - 情绪标签 / Emotion tag (e.g., "Happy", "Angry")
	 */
	UFUNCTION(BlueprintCallable, Category = "Emotion Display")
	void ShowEmotion(const FString& Emotion);
	
	/**
	 * 显示对话泡泡 / Display Speech Bubble
	 * 
	 * @param Message - 对话内容 / Speech content
	 */
	UFUNCTION(BlueprintCallable, Category = "Emotion Display")
	void ShowSpeechBubble(const FString& Message);
	
	/**
	 * 隐藏 emoji / Hide Emoji
	 */
	UFUNCTION(BlueprintCallable, Category = "Emotion Display")
	void HideEmotion();
	
	/**
	 * 隐藏对话泡泡 / Hide Speech Bubble
	 */
	UFUNCTION(BlueprintCallable, Category = "Emotion Display")
	void HideSpeechBubble();
	
	/**
	 * 根据情绪获取 emoji 文本 / Get Emoji Text by Emotion
	 * 
	 * @param Emotion - 情绪标签 / Emotion tag
	 * @return Emoji 文本 / Emoji text
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Emotion Display")
	FString GetEmojiForEmotion(const FString& Emotion) const;
	
	/**
	 * 根据情绪获取对话内容 / Get Message by Emotion
	 * 
	 * @param Emotion - 情绪标签 / Emotion tag
	 * @return 对话内容 / Message text
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Emotion Display")
	FString GetMessageForEmotion(const FString& Emotion) const;
	
	/**
	 * 根据情绪获取 emoji 纹理 / Get Emoji Texture by Emotion
	 * 
	 * @param Emotion - 情绪标签 / Emotion tag
	 * @return Emoji 纹理 / Emoji texture
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Emotion Display")
	UTexture2D* GetEmojiTextureForEmotion(const FString& Emotion) const;

protected:
	// ========== Emoji 配置 (Emoji Configuration) ==========
	
	/**
	 * Emoji 配置数据资产 / Emoji Config Data Asset
	 * 
	 * 存储所有 emoji 的纹理映射
	 * Stores all emoji texture mappings
	 * 
	 * 在 Blueprint 中设置，指向 DA_EmojiConfig 资产
	 * Set in Blueprint, pointing to DA_EmojiConfig asset
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion Display")
	class UEmojiConfigDataAsset* EmojiConfigAsset;

protected:
	// ========== 内部组件 (Internal Components) ==========
	
	/**
	 * Emoji Widget 组件 / Emoji Widget Component
	 */
	UPROPERTY()
	UWidgetComponent* EmojiWidgetComponent;
	
	/**
	 * 对话泡泡 Widget 组件 / Speech Bubble Widget Component
	 */
	UPROPERTY()
	UWidgetComponent* SpeechBubbleWidgetComponent;
	
	// ========== 定时器 (Timers) ==========
	
	/**
	 * Emoji 自动隐藏定时器 / Emoji Auto-Hide Timer
	 */
	FTimerHandle EmojiHideTimerHandle;
	
	/**
	 * 对话泡泡自动隐藏定时器 / Speech Bubble Auto-Hide Timer
	 */
	FTimerHandle SpeechBubbleHideTimerHandle;
	
	// ========== 辅助函数 (Helper Functions) ==========
	
	/**
	 * 创建 Widget 组件 / Create Widget Component
	 */
	void CreateWidgetComponents();
	
	/**
	 * 绑定到 CognitionComponent 的事件 / Bind to CognitionComponent Events
	 */
	void BindToCognitionEvents();
	
	/**
	 * 当情绪变化时的回调 / Callback when emotion changes
	 */
	UFUNCTION()
	void OnEmotionChanged(const FMentalState& NewState);
};
