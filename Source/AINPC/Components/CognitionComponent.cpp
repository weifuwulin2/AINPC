#include "CognitionComponent.h"

#include "Controller/UtilityAIController.h"
#include "Components/PersonalityComponent.h"
#include "LLM/LLMCommunicator.h"
#include "UtilityAI/SentimentMapping.h"
#include "UtilityAI/MentalStateInterpolation.h"

UCognitionComponent::UCognitionComponent()
{
	// 大脑需要每帧 Tick 来更新插值
	// Brain needs to Tick every frame to update interpolation
	PrimaryComponentTick.bCanEverTick = true;
}

void UCognitionComponent::BeginPlay()
{
	Super::BeginPlay();

	// 0. 获取 Owner 上的 MemoryComponent
	// Get MemoryComponent from Owner
	if (AActor* Owner = GetOwner())
	{
		MemoryComp = Owner->FindComponentByClass<UMemoryComponent>();
		if (!MemoryComp)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Cognition] Warning: No MemoryComponent found on Owner! Memories will not be stored/retrieved."));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[Cognition] Connected to MemoryComponent."));
		}
	}

	// 1. 初始化 LLM 服务对象
	LLMService = NewObject<ULLMCommunicator>(this);

	// 2. 读取配置 (逻辑从 Controller 移到了这里)
	FString ConfigApiKey;
	FString ConfigApiUrl;

	GConfig->GetString(TEXT("LLM.Settings"), TEXT("ApiKey"), ConfigApiKey, GGameIni);
	GConfig->GetString(TEXT("LLM.Settings"), TEXT("ApiUrl"), ConfigApiUrl, GGameIni);

	// 安全检查
	if (ConfigApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Cognition] FATAL: API Key not found in DefaultGame.ini!"));
		return;
	}

	if (ConfigApiUrl.IsEmpty())
	{
		ConfigApiUrl = TEXT("https://api.deepseek.com/chat/completions");
	}

	// 3. 初始化 Service
	LLMService->Init(ConfigApiKey, ConfigApiUrl);
    
	UE_LOG(LogTemp, Log, TEXT("[Cognition] Brain Initialized via Config."));

	// 4. 初始化语义映射器 / Initialize Sentiment Mapper
	SentimentMapper = NewObject<USentimentMapper>(this);
	SentimentMapper->Initialize();  // 使用默认映射
	UE_LOG(LogTemp, Log, TEXT("[Cognition] SentimentMapper initialized"));

	// 5. 初始化插值器 / Initialize Interpolator
	Interpolator = NewObject<UMentalStateInterpolator>(this);
	
	// 配置不同情绪的插值速度 / Configure interpolation speeds
	FMentalStateInterpConfig FastConfig;
	FastConfig.InterpSpeed = 5.0f;  // 快速变化
	FastConfig.SnapThreshold = 0.01f;
	FastConfig.bEnableRandomPerturbation = true;
	FastConfig.PerturbationRange = 0.05f;
	
	FMentalStateInterpConfig MediumConfig;
	MediumConfig.InterpSpeed = 2.0f;  // 中速变化
	MediumConfig.SnapThreshold = 0.01f;
	MediumConfig.bEnableRandomPerturbation = true;
	MediumConfig.PerturbationRange = 0.05f;
	
	FMentalStateInterpConfig SlowConfig;
	SlowConfig.InterpSpeed = 0.5f;  // 慢速变化
	SlowConfig.SnapThreshold = 0.01f;
	SlowConfig.bEnableRandomPerturbation = true;
	SlowConfig.PerturbationRange = 0.03f;
	
	// 应用配置 / Apply configurations
	Interpolator->SetInterpConfig("Indignity", FastConfig);        // 屈辱：快速 (5.0)
	Interpolator->SetInterpConfig("Perceived_Threat", FastConfig); // 威胁：快速
	
	Interpolator->SetInterpConfig("Hunger", MediumConfig);         // 饥饿：中速 (2.0)
	Interpolator->SetInterpConfig("Fatigue", MediumConfig);        // 疲劳：中速
	Interpolator->SetInterpConfig("Loneliness", MediumConfig);     // 孤独：中速
	
	Interpolator->SetInterpConfig("Trust", SlowConfig);            // 信任：慢速 (0.5) (兼容)
	Interpolator->SetInterpConfig("Boredom", SlowConfig);          // 无聊：慢速
	
	UE_LOG(LogTemp, Log, TEXT("[Cognition] Interpolator initialized with custom speeds"));
}

void UCognitionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 获取 MentalState（从 Controller）
	// Get MentalState (from Controller)
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (AIController)
	{
		AUtilityAIController* UtilityController = Cast<AUtilityAIController>(AIController);
		if (UtilityController && UtilityController->MentalState && Interpolator)
		{
			// 每帧更新插值
			// Update interpolation every frame
			Interpolator->UpdateInterpolation(UtilityController->MentalState, DeltaTime);
		}
	}
}

void UCognitionComponent::ProcessStimulus(FString SituationDescription)
{
	// 🔍 关键日志：确认 ProcessStimulus 被调用
	UE_LOG(LogTemp, Warning, TEXT("[Cognition] Processing Stimulus: %s"), *SituationDescription);
	
	// 1. 存入新记忆 (交给 MemoryComponent 处理，它会自己决定是否触发反思)
	// Phase 4: Memory System handles storage and reflection trigger internally
	// We don't manually AddMemory here anymore using string, we expect Controller to have called CommitEvent.
	// However, ProcessStimulus is called by Controller::RelaySensoryToCognition.
	// Wait, RelaySensoryToCognition is legacy?
	// The new path is: Event -> Sensory -> Memory.
	// The new path for "Thinking" is: ... when does thinking happen?
	// "Phase 2: Social Layer - 运作流程：触发：当生理层判断事件重要且非战斗时"
	// So MemoryComponent or Controller should trigger Cognition.
	
	// Let's assume ProcessStimulus is now the entry point for "Thinking about a situation".
	// So we DO need to retrieve memories here.

	// 2. 检索检索 (Retrieval)
	FString ContextMemory = "";
	if (MemoryComp)
	{
		TArray<FMemoryItem> Memories = MemoryComp->RetrieveRelevantMemories(SituationDescription, 5);
		for (const FMemoryItem& Item : Memories)
		{
			ContextMemory += FString::Printf(TEXT("- [%s] %s (Imp: %.1f)\n"), 
				*Item.Timestamp.ToString(), *Item.Description, Item.ImportanceScore);
		}
	}
	
	// If no memory found
	if (ContextMemory.IsEmpty())
	{
		ContextMemory = "No relevant memories.";
	}

	// ✅ 从 PersonalityComponent 获取角色信息 / Get role info from PersonalityComponent
	FString ActualRoleDescription = RoleDescription; // Default fallback
	FString ActualBehavioralGuidelines = BehavioralGuidelines;
	FString PersonalityIDStr = TEXT("Unknown");
	FString FactionStr = TEXT("Neutral");
	
	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		if (AUtilityAIController* UtilityController = Cast<AUtilityAIController>(AIController))
		{
			if (UPersonalityComponent* PersonalityComp = UtilityController->PersonalityComp)
			{
				// 使用 PersonalityComponent 中的配置
				// Use configuration from PersonalityComponent
				if (!PersonalityComp->Personality.RoleDescription.IsEmpty())
				{
					ActualRoleDescription = PersonalityComp->Personality.RoleDescription;
				}
				if (!PersonalityComp->Personality.BehavioralGuidelines.IsEmpty())
				{
					ActualBehavioralGuidelines = PersonalityComp->Personality.BehavioralGuidelines;
				}
				
				// 获取 PersonalityID 和 Faction
				// Get PersonalityID and Faction
				PersonalityIDStr = PersonalityComp->PersonalityID.ToString();
				FactionStr = UEnum::GetValueAsName(PersonalityComp->Personality.Faction).ToString();
				
				UE_LOG(LogTemp, Log, TEXT("[Cognition] Using personality from PersonalityComponent: ID=%s, Faction=%s"), 
					*PersonalityIDStr, *FactionStr);
			}
		}
	}
	
	// ⚠️ 关键修复：如果 Personality 还没初始化（ID为None），不要发送请求，避免 LLM 产生幻觉
	// Critical Fix: If Personality is not initialized (ID is None), do not send request to avoid LLM hallucinations
	if (PersonalityIDStr == "None" || PersonalityIDStr == "Default" || PersonalityIDStr.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Cognition] PersonalityID not set yet. Scheduling retry in 0.5s..."));
		
		PendingStimulus = SituationDescription;
		
		// 避免重复设置 Timer
		if (!GetWorld()->GetTimerManager().IsTimerActive(RetryStimulusTimerHandle))
		{
			UE_LOG(LogTemp, Log, TEXT("[Cognition] Timer started. Will retry in 0.5s."));
			GetWorld()->GetTimerManager().SetTimer(RetryStimulusTimerHandle, [this]()
			{
				if (!PendingStimulus.IsEmpty())
				{
					UE_LOG(LogTemp, Log, TEXT("[Cognition] Retrying pending stimulus..."));
					
					// 先复制并清空，防止 ProcessStimulus 再次 Pending 后被这里误删
					// Copy and clear first to prevent accidental deletion if ProcessStimulus pends again
					FString CheckStimulus = PendingStimulus;
					PendingStimulus.Empty();
					
					ProcessStimulus(CheckStimulus);
				}
			}, 0.5f, false);
		}
		
		return;
	}
	
	// 构建角色部分 / Build role section
	// ⚠️ 关键：明确告诉 LLM 这是什么类型的 NPC
	FString RoleSection = FString::Printf(TEXT(
		"You are: %s (Faction: %s)\n"
		"Role: %s\n"), *PersonalityIDStr, *FactionStr, *ActualRoleDescription);
	
	// 如果有行为准则，添加到角色部分 / If there are behavioral guidelines, add to role section
	if (!ActualBehavioralGuidelines.IsEmpty())
	{
		RoleSection += FString::Printf(TEXT("Rules: %s\n"), *ActualBehavioralGuidelines);
	}
	
	// ⚠️ 根据 PersonalityID 添加性格驱动而非硬性规则
	// Add personality drives instead of hard rules to allow LLM emergence
	if (PersonalityIDStr.Contains(TEXT("Zombie")) || FactionStr.Contains(TEXT("Monster")))
	{
		// 僵尸：生理限制和原始驱动 / Physiological limits and primal drives
		RoleSection += TEXT("\n[INSTINCTS] Driven purely by insatiable hunger for living flesh. No fear, no pain, no higher logic.\n[LIMITATION] Brain rot preventing complex speech (can only grunt/hiss/say single broken words).\n");
	}
	else if (PersonalityIDStr.Contains(TEXT("Warrior")) || PersonalityIDStr.Contains(TEXT("Brave")))
	{
		// 战士：价值观驱动 / Value driven
		RoleSection += TEXT("\n[VALUES] Honor, Glory, Strength. You despise cowardice.\n[Tendency] You prefer to face threats head-on unless the situation is absolutely hopeless.\n");
	}
	else if (PersonalityIDStr.Contains(TEXT("Merchant")))
	{
		RoleSection += TEXT("\n[VALUES] Profit, Wealth, Self-Preservation.\n[Tendency] You avoid physical danger and prefer to negotiate or flee to protect your goods.\n");
	}

	// 构造精简的 Prompt / Build concise prompt
	FString Prompt = FString::Printf(TEXT(
		"%s\n"
		"Situation: %s\n"
		"Memories: %s\n\n"
		"Output valid JSON based on this TypeScript definition. ALL strings must be double-quoted.\n"
		"type Tag = \"None\" | \"Slight\" | \"Moderate\" | \"Strong\" | \"Extreme\";\n"
		"interface Response {\n"
		"  Hunger: \"None\";\n"
		"  Fatigue: \"None\";\n"
		"  Perceived_Threat: Tag;\n"
		"  Loneliness: Tag;\n"
		"  Indignity: Tag;\n"
		"  Boredom: Tag;\n"
		"  Intention: \"Attack\" | \"Flee\" | \"Idle\" | \"Talk\";\n"
		"  Emotion: \"Scared\" | \"Anxious\" | \"Sad\" | \"Suspicious\" | \"Happy\" | \"Angry\" | \"Proud\" | \"Curious\" | \"Determined\" | \"Confused\" | \"Excited\" | \"Neutral\";\n"
		"  Speech: string; // approx 5 words, match personality\n"
		"}\n"
	), *RoleSection, *SituationDescription, *ContextMemory);
	
	UE_LOG(LogTemp, Log, TEXT("[Cognition] Sending to LLM..."));
	
	// 发送请求，并绑定内部回调 OnLLMReply
	LLMService->SendRequest(
		Prompt,
		FOnLLMResponse::CreateUObject(this, &UCognitionComponent::OnLLMReply)
	);
}

void UCognitionComponent::OnLLMReply(bool bSuccess, const FMentalState& NewState)
{
	if (bSuccess)
	{
		// 不要直接广播，而是使用 Interpolator 设置目标值
		// Don't broadcast directly, use Interpolator to set target values
		
		if (Interpolator)
		{
			// 使用宏自动为所有字段设置目标值
			// Use macro to automatically set target values for all fields
			#define SET_TARGET_VALUE(Name, DefaultValue, DisplayName, Description) \
				Interpolator->SetTargetValue(TEXT(#Name), NewState.Name);
			
			MENTAL_STATE_FIELDS(SET_TARGET_VALUE)
			
			#undef SET_TARGET_VALUE
			
			UE_LOG(LogTemp, Log, TEXT("[Cognition] Target values set from LLM"));
			UE_LOG(LogTemp, Log, TEXT("  Indignity target: %.2f, Boredom target: %.2f"), 
			       NewState.Indignity, NewState.Boredom);
			
			// 注意：实际的 MentalState 更新在 TickComponent 中通过插值完成
			// Note: Actual MentalState update happens in TickComponent via interpolation
			
			// 仍然广播事件，但现在是"目标值已设置"的通知
			// Still broadcast event, but now it's a "target values set" notification
			OnMentalStateChanged.Broadcast(NewState);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Cognition] Interpolator not initialized! Broadcasting directly."));
			OnMentalStateChanged.Broadcast(NewState);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Cognition] Failed to process thought."));
	}
}

// CognitionComponent.cpp

void UCognitionComponent::StartDreaming()
{
	if (!MemoryComp || !LLMService) return;

	// 1. 获取流水账
	FString DailyLogs = MemoryComp->GetAllRecentMemoriesAsString();
	if (DailyLogs.IsEmpty()) return;

	// 2. 构造 Prompt (强制让 LLM 输出 JSON 数组，方便代码解析)
	FString Prompt = FString::Printf(TEXT(
		"Here are my recent memories:\n%s\n"
		"Task: Summarize these events into 3 concise, high-level insights about the world or the player.\n"
		"Output Format: A pure JSON array of strings. Example: [\"Player is hostile\", \"Food is scarce\"]\n"
		"Do NOT output markdown."
	), *DailyLogs);

	UE_LOG(LogTemp, Log, TEXT("[Dreaming] Sending logs to LLM..."));

	// 3. 发送请求 (注意：这里我们复用 LLMService，但需要 LLMCommunicator 支持返回原始 String 的回调)
	// 假设你的 LLMCommunicator 有一个 SendRequestRaw 或者你重载了回调，这里展示逻辑核心
	LLMService->SendRequestRaw(Prompt, FOnLLMResponseRaw::CreateUObject(this, &UCognitionComponent::OnDreamingAnalysisComplete));
}

void UCognitionComponent::OnDreamingAnalysisComplete(bool bSuccess, const FString& RawResponse)
{
	if (!bSuccess) return;

	// 4. 解析 JSON 数组
	// RawResponse 应该是 ["Insight 1", "Insight 2"]
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawResponse);

	TArray<FString> ExtractedInsights;
    
	if (FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		for (auto Value : JsonArray)
		{
			ExtractedInsights.Add(Value->AsString());
		}
	}

	// 5. 存入长期记忆
	if (MemoryComp && ExtractedInsights.Num() > 0)
	{
		MemoryComp->ConsolidateMemories(ExtractedInsights);
	}
}