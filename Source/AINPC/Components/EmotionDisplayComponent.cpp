// EmotionDisplayComponent.cpp
// 情绪显示组件实现 / Emotion Display Component Implementation

#include "Components/EmotionDisplayComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CognitionComponent.h"
#include "Controller/UtilityAIController.h"
#include "LLM/LLMCommunicator.h"  // For FMentalState definition
#include "Config/EmojiConfigDataAsset.h"  // For emoji texture configuration
#include "AIController.h"  // For Cast<AAIController>
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"  // For UImage widget
#include "GameFramework/Character.h"
#include "TimerManager.h"

UEmotionDisplayComponent::UEmotionDisplayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEmotionDisplayComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 创建 Widget 组件
	// Create Widget Components
	CreateWidgetComponents();
	
	// 绑定到 CognitionComponent 的事件
	// Bind to CognitionComponent Events
	BindToCognitionEvents();
	
	// 测试：2秒后显示一个测试 emoji
	// Test: Show a test emoji after 2 seconds
	if (GetWorld())
	{
		FTimerHandle TestTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			TestTimerHandle,
			[this]()
			{
				UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] TEST: Showing Happy emoji"));
				ShowEmotion(TEXT("Happy"));
			},
			2.0f,
			false
		);
	}
}


void UEmotionDisplayComponent::CreateWidgetComponents()
{
	// Owner 是 AI Controller，需要获取它控制的 Pawn
	// Owner is AI Controller, need to get the controlled Pawn
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] Owner is not an AIController!"));
		return;
	}
	
	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] No Pawn controlled yet, will retry later"));
		return;
	}
	
	// 获取 Pawn 的根组件
	// Get Pawn's root component
	USceneComponent* RootComp = ControlledPawn->GetRootComponent();
	if (!RootComp)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] Pawn has no root component!"));
		return;
	}
	
	// 创建 Emoji Widget 组件（附加到 Pawn，不是 Controller）
	// Create Emoji Widget Component (attach to Pawn, not Controller)
	EmojiWidgetComponent = NewObject<UWidgetComponent>(ControlledPawn, UWidgetComponent::StaticClass(), TEXT("EmojiWidget"));
	if (EmojiWidgetComponent)
	{
		EmojiWidgetComponent->RegisterComponent();
		EmojiWidgetComponent->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
		EmojiWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HeightOffset));
		EmojiWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		EmojiWidgetComponent->SetDrawSize(FVector2D(100.0f, 100.0f));
		EmojiWidgetComponent->SetVisibility(false); // 初始隐藏 / Initially hidden
		
		if (EmojiWidgetClass)
		{
			EmojiWidgetComponent->SetWidgetClass(EmojiWidgetClass);
		}
		
		UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] Emoji Widget Component created and attached to Pawn: %s"), *ControlledPawn->GetName());
	}
	
	// 创建对话泡泡 Widget 组件（附加到 Pawn）
	// Create Speech Bubble Widget Component (attach to Pawn)
	SpeechBubbleWidgetComponent = NewObject<UWidgetComponent>(ControlledPawn, UWidgetComponent::StaticClass(), TEXT("SpeechBubbleWidget"));
	if (SpeechBubbleWidgetComponent)
	{
		SpeechBubbleWidgetComponent->RegisterComponent();
		SpeechBubbleWidgetComponent->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
		SpeechBubbleWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HeightOffset + 50.0f)); // 比 emoji 高一点
		SpeechBubbleWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		SpeechBubbleWidgetComponent->SetDrawSize(FVector2D(300.0f, 100.0f));
		SpeechBubbleWidgetComponent->SetVisibility(false); // 初始隐藏 / Initially hidden
		
		if (SpeechBubbleWidgetClass)
		{
			SpeechBubbleWidgetComponent->SetWidgetClass(SpeechBubbleWidgetClass);
		}
		
		UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] Speech Bubble Widget Component created and attached to Pawn: %s"), *ControlledPawn->GetName());
	}
}

void UEmotionDisplayComponent::BindToCognitionEvents()
{
	// 获取 AI Controller
	// Get AI Controller
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (!AIController) return;
	
	AUtilityAIController* UtilityController = Cast<AUtilityAIController>(AIController);
	if (!UtilityController || !UtilityController->CognitionComp) return;
	
	// 绑定到 OnMentalStateChanged 事件
	// Bind to OnMentalStateChanged event
	UtilityController->CognitionComp->OnMentalStateChanged.AddDynamic(this, &UEmotionDisplayComponent::OnEmotionChanged);
	
	UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] Bound to CognitionComponent events"));
}

void UEmotionDisplayComponent::OnEmotionChanged(const FMentalState& NewState)
{
	// 当情绪变化时，显示对应的 emoji
	// When emotion changes, display corresponding emoji
	if (!NewState.Emotion.IsEmpty() && NewState.Emotion != TEXT("Neutral"))
	{
		ShowEmotion(NewState.Emotion);
	}
	
	// 显示 LLM 生成的对话内容
	// Display LLM-generated speech content
	if (!NewState.Speech.IsEmpty())
	{
		ShowSpeechBubble(NewState.Speech);
	}
}

void UEmotionDisplayComponent::ShowEmotion(const FString& Emotion)
{
	if (!EmojiWidgetComponent) return;
	
	// 更新 Widget 内容
	// Update Widget content
	UUserWidget* Widget = EmojiWidgetComponent->GetWidget();
	if (Widget)
	{
		// 使用 Image widget 显示 emoji 图片
		// Use Image widget to display emoji texture
		UImage* EmojiImage = Cast<UImage>(Widget->GetWidgetFromName(TEXT("EmojiImage")));
		if (EmojiImage)
		{
			// 获取对应的 emoji 纹理
			// Get corresponding emoji texture
			UTexture2D* EmojiTexture = GetEmojiTextureForEmotion(Emotion);
			if (EmojiTexture)
			{
				EmojiImage->SetBrushFromTexture(EmojiTexture);
				UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] Set emoji texture for emotion: %s"), *Emotion);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] No texture found for emotion: %s"), *Emotion);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] EmojiImage widget not found in WBP_EmojiDisplay"));
		}
	}
	
	// 显示 Widget
	// Show Widget
	EmojiWidgetComponent->SetVisibility(true);
	
	UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] Showing emotion: %s"), *Emotion);
	
	// 清除之前的定时器
	// Clear previous timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(EmojiHideTimerHandle);
		
		// 设置新的定时器，自动隐藏
		// Set new timer to auto-hide
		GetWorld()->GetTimerManager().SetTimer(
			EmojiHideTimerHandle,
			this,
			&UEmotionDisplayComponent::HideEmotion,
			EmojiDisplayDuration,
			false
		);
	}
}

void UEmotionDisplayComponent::ShowSpeechBubble(const FString& Message)
{
	if (!SpeechBubbleWidgetComponent) return;
	
	// 更新 Widget 内容
	// Update Widget content
	UUserWidget* Widget = SpeechBubbleWidgetComponent->GetWidget();
	if (Widget)
	{
		// 假设 Widget 有一个名为 "MessageText" 的 TextBlock
		// Assume Widget has a TextBlock named "MessageText"
		UTextBlock* MessageTextBlock = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("MessageText")));
		if (MessageTextBlock)
		{
			MessageTextBlock->SetText(FText::FromString(Message));
		}
	}
	
	// 显示 Widget
	// Show Widget
	SpeechBubbleWidgetComponent->SetVisibility(true);
	
	UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] Showing speech bubble: %s"), *Message);
	
	// 清除之前的定时器
	// Clear previous timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SpeechBubbleHideTimerHandle);
		
		// 设置新的定时器，自动隐藏
		// Set new timer to auto-hide
		GetWorld()->GetTimerManager().SetTimer(
			SpeechBubbleHideTimerHandle,
			this,
			&UEmotionDisplayComponent::HideSpeechBubble,
			SpeechBubbleDisplayDuration,
			false
		);
	}
}

void UEmotionDisplayComponent::HideEmotion()
{
	if (EmojiWidgetComponent)
	{
		EmojiWidgetComponent->SetVisibility(false);
		UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] Hiding emotion"));
	}
}

void UEmotionDisplayComponent::HideSpeechBubble()
{
	if (SpeechBubbleWidgetComponent)
	{
		SpeechBubbleWidgetComponent->SetVisibility(false);
		UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] Hiding speech bubble"));
	}
}

FString UEmotionDisplayComponent::GetEmojiForEmotion(const FString& Emotion) const
{
	// Emoji 映射表 / Emoji Mapping Table
	// 根据情绪标签返回对应的 emoji
	// Return corresponding emoji based on emotion tag
	
	if (Emotion == TEXT("Happy"))
	{
		return TEXT("😊");
	}
	else if (Emotion == TEXT("Angry"))
	{
		return TEXT("😠");
	}
	else if (Emotion == TEXT("Scared"))
	{
		return TEXT("😨");
	}
	else if (Emotion == TEXT("Sad"))
	{
		return TEXT("😢");
	}
	else if (Emotion == TEXT("Confused"))
	{
		return TEXT("😕");
	}
	else if (Emotion == TEXT("Excited"))
	{
		return TEXT("😃");
	}
	else if (Emotion == TEXT("Neutral"))
	{
		return TEXT("😐");
	}
	else if (Emotion == TEXT("Suspicious"))
	{
		return TEXT("🤨");
	}
	else if (Emotion == TEXT("Curious"))
	{
		return TEXT("🤔");
	}
	else if (Emotion == TEXT("Tired"))
	{
		return TEXT("😴");
	}
	else
	{
		// 默认 emoji / Default emoji
		return TEXT("😐");
	}
}

FString UEmotionDisplayComponent::GetMessageForEmotion(const FString& Emotion) const
{
	// 对话内容映射表 / Message Mapping Table
	// 根据情绪标签返回对应的对话内容
	// Return corresponding message based on emotion tag
	
	if (Emotion == TEXT("Happy"))
	{
		return TEXT("I'm feeling great!");
		// return TEXT("我感觉很好！");
	}
	else if (Emotion == TEXT("Angry"))
	{
		return TEXT("I'm so angry!");
		// return TEXT("我很生气！");
	}
	else if (Emotion == TEXT("Scared"))
	{
		return TEXT("I'm scared...");
		// return TEXT("我害怕...");
	}
	else if (Emotion == TEXT("Sad"))
	{
		return TEXT("I feel sad...");
		// return TEXT("我很难过...");
	}
	else if (Emotion == TEXT("Confused"))
	{
		return TEXT("I'm confused...");
		// return TEXT("我很困惑...");
	}
	else if (Emotion == TEXT("Excited"))
	{
		return TEXT("I'm so excited!");
		// return TEXT("我好兴奋！");
	}
	else if (Emotion == TEXT("Suspicious"))
	{
		return TEXT("Something's not right...");
		// return TEXT("有点不对劲...");
	}
	else if (Emotion == TEXT("Curious"))
	{
		return TEXT("Interesting...");
		// return TEXT("有意思...");
	}
	else if (Emotion == TEXT("Tired"))
	{
		return TEXT("I'm so tired...");
		// return TEXT("我好累...");
	}
	else
	{
		// 默认不显示对话泡泡 / Default: no speech bubble
		return TEXT("");
	}
}

UTexture2D* UEmotionDisplayComponent::GetEmojiTextureForEmotion(const FString& Emotion) const
{
	// 检查 DataAsset 是否设置
	// Check if DataAsset is set
	if (!EmojiConfigAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] EmojiConfigAsset is not set! Please assign it in Blueprint."));
		return nullptr;
	}
	
	// 从 DataAsset 中获取纹理
	// Get texture from DataAsset
	UTexture2D* Texture = EmojiConfigAsset->GetTextureForEmotion(Emotion);
	
	if (Texture)
	{
		return Texture;
	}
	
	// 如果没有找到，尝试使用默认纹理
	// If not found, try to use default texture
	UTexture2D* DefaultTexture = EmojiConfigAsset->GetDefaultTexture();
	if (DefaultTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] No texture for emotion '%s', using default"), *Emotion);
		return DefaultTexture;
	}
	
	// 如果连默认都没有，返回 nullptr
	// If even default is not found, return nullptr
	UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] No texture found for emotion '%s' and no default texture"), *Emotion);
	return nullptr;
}
