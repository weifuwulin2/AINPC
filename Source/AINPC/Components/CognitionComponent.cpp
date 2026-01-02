#include "CognitionComponent.h"

#include "LLM/LLMCommunicator.h"

UCognitionComponent::UCognitionComponent()
{
	// 大脑不需要每帧 Tick，它只在事件驱动下工作
	PrimaryComponentTick.bCanEverTick = false; 
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

	MemoryComp = CreateDefaultSubobject<UMemoryComponent>(TEXT("Memory"));
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
	"Memories:\n%s\n"
	"Event: %s\n"
	"Task: 1. Analyze mental state (Anger, Fear).\n"
	"      2. Rate the 'Importance' of this event (0.0 to 1.0) based on how much it affects your survival or emotions.\n"
	"Output JSON: {\"Anger\":..., \"Fear\":..., \"Importance\":...}"
	),*ContextMemory,*SituationDescription);
	
	// 发送请求，并绑定内部回调 OnLLMReply
	LLMService->SendRequest(
		SituationDescription,
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