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

	// 🔍 调试日志：查看实际读取的配置
	UE_LOG(LogTemp, Warning, TEXT("=== LLM Configuration Debug ==="));
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] Config File: %s"), *GGameIni);
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] ApiKey: %s"), ConfigApiKey.IsEmpty() ? TEXT("<EMPTY>") : TEXT("<SET>"));
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] ApiUrl: %s"), ConfigApiUrl.IsEmpty() ? TEXT("<EMPTY>") : *ConfigApiUrl);
	UE_LOG(LogTemp, Warning, TEXT("=============================="));

	// 安全检查
	if (ConfigApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Cognition] FATAL: API Key not found in DefaultGame.ini!"));
		UE_LOG(LogTemp, Error, TEXT("[Cognition] Please check Config/DefaultGame.ini has [LLM.Settings] section"));
		return;
	}

	if (ConfigApiUrl.IsEmpty())
	{
		ConfigApiUrl = TEXT("https://api.deepseek.com/chat/completions");
		UE_LOG(LogTemp, Warning, TEXT("[Cognition] ApiUrl not set, using default: %s"), *ConfigApiUrl);
	}

	// 3. 初始化 Service
	LLMService->Init(ConfigApiKey, ConfigApiUrl);
    
	UE_LOG(LogTemp, Log, TEXT("[Cognition] Brain Initialized via Config."));
	UE_LOG(LogTemp, Log, TEXT("[Cognition] Using API URL: %s"), *ConfigApiUrl);

	
}

void UCognitionComponent::ProcessStimulus(FString SituationDescription)
{
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

	FString Prompt = FString::Printf(TEXT(
	"You are an NPC in a combat game. Analyze the following event and determine your emotional response.\n"
	"\n"
	"Context:\n"
	"- You are a combat NPC who can feel emotions\n"
	"- Seeing a Player should make you alert or aggressive\n"
	"- Being attacked should increase Anger and Fear\n"
	"- Neutral events should have low emotional values\n"
	"\n"
	"Recent Memories:\n%s\n"
	"\n"
	"Current Event: %s\n"
	"\n"
	"Task: Analyze your emotional state based on this event.\n"
	"- Anger: 0.0 (calm) to 1.0 (furious). Increase when threatened or attacked.\n"
	"- Fear: 0.0 (brave) to 1.0 (terrified). Increase when facing danger.\n"
	"- Confidence: 0.0 (scared) to 1.0 (confident). Decrease when outmatched.\n"
	"\n"
	"Examples:\n"
	"- Event: 'I saw Player' → {\"Anger\":0.6, \"Fear\":0.3, \"Confidence\":0.5}\n"
	"- Event: 'I was attacked by Player taking 50.0 damage' → {\"Anger\":0.9, \"Fear\":0.7, \"Confidence\":0.3}\n"
	"- Event: 'I saw an enemy NPC' → {\"Anger\":0.4, \"Fear\":0.2, \"Confidence\":0.6}\n"
	"\n"
	"Output ONLY a JSON object with these exact fields (no markdown, no explanation):\n"
	"{\"Anger\":..., \"Fear\":..., \"Confidence\":...}"
	),*ContextMemory,*SituationDescription);
	
	// 🔍 调试：查看发送给 LLM 的 Prompt
	UE_LOG(LogTemp, Warning, TEXT("=== LLM Prompt ==="));
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Prompt);
	UE_LOG(LogTemp, Warning, TEXT("=================="));
	
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