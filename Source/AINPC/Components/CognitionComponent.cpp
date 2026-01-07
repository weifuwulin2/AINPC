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

	MemoryComp = CreateDefaultSubobject<UMemoryComponent>(TEXT("Memory"));
}

void UCognitionComponent::BeginPlay()
{
	Super::BeginPlay();

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
	
	// 1. 存入新记忆 (把刚才看到的事存下来)
	if (MemoryComp)
	{
		MemoryComp->AddMemory(SituationDescription);
	}

	// 2. 检索旧记忆 (寻找相关性)
	FString ContextMemory = "";
	if (MemoryComp)
	{
		ContextMemory = MemoryComp->RetrieveRelevantMemories(SituationDescription);
	}

	// 构造 JSON 格式的 Prompt，提高 LLM 准确率
	// Construct JSON-formatted Prompt for better LLM accuracy
	FString Prompt = FString::Printf(TEXT(
		"You are an NPC's cognitive system in a game world. Your role is to analyze situations and output your mental state.\n"
		"\n"
		"=== INPUT ===\n"
		"Recent Memories:\n"
		"%s\n"
		"\n"
		"Current Situation:\n"
		"%s\n"
		"\n"
		"=== JURISDICTION RULES ===\n"
		"Variables are managed by different systems:\n"
		"\n"
		"[ENGINE-EXCLUSIVE] (READ-ONLY - DO NOT MODIFY):\n"
		"- Hunger: Managed by game engine (time-based metabolism)\n"
		"- Energy: Managed by game engine (activity-based)\n"
		"→ You MUST output 0.0 for these (they will be overridden by engine)\n"
		"\n"
		"[LLM-EXCLUSIVE] (YOU CONTROL THESE):\n"
		"- Trust: Based on honesty, betrayal, help (0.0=no trust, 1.0=complete trust)\n"
		"- Anger: Based on insults, provocations, apologies (0.0=calm, 1.0=furious)\n"
		"- Social_Status: Based on respect, being looked down upon (0.0=don't care, 1.0=obsessed)\n"
		"- Curiosity: Based on mysteries, secrets, strange things (0.0=not curious, 1.0=extremely curious)\n"
		"\n"
		"[HYBRID] (YOU CAN INCREASE based on psychological factors):\n"
		"- Perceived_Threat: Engine detects physical threats, you detect verbal/social threats\n"
		"- Resource_Anxiety: Engine detects low resources, you detect rumors/worries\n"
		"- Loneliness: Engine tracks time, you detect being ignored/excluded\n"
		"- Duty_Urgency: Engine tracks schedule, you detect urgent commands/orders\n"
		"\n"
		"=== ANALYSIS GUIDELINES ===\n"
		"Trust:\n"
		"- Increase if: Player helps, is honest, keeps promises\n"
		"- Decrease if: Player lies, betrays, breaks promises\n"
		"\n"
		"Anger:\n"
		"- Increase if: Insulted, provoked, threatened verbally\n"
		"- Decrease if: Apologized to, calmed down, situation resolved\n"
		"\n"
		"Social_Status:\n"
		"- Increase if: Being looked down upon, disrespected, challenged\n"
		"- Decrease if: Being respected, admired, status confirmed\n"
		"\n"
		"Curiosity:\n"
		"- Increase if: Hear secrets, see strange things, mysteries mentioned\n"
		"- Decrease if: Questions answered, mysteries solved\n"
		"\n"
		"Perceived_Threat (Hybrid):\n"
		"- Increase if: Verbal threats (\"I'll kill you\"), hostile tone, dangerous implications\n"
		"\n"
		"Resource_Anxiety (Hybrid):\n"
		"- Increase if: Hear rumors of scarcity, war coming, economic crisis\n"
		"\n"
		"Loneliness (Hybrid):\n"
		"- Increase if: Being ignored in conversation, excluded from group, cold-shouldered\n"
		"\n"
		"Duty_Urgency (Hybrid):\n"
		"- Increase if: Urgent orders given, emergency declared, \"immediately\" mentioned\n"
		"\n"
		"=== SENTIMENT INTENSITY TAGS ===\n"
		"IMPORTANT: Do NOT output numerical values (0.0-1.0). Use TAGS instead.\n"
		"\n"
		"Choose from these STANDARD INTENSITY TAGS:\n"
		"[None, Slight, Moderate, Strong, Extreme]\n"
		"\n"
		"Mapping:\n"
		"- None/Neutral: 无感 (no feeling)\n"
		"- Slight/Low: 轻微 (slight)\n"
		"- Moderate/Medium: 中等 (moderate)\n"
		"- Strong/High: 强烈 (strong)\n"
		"- Extreme/Max: 极端 (extreme)\n"
		"\n"
		"You can also use NATURAL LANGUAGE TAGS for specific emotions:\n"
		"- Anger: Annoyed, Angry, Furious, Enraged\n"
		"- Trust: Suspicious, Trusting, Devoted\n"
		"- Hunger: Satisfied, Peckish, Hungry, Starving\n"
		"- Threat: Safe, Cautious, Threatened, Terrified\n"
		"\n"
		"=== OUTPUT FORMAT ===\n"
		"Respond with ONLY a valid JSON object. No markdown, no explanation, no extra text.\n"
		"Use TAGS, NOT numbers!\n"
		"\n"
		"{\n"
		"  \"Hunger\": \"None\",\n"
		"  \"Energy\": \"None\",\n"
		"  \"Perceived_Threat\": \"None\",\n"
		"  \"Resource_Anxiety\": \"None\",\n"
		"  \"Loneliness\": \"None\",\n"
		"  \"Trust\": \"Moderate\",\n"
		"  \"Anger\": \"None\",\n"
		"  \"Social_Status\": \"Moderate\",\n"
		"  \"Duty_Urgency\": \"None\",\n"
		"  \"Curiosity\": \"Moderate\"\n"
		"}\n"
		"\n"
		"=== EXAMPLES ===\n"
		"Situation: \"Player says: You're an idiot!\"\n"
		"Output: {\"Hunger\":\"None\",\"Energy\":\"None\",\"Perceived_Threat\":\"Slight\",\"Resource_Anxiety\":\"None\",\"Loneliness\":\"None\",\"Trust\":\"Slight\",\"Anger\":\"Furious\",\"Social_Status\":\"Strong\",\"Duty_Urgency\":\"None\",\"Curiosity\":\"None\"}\n"
		"Reason: Insulted → Furious (high anger), Trust decreases to Slight, Social_Status Strong (wants to prove self)\n"
		"\n"
		"Situation: \"Player says: I'll kill you!\"\n"
		"Output: {\"Hunger\":\"None\",\"Energy\":\"None\",\"Perceived_Threat\":\"Terrified\",\"Resource_Anxiety\":\"None\",\"Loneliness\":\"None\",\"Trust\":\"Suspicious\",\"Anger\":\"Angry\",\"Social_Status\":\"Moderate\",\"Duty_Urgency\":\"None\",\"Curiosity\":\"None\"}\n"
		"Reason: Verbal threat → Terrified (very high threat), Suspicious (low trust), Angry (moderate anger)\n"
		"\n"
		"Situation: \"Player helps NPC complete a task\"\n"
		"Output: {\"Hunger\":\"None\",\"Energy\":\"None\",\"Perceived_Threat\":\"Safe\",\"Resource_Anxiety\":\"None\",\"Loneliness\":\"None\",\"Trust\":\"Trusting\",\"Anger\":\"None\",\"Social_Status\":\"Slight\",\"Duty_Urgency\":\"None\",\"Curiosity\":\"None\"}\n"
		"Reason: Player helped → Trusting (high trust), Safe (no threat), Slight Social_Status (satisfied)\n"
		"\n"
		"Now analyze the current situation and output JSON with TAGS:\n"
	), *ContextMemory, *SituationDescription);
	
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