#include "CognitionComponent.h"

#include "LLM/LLMCommunicator.h"

UCognitionComponent::UCognitionComponent()
{
	// 大脑不需要每帧 Tick，它只在事件驱动下工作
	PrimaryComponentTick.bCanEverTick = false;

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
		"=== OUTPUT FORMAT ===\n"
		"Respond with ONLY a valid JSON object. No markdown, no explanation, no extra text.\n"
		"\n"
		"{\n"
		"  \"Hunger\": 0.0,\n"
		"  \"Energy\": 0.0,\n"
		"  \"Perceived_Threat\": 0.0,\n"
		"  \"Resource_Anxiety\": 0.0,\n"
		"  \"Loneliness\": 0.0,\n"
		"  \"Trust\": 0.5,\n"
		"  \"Anger\": 0.0,\n"
		"  \"Social_Status\": 0.5,\n"
		"  \"Duty_Urgency\": 0.0,\n"
		"  \"Curiosity\": 0.5\n"
		"}\n"
		"\n"
		"=== EXAMPLES ===\n"
		"Situation: \"Player says: You're an idiot!\"\n"
		"Output: {\"Hunger\":0.0,\"Energy\":0.0,\"Perceived_Threat\":0.1,\"Resource_Anxiety\":0.0,\"Loneliness\":0.0,\"Trust\":0.3,\"Anger\":0.8,\"Social_Status\":0.7,\"Duty_Urgency\":0.0,\"Curiosity\":0.0}\n"
		"Reason: Insulted → High Anger, Trust decreases, Social_Status increases (wants to prove self)\n"
		"\n"
		"Situation: \"Player says: I'll kill you!\"\n"
		"Output: {\"Hunger\":0.0,\"Energy\":0.0,\"Perceived_Threat\":0.9,\"Resource_Anxiety\":0.0,\"Loneliness\":0.0,\"Trust\":0.1,\"Anger\":0.6,\"Social_Status\":0.5,\"Duty_Urgency\":0.0,\"Curiosity\":0.0}\n"
		"Reason: Verbal threat → Very high Perceived_Threat, Low Trust, Moderate Anger\n"
		"\n"
		"Situation: \"Player helps NPC complete a task\"\n"
		"Output: {\"Hunger\":0.0,\"Energy\":0.0,\"Perceived_Threat\":0.0,\"Resource_Anxiety\":0.0,\"Loneliness\":0.0,\"Trust\":0.8,\"Anger\":0.0,\"Social_Status\":0.3,\"Duty_Urgency\":0.0,\"Curiosity\":0.0}\n"
		"Reason: Player helped → High Trust, Low Social_Status need (satisfied)\n"
		"\n"
		"Now analyze the current situation and output JSON:\n"
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
		// 思考成功，广播结果给 Controller (或其他监听者)
		OnMentalStateChanged.Broadcast(NewState);
		UE_LOG(LogTemp, Log, TEXT("[Cognition] Mental State Broadcasted. Anger: %.2f"), NewState.Anger);
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