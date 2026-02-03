// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "UObject/NoExportTypes.h"
#include "UtilityAI/MentalStateFields.h"
#include "LLMCommunicator.generated.h"

class USentimentMapper;

// 1. The Data Structure (自动生成)
USTRUCT(BlueprintType)
struct FMentalState
{
	GENERATED_BODY()

	// ✅ 使用宏自动生成所有字段
	#define DECLARE_MENTAL_STATE_FIELD(Name, DefaultValue, DisplayName, Description) \
		UPROPERTY() float Name = DefaultValue;

	MENTAL_STATE_FIELDS(DECLARE_MENTAL_STATE_FIELD)

	#undef DECLARE_MENTAL_STATE_FIELD

	// 行为意图 / Behavioral Intention
	UPROPERTY(BlueprintReadWrite)
	FString Intention = TEXT("Idle");

	// 情绪标签 / Emotion Label
	UPROPERTY(BlueprintReadWrite)
	FString Emotion = TEXT("Neutral");

	// 对话内容 / Speech Content
	UPROPERTY(BlueprintReadWrite)
	FString Speech = TEXT("");
};

// 2. The Callback Delegates
DECLARE_DELEGATE_TwoParams(FOnLLMResponse, bool /*bSuccess*/, const FMentalState& /*State*/);
DECLARE_DELEGATE_TwoParams(FOnLLMResponseRaw, bool, const FString&);

/**
 * LLM 通信器 - 双模式 API
 *
 * Roleplay Mode: 用于 CognitionComponent 的角色扮演请求 (MentalState JSON schema, 高温度)
 * Functional Mode: 用于 Dreaming、Target Selection、Social Reflection 等分析任务 (自定义系统提示, 低温度)
 */
UCLASS()
class AINPC_API ULLMCommunicator : public UObject
{
	GENERATED_BODY()

public:
	void Init(const FString& InApiKey, const FString& InUrl = TEXT("https://api.deepseek.com/chat/completions"));

	// ============================================================
	// NEW API: Dual-Mode Requests
	// ============================================================

	/**
	 * Roleplay Request: 用于 CognitionComponent ProcessStimulus
	 * - System Prompt: Instructions + MentalState JSON Schema (静态, 可缓存)
	 * - Temperature: 0.7 (创造性角色扮演)
	 * - JSON Mode: ON (强制结构化输出)
	 * - 解析响应为 FMentalState
	 */
	void SendRoleplayRequest(const FString& UserPrompt, FOnLLMResponse OnComplete);

	/**
	 * Functional Request: 用于 Dreaming, Target Selection, Social Reflection 等
	 * - System Prompt: 调用者自定义 (任务专用)
	 * - Temperature: 默认 0.3 (确定性分析)
	 * - JSON Mode: 可选
	 * - MaxTokens: 可限制输出长度
	 * - 返回原始字符串
	 */
	void SendFunctionalRequest(
		const FString& SystemPrompt,
		const FString& UserPrompt,
		FOnLLMResponseRaw OnComplete,
		float Temperature = 0.3f,
		bool bJsonMode = false,
		int32 MaxTokens = 256
	);

	// ============================================================
	// DEPRECATED: Old API (forwards to new methods, no breaking change)
	// ============================================================

	UE_DEPRECATED(5.0, "Use SendRoleplayRequest instead.")
	void SendRequest(const FString& UserInput, FOnLLMResponse OnComplete);

	UE_DEPRECATED(5.0, "Use SendFunctionalRequest instead.")
	void SendRequestRaw(const FString& Prompt, FOnLLMResponseRaw OnComplete);

private:
	// Shared HTTP sending logic
	void SendHTTPRequest(
		const FString& SystemPrompt,
		const FString& UserPrompt,
		float Temperature,
		bool bJsonMode,
		int32 MaxTokens,
		TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> OnComplete
	);

	// Response handlers
	void HandleRoleplayResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void HandleFunctionalResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	// Parse LLM content string from API envelope (choices[0].message.content)
	bool ExtractContentFromResponse(FHttpResponsePtr Response, bool bWasSuccessful, FString& OutContent);

	FString ApiKey;
	FString ApiUrl;

	// Build the static roleplay system prompt (Instructions + MentalState Schema)
	FString BuildRoleplaySystemPrompt() const;

	// ✅ 并发回调存储
	TMap<FHttpRequestPtr, FOnLLMResponse> PendingCallbacks;
	TMap<FHttpRequestPtr, FOnLLMResponseRaw> PendingRawCallbacks;

	// 语义映射器
	UPROPERTY()
	USentimentMapper* SentimentMapper;
};
