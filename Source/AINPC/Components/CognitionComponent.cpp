#include "CognitionComponent.h"

#include "Controller/UtilityAIController.h"
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
	Interpolator->SetInterpConfig("Anger", FastConfig);           // 愤怒：快速 (5.0)
	Interpolator->SetInterpConfig("Perceived_Threat", FastConfig); // 威胁：快速
	
	Interpolator->SetInterpConfig("Hunger", MediumConfig);         // 饥饿：中速 (2.0)
	Interpolator->SetInterpConfig("Energy", MediumConfig);         // 精力：中速
	Interpolator->SetInterpConfig("Loneliness", MediumConfig);     // 孤独：中速
	
	Interpolator->SetInterpConfig("Trust", SlowConfig);            // 信任：慢速 (0.5)
	Interpolator->SetInterpConfig("Social_Status", SlowConfig);    // 社会地位：慢速
	Interpolator->SetInterpConfig("Curiosity", SlowConfig);        // 好奇心：慢速
	
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

	// 构建角色部分 / Build role section
	FString RoleSection = FString::Printf(TEXT(
		"=== YOUR ROLE ===\n"
		"%s\n"), *RoleDescription);
	
	// 如果有行为准则，添加到角色部分 / If there are behavioral guidelines, add to role section
	if (!BehavioralGuidelines.IsEmpty())
	{
		RoleSection += FString::Printf(TEXT(
			"\n"
			"=== BEHAVIORAL GUIDELINES ===\n"
			"%s\n"), *BehavioralGuidelines);
	}

	// 构造 JSON 格式的 Prompt
	// ... (rest of prompt construction)
	
	FString Prompt = FString::Printf(TEXT(
		"You are an NPC's cognitive system. Analyze the situation and output mental state.\n"
		"\n"
		"%s"
		"\n"
		"=== INPUT ===\n"
		"Recent Memories:\n"
		"%s\n"
		"\n"
		"Current Situation:\n"
		"%s\n"
		"\n"
		"=== MENTAL STATE VARIABLES ===\n"
		"[LLM-CONTROLLED] - You analyze these based on social/psychological factors:\n"
		"- Trust: Honesty, betrayal, help (0.0=no trust, 1.0=complete trust)\n"
		"- Anger: Insults, provocations, apologies (0.0=calm, 1.0=furious)\n"
		"- Social_Status: Respect, being looked down upon (0.0=don't care, 1.0=obsessed)\n"
		"- Curiosity: Mysteries, secrets, strange things (0.0=not curious, 1.0=very curious)\n"
		"\n"
		"[HYBRID] - You can increase based on verbal/social cues:\n"
		"- Perceived_Threat: Verbal threats, hostile tone, dangerous implications\n"
		"- Resource_Anxiety: Rumors of scarcity, war, economic crisis\n"
		"- Loneliness: Being ignored, excluded, cold-shouldered\n"
		"- Duty_Urgency: Urgent orders, emergency, 'immediately' mentioned\n"
		"\n"
		"[ENGINE-MANAGED] - Always output 'None' (engine will override):\n"
		"- Hunger, Energy\n"
		"\n"
		"=== OUTPUT TAGS ===\n"
		"Use INTENSITY TAGS, not numbers:\n"
		"[None, Slight, Moderate, Strong, Extreme]\n"
		"\n"
		"Or use NATURAL TAGS:\n"
		"- Anger: Annoyed, Angry, Furious\n"
		"- Trust: Suspicious, Trusting, Devoted\n"
		"- Threat: Safe, Cautious, Threatened, Terrified\n"
		"\n"
		"=== INTENTION (for action coordination) ===\n"
		"Choose ONE primary action intention based on the situation:\n"
		"Available intentions:\n"
		"- Attack: Engage hostile target in combat\n"
		"- Flee: Escape from danger or threatening situation\n"
		"- Idle: No specific action needed, passive state\n"
		"- Talk: Initiate or respond to social interaction\n"
		"\n"
		"=== EMOTION (for emoji display) ===\n"
		"Choose ONE emotion based on the DOMINANT need (Maslow's Hierarchy):\n"
		"\n"
		"Priority (highest to lowest):\n"
		"1. SAFETY: Scared (high threat), Anxious (resource anxiety)\n"
		"2. SOCIAL: Sad (lonely), Suspicious (low trust), Happy (high trust + safe)\n"
		"3. ESTEEM: Angry (high anger), Proud (high status)\n"
		"4. SELF-ACTUALIZATION: Curious (high curiosity), Determined (duty urgent)\n"
		"5. OTHER: Confused (unclear), Excited (opportunity), Neutral (default)\n"
		"\n"
		"Available emotions:\n"
		"Scared, Anxious, Sad, Suspicious, Happy, Angry, Proud, Curious, Determined, Confused, Excited, Neutral\n"
		"\n"
		"=== SPEECH (for dialogue bubble) ===\n"
		"Generate a SHORT dialogue (5-15 words):\n"
		"- Match the emotion and INTENTION\n"
		"- Stay in character\n"
		"- Be ORIGINAL (don't copy examples)\n"
		"- Natural, conversational language\n"
		"\n"
		"=== OUTPUT FORMAT ===\n"
		"Respond with ONLY valid JSON. No markdown, no explanation.\n"
		"\n"
		"Required fields:\n"
		"{\n"
		"  \"Hunger\": \"None\",\n"
		"  \"Energy\": \"None\",\n"
		"  \"Perceived_Threat\": \"<tag>\",\n"
		"  \"Resource_Anxiety\": \"<tag>\",\n"
		"  \"Loneliness\": \"<tag>\",\n"
		"  \"Trust\": \"<tag>\",\n"
		"  \"Anger\": \"<tag>\",\n"
		"  \"Social_Status\": \"<tag>\",\n"
		"  \"Duty_Urgency\": \"<tag>\",\n"
		"  \"Curiosity\": \"<tag>\",\n"
		"  \"Intention\": \"<Attack|Flee|Idle|Talk>\",\n"
		"  \"Emotion\": \"<emotion>\",\n"
		"  \"Speech\": \"<original dialogue>\"\n"
		"}\n"
		"\n"
		"Now analyze:\n"
	), *RoleSection, *ContextMemory, *SituationDescription);
	
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
			UE_LOG(LogTemp, Log, TEXT("  Anger target: %.2f, Trust target: %.2f"), 
			       NewState.Anger, NewState.Trust);
			
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