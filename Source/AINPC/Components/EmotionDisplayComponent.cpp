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
#include "AINPC.h"
#include "GoalComponent.h"
#include "NPCDefinitionComponent.h"
#include "UtilityAIComponent.h"
#include "Misc/OutputDeviceNull.h"

UEmotionDisplayComponent::UEmotionDisplayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEmotionDisplayComponent::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
	AINPC_LOG(Warning, "BeginPlay called");

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("[EmotionDisplay] Component Active!"));
	}
	
	// 延迟创建 Widget 组件，确保 Pawn 已经被 Possess
	// Delay Widget creation to ensure Pawn is possessed
	if (GetWorld())
	{
		FTimerHandle DelayHandle;
		GetWorld()->GetTimerManager().SetTimer(
			DelayHandle,
			[this]()
			{
				AINPC_LOG(Warning, "Delayed initialization starting...");
				CreateWidgetComponents();
				BindToCognitionEvents();

				// Update Nameplate
				// Robust Controller Retrieval
				AAIController* AIC = Cast<AAIController>(GetOwner());
				APawn* PawnOwner = Cast<APawn>(GetOwner());
				
				if (!AIC && PawnOwner)
				{
					AIC = Cast<AAIController>(PawnOwner->GetController());
				}

				if (AIC)
				{
					// Try to find Definition Component on Controller OR Pawn
					UNPCDefinitionComponent* DefComp = AIC->FindComponentByClass<UNPCDefinitionComponent>();
					if (!DefComp && AIC->GetPawn())
					{
						DefComp = AIC->GetPawn()->FindComponentByClass<UNPCDefinitionComponent>();
					}

					if (DefComp)
					{
						// Try to get modular name
						FString FinalName = AIC->GetPawn() ? AIC->GetPawn()->GetName() : AIC->GetName();
						FNPCNameDef NameDef;
						if (DefComp->GetNameDef(NameDef))
						{
							FinalName = NameDef.FirstName;
							if (!NameDef.Surname.IsEmpty())
							{
								FinalName += " " + NameDef.Surname;
							}
						}
						
						UpdateNameplate(FinalName, DefComp->PersonalityID.ToString(), DefComp->ProfessionID.ToString());
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] ❌ Could not find UNPCDefinitionComponent on Controller or Pawn!"));
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] ❌ Could not resolve AIController in BeginPlay!"));
				}
			},
			0.5f,  // 延迟 0.5 秒
			false
		);
		
		UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] Scheduled delayed initialization"));
	}
	else
	{
		AINPC_LOG_ERROR("GetWorld() returned nullptr!");
	}
	
	UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
}


void UEmotionDisplayComponent::CreateWidgetComponents()
{
	AINPC_LOG(Warning, "CreateWidgetComponents called");
	
	// Owner 是 AI Controller，需要获取它控制的 Pawn
	// Owner is AI Controller, need to get the controlled Pawn
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (!AIController)
	{
		AINPC_LOG_ERROR("❌ Owner is not an AIController! Owner: %s", 
		       GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] ✅ AIController found: %s"), *AIController->GetName());
	
	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		AINPC_LOG_ERROR("❌ No Pawn controlled yet by %s", *AIController->GetName());
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] ✅ Controlled Pawn found: %s"), *ControlledPawn->GetName());
	
	// 获取 Pawn 的根组件
	// Get Pawn's root component
	USceneComponent* RootComp = ControlledPawn->GetRootComponent();
	if (!RootComp)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] ❌ Pawn has no root component!"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] ✅ Root component found: %s"), *RootComp->GetName());
	
	// 创建 Emoji Widget 组件（附加到 Pawn，不是 Controller）
	// Create Emoji Widget Component (attach to Pawn, not Controller)
	EmojiWidgetComponent = NewObject<UWidgetComponent>(ControlledPawn, UWidgetComponent::StaticClass(), TEXT("EmojiWidget"));
	if (EmojiWidgetComponent)
	{
		EmojiWidgetComponent->RegisterComponent();
		EmojiWidgetComponent->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
		EmojiWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HeightOffset));
		EmojiWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		EmojiWidgetComponent->SetDrawSize(FVector2D(32.0f, 32.0f));  // 改为 32x32
		EmojiWidgetComponent->SetVisibility(false); // 初始隐藏 / Initially hidden
		EmojiWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // ✅ Disable Collision
		
		if (EmojiWidgetClass)
		{
			EmojiWidgetComponent->SetWidgetClass(EmojiWidgetClass);
			UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] ✅ Emoji Widget Component created with class: %s"), 
			       *EmojiWidgetClass->GetName());
		}
		else
		{
			AINPC_LOG_ERROR("❌ EmojiWidgetClass is not set!");
		}
		
		UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] ✅ Emoji Widget attached to Pawn: %s"), *ControlledPawn->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] ❌ Failed to create EmojiWidgetComponent!"));
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
		SpeechBubbleWidgetComponent->SetDrawSize(FVector2D(200.0f, 60.0f));  // 改为 200x60
		SpeechBubbleWidgetComponent->SetVisibility(false); // 初始隐藏 / Initially hidden
		SpeechBubbleWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // ✅ Disable Collision
		
		if (SpeechBubbleWidgetClass)
		{
			SpeechBubbleWidgetComponent->SetWidgetClass(SpeechBubbleWidgetClass);
			UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] ✅ Speech Bubble Widget Component created with class: %s"), 
			       *SpeechBubbleWidgetClass->GetName());
		}
		else
		{
			AINPC_LOG_ERROR("❌ SpeechBubbleWidgetClass is not set!");
		}
		
		UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] ✅ Speech Bubble Widget attached to Pawn: %s"), *ControlledPawn->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] ❌ Failed to create SpeechBubbleWidgetComponent!"));
	}

	// 3. Nameplate Widget
	if (NameplateWidgetClass)
	{
		NameplateWidgetComponent = NewObject<UWidgetComponent>(ControlledPawn, TEXT("NameplateWidget"));
		if (NameplateWidgetComponent)
		{
			NameplateWidgetComponent->RegisterComponent();
			NameplateWidgetComponent->AttachToComponent(ControlledPawn->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			NameplateWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
			NameplateWidgetComponent->SetDrawAtDesiredSize(true);
			
			// 位置调整：在头顶下方一点，或更高 (Position adjustment)
			NameplateWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HeightOffset + 25.0f)); // Slightly higher than others
			NameplateWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // ✅ Disable Collision
			
			NameplateWidgetComponent->SetWidgetClass(NameplateWidgetClass);
			NameplateWidgetComponent->SetVisibility(true); // Always visible by default
			
			AINPC_LOG(Log, "✅ Nameplate Widget attached to Pawn: %s", *ControlledPawn->GetName());
		}
	}
	else
	{
		// Optional, so no error log if missing
		AINPC_LOG(Log, "NameplateWidgetClass not set, skipping nameplate.");
	}
}

void UEmotionDisplayComponent::UpdateNameplate(const FString& Name, const FString& Personality, const FString& Profession)
{
	if (!NameplateWidgetComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] NameplateWidgetComponent is NULL!"));
		return;
	}

	UUserWidget* Widget = NameplateWidgetComponent->GetWidget();
	if (!Widget)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] Widget Instance is NULL! (Maybe not initialized yet?)"));
		return;
	}

	// 尝试设置文本 (Try to set text)
	// Format: [Personality] Name <Profession>
	
	FString DisplayText = FString::Printf(TEXT("[%s] %s"), *Personality, *Name);
	if (!Profession.IsEmpty() && Profession != TEXT("None"))
	{
		DisplayText += FString::Printf(TEXT(" <%s>"), *Profession);
	}

	UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] Updating Nameplate: %s"), *DisplayText);

	// 使用反射查找名为 "NameText" 的 TextBlock (Use reflection to find TextBlock named "NameText")
	if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("NameText"))))
	{
		TextBlock->SetText(FText::FromString(DisplayText));
		UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] ✅ Metadata found 'NameText' and set text."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] ⚠️ Could not find 'NameText' via C++. Trying Blueprint Function 'SetNameText'..."));
		
		// 如果找不到 TextBlock，尝试调用 Blueprint 函数 "SetNameText"
		// Fallback: Call Blueprint function "SetNameText"
		FOutputDeviceNull Ar;
		FString Cmd = FString::Printf(TEXT("SetNameText \"%s\""), *DisplayText);
		Widget->CallFunctionByNameWithArguments(*Cmd, Ar, nullptr, true);
	}
}

void UEmotionDisplayComponent::BindToCognitionEvents()
{
	UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] BindToCognitionEvents called"));
	
	// 获取 AI Controller
	// Get AI Controller
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] ❌ Owner is not an AIController!"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] ✅ AIController found: %s"), *AIController->GetName());
	
	AUtilityAIController* UtilityController = Cast<AUtilityAIController>(AIController);
	if (!UtilityController)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] ❌ AIController is not a UtilityAIController!"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] ✅ UtilityAIController found: %s"), *UtilityController->GetName());
	
	if (!UtilityController->CognitionComp)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] ❌ CognitionComp is null!"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] ✅ CognitionComp found"));
	
	// 绑定到 OnMentalStateChanged 事件
	// Bind to OnMentalStateChanged event
	UtilityController->CognitionComp->OnMentalStateChanged.AddDynamic(this, &UEmotionDisplayComponent::OnEmotionChanged);
	
	AINPC_LOG(Warning, "✅ Successfully bound to CognitionComponent events");
}

void UEmotionDisplayComponent::OnEmotionChanged(const FMentalState& NewState)
{
	// 防抖动：只在情绪真正改变时才显示
	// Debounce: Only show when emotion actually changes
	static FString LastEmotion = TEXT("");
	static FString LastSpeech = TEXT("");
	
	// 当情绪变化时，显示对应的 emoji
	// When emotion changes, display corresponding emoji
	if (!NewState.Emotion.IsEmpty() && NewState.Emotion != TEXT("Neutral"))
	{
		// 只在情绪改变时才更新显示
		if (NewState.Emotion != LastEmotion)
		{
			LastEmotion = NewState.Emotion;
			ShowEmotion(NewState.Emotion);
		}
	}
	
	// 显示 LLM 生成的对话内容
	// Display LLM-generated speech content
	if (!NewState.Speech.IsEmpty())
	{
		// 只在对话改变时才更新显示
		if (NewState.Speech != LastSpeech)
		{
			// ✅ Simplified Speech Gate: 默认允许说话，只在睡觉时禁言
			// Default allow speech, only block during Sleep
			bool bCanSpeak = true;
			
			if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(GetOwner()))
			{
				if (UAICon->UtilityComp && UAICon->UtilityComp->CurrentAction)
				{
					FString ActionName = UAICon->UtilityComp->CurrentAction->ActionName;
					if (ActionName.Contains(TEXT("Sleep")))
					{
						bCanSpeak = false;
					}
				}
			}
			
			if (bCanSpeak)
			{
				LastSpeech = NewState.Speech;
				ShowSpeechBubble(NewState.Speech);
			}
			else
			{
				// NPC is sleeping, discard speech
				UE_LOG(LogTemp, Verbose, TEXT("[EmotionDisplay] Speech suppressed (sleeping): %s"), *NewState.Speech);
			}
		}
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
	
	AINPC_LOG(Log, "Showing emotion: %s", *Emotion);
	
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
	
	AINPC_LOG(Log, "Showing speech bubble: %s", *Message);
	
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
	// 先检查缓存
	// Check cache first
	if (TextureCache.Contains(Emotion))
	{
		UTexture2D* CachedTexture = TextureCache[Emotion];
		if (CachedTexture)
		{
			return CachedTexture;
		}
	}
	
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
		// 缓存纹理
		// Cache the texture
		const_cast<UEmotionDisplayComponent*>(this)->TextureCache.Add(Emotion, Texture);
		UE_LOG(LogTemp, Log, TEXT("[EmotionDisplay] Cached texture for emotion: %s"), *Emotion);
		return Texture;
	}
	
	// 如果没有找到，尝试使用默认纹理
	// If not found, try to use default texture
	UTexture2D* DefaultTexture = EmojiConfigAsset->GetDefaultTexture();
	if (DefaultTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EmotionDisplay] No texture for emotion '%s', using default"), *Emotion);
		// 也缓存默认纹理
		// Also cache default texture
		const_cast<UEmotionDisplayComponent*>(this)->TextureCache.Add(Emotion, DefaultTexture);
		return DefaultTexture;
	}
	
	// 如果连默认都没有，返回 nullptr
	// If even default is not found, return nullptr
	UE_LOG(LogTemp, Error, TEXT("[EmotionDisplay] No texture found for emotion '%s' and no default texture"), *Emotion);
	return nullptr;
}
