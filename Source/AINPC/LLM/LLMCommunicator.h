// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "UObject/NoExportTypes.h"
#include "LLMCommunicator.generated.h"

// 1. The Data Structure
USTRUCT(BlueprintType)
struct FMentalState
{
	GENERATED_BODY()
	UPROPERTY() float Anger = 0.0f;
	UPROPERTY() float Fear = 0.0f;
	UPROPERTY() float Confidence = 0.0f;
	UPROPERTY() float SocialBattery = 0.0f;
	UPROPERTY() float Hunger = 0.0f;
};

// 2. The Callback Delegate
DECLARE_DELEGATE_TwoParams(FOnLLMResponse, bool /*bSuccess*/, const FMentalState& /*State*/);
/**
 * 
 */
UCLASS()
class AINPC_API ULLMCommunicator : public UObject
{
	GENERATED_BODY()

public:
	void Init(const FString& InApiKey, const FString& InUrl = TEXT("https://api.deepseek.com/chat/completions"));
	void SendRequest(const FString& UserInput, FOnLLMResponse OnComplete);

private:
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	FString ApiKey;
	FString ApiUrl;
	FOnLLMResponse CurrentCallback;
};