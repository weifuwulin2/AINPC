# LLM + Utility AI 架构重构实施计划

**基于:** `LLM_Utility_Architecture.md`  
**目标:** 将LLM从"情绪传感器"升级为"认知大脑"

---

## 📋 实施清单

### ✅ Phase 1: 数据结构重构 (2-3小时)

#### 1.1 创建新的认知状态结构

**文件:** `Source/AINPC/Public/Data/CognitiveState.h` (新建)

```cpp
#pragma once

#include "CoreMinimal.h"
#include "CognitiveState.generated.h"

/**
 * 认知状态 - LLM的输出格式
 * 包含情境理解、意图和行为偏好
 */
USTRUCT(BlueprintType)
struct AINPC_API FCognitiveState
{
    GENERATED_BODY()
    
    // ========== 情境评估 ==========
    
    /** LLM对当前情况的总结 */
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Situation")
    FString SituationSummary;
    
    /** 威胁程度 (0.0 = 无威胁, 1.0 = 极度危险) */
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Situation")
    float ThreatLevel = 0.0f;
    
    /** 机会程度 (0.0 = 无机会, 1.0 = 绝佳机会) */
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Situation")
    float OpportunityLevel = 0.0f;
    
    // ========== 意图系统 ==========
    
    /** 主要意图 (escape, attack, observe, interact, rest) */
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Intent")
    FString PrimaryIntent;
    
    /** 意图的确定性 (0.0 = 不确定, 1.0 = 非常确定) */
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Intent")
    float IntentConfidence = 0.5f;
    
    // ========== 行为偏好 ==========
    
    /** 对各个动作的偏好程度 (ActionName -> Preference 0.0-1.0) */
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Preferences")
    TMap<FString, float> ActionPreferences;
    
    // ========== 情绪状态（辅助信息）==========
    
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Emotions")
    float Anger = 0.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Emotions")
    float Fear = 0.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Emotions")
    float Confidence = 0.5f;
    
    // ========== 元认知（调试用）==========
    
    /** LLM的推理过程 */
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Meta")
    FString Reasoning;
    
    /** 时间戳 */
    UPROPERTY(BlueprintReadOnly, Category = "Cognition|Meta")
    float Timestamp = 0.0f;
};
```

#### 1.2 更新LLMCommunicator

**文件:** `Source/AINPC/LLM/LLMCommunicator.h`

```cpp
// 添加新的回调类型
DECLARE_DELEGATE_TwoParams(FOnCognitiveResponse, bool /*bSuccess*/, const FCognitiveState& /*State*/);

class AINPC_API ULLMCommunicator : public UObject
{
    // ... 现有代码 ...
    
    /** 新的认知请求接口 */
    void SendCognitiveRequest(const FString& SituationDescription, FOnCognitiveResponse OnComplete);
    
    /** 保留旧接口以兼容 */
    void SendRequest(const FString& UserInput, FOnLLMResponse OnComplete);
    
private:
    void OnCognitiveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    
    TMap<FHttpRequestPtr, FOnCognitiveResponse> PendingCognitiveCallbacks;
};
```

#### 1.3 更新CognitionComponent

**文件:** `Source/AINPC/Components/CognitionComponent.h`

```cpp
// 更新事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCognitiveStateChanged, const FCognitiveState&, NewState);

class AINPC_API UCognitionComponent : public UActorComponent
{
    // ... 现有代码 ...
    
    UPROPERTY(BlueprintAssignable, Category = "Cognition")
    FOnCognitiveStateChanged OnCognitiveStateChanged;
    
    /** 当前认知状态 */
    UPROPERTY(BlueprintReadOnly, Category = "Cognition")
    FCognitiveState CurrentCognitiveState;
    
private:
    void OnCognitiveReply(bool bSuccess, const FCognitiveState& NewState);
};
```

---

### ✅ Phase 2: Prompt重构 (1-2小时)

#### 2.1 新的System Prompt

**文件:** `Source/AINPC/LLM/LLMCommunicator.cpp`

在 `SendCognitiveRequest()` 中使用新的Prompt：

```cpp
void ULLMCommunicator::SendCognitiveRequest(const FString& SituationDescription, FOnCognitiveResponse OnComplete)
{
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ApiUrl);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *ApiKey));
    
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    RootObject->SetStringField("model", "deepseek-chat");
    RootObject->SetNumberField("temperature", 0.7);
    
    // 启用JSON模式
    TSharedPtr<FJsonObject> ResponseFormatObj = MakeShareable(new FJsonObject());
    ResponseFormatObj->SetStringField("type", "json_object");
    RootObject->SetObjectField("response_format", ResponseFormatObj);
    
    TArray<TSharedPtr<FJsonValue>> MessagesArray;
    
    // ========== 新的System Prompt ==========
    TSharedPtr<FJsonObject> SystemMsg = MakeShareable(new FJsonObject());
    SystemMsg->SetStringField("role", "system");
    
    FString SystemPrompt = TEXT(
        "You are the COGNITIVE BRAIN of an NPC in a game world.\n"
        "\n"
        "Your role:\n"
        "1. UNDERSTAND the situation from sensory input\n"
        "2. REASON about threats, opportunities, and context\n"
        "3. FORM intentions about what you want to achieve\n"
        "4. EXPRESS behavioral preferences (NOT direct commands)\n"
        "\n"
        "You do NOT control the body. You provide cognitive guidance.\n"
        "The motor system (Utility AI) translates your intentions into actions.\n"
        "\n"
        "Output JSON format:\n"
        "{\n"
        "  \"situation_summary\": \"Brief description\",\n"
        "  \"threat_level\": 0.0-1.0,\n"
        "  \"opportunity_level\": 0.0-1.0,\n"
        "  \"primary_intent\": \"escape|attack|observe|interact|rest\",\n"
        "  \"intent_confidence\": 0.0-1.0,\n"
        "  \"action_preferences\": {\n"
        "    \"Test_Flee\": 0.0-1.0,\n"
        "    \"Test_Attack\": 0.0-1.0,\n"
        "    \"Test_Idle\": 0.0-1.0\n"
        "  },\n"
        "  \"emotions\": {\n"
        "    \"anger\": 0.0-1.0,\n"
        "    \"fear\": 0.0-1.0,\n"
        "    \"confidence\": 0.0-1.0\n"
        "  },\n"
        "  \"reasoning\": \"Why this intent (1-2 sentences)\"\n"
        "}\n"
        "\n"
        "Example:\n"
        "Input: \"I see a player approaching. They damaged me recently.\"\n"
        "Output: {\n"
        "  \"situation_summary\": \"Hostile player returning, I'm wounded\",\n"
        "  \"threat_level\": 0.8,\n"
        "  \"opportunity_level\": 0.1,\n"
        "  \"primary_intent\": \"escape\",\n"
        "  \"intent_confidence\": 0.9,\n"
        "  \"action_preferences\": {\n"
        "    \"Test_Flee\": 0.9,\n"
        "    \"Test_Attack\": 0.1,\n"
        "    \"Test_Idle\": 0.0\n"
        "  },\n"
        "  \"emotions\": {\"anger\": 0.3, \"fear\": 0.8, \"confidence\": 0.2},\n"
        "  \"reasoning\": \"This player is dangerous and I'm hurt. I must escape.\"\n"
        "}"
    );
    
    SystemMsg->SetStringField("content", SystemPrompt);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(SystemMsg)));
    
    // User Message
    TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject());
    UserMsg->SetStringField("role", "user");
    UserMsg->SetStringField("content", SituationDescription);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMsg)));
    
    RootObject->SetArrayField("messages", MessagesArray);
    
    // 序列化并发送
    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    Request->SetContentAsString(RequestBody);
    
    PendingCognitiveCallbacks.Add(Request, OnComplete);
    Request->OnProcessRequestComplete().BindUObject(this, &ULLMCommunicator::OnCognitiveResponseReceived);
    Request->ProcessRequest();
}
```

#### 2.2 解析新的JSON响应

```cpp
void ULLMCommunicator::OnCognitiveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FCognitiveState ResultState;
    
    FOnCognitiveResponse* CallbackPtr = PendingCognitiveCallbacks.Find(Request);
    if (!CallbackPtr) return;
    
    FOnCognitiveResponse Callback = *CallbackPtr;
    PendingCognitiveCallbacks.Remove(Request);
    
    if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
    {
        Callback.ExecuteIfBound(false, ResultState);
        return;
    }
    
    // 解析JSON
    TSharedPtr<FJsonObject> JsonResponse;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
    
    if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
    {
        const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
        if (JsonResponse->TryGetArrayField("choices", ChoicesArray) && ChoicesArray->Num() > 0)
        {
            TSharedPtr<FJsonObject> FirstChoice = (*ChoicesArray)[0]->AsObject();
            TSharedPtr<FJsonObject> MessageObj = FirstChoice->GetObjectField("message");
            FString ContentString = MessageObj->GetStringField("content");
            
            ContentString = ContentString.Replace(TEXT("```json"), TEXT("")).Replace(TEXT("```"), TEXT("")).TrimStartAndEnd();
            
            TSharedPtr<FJsonObject> ContentJson;
            TSharedRef<TJsonReader<>> ContentReader = TJsonReaderFactory<>::Create(ContentString);
            
            if (FJsonSerializer::Deserialize(ContentReader, ContentJson) && ContentJson.IsValid())
            {
                // 解析情境评估
                ResultState.SituationSummary = ContentJson->GetStringField("situation_summary");
                ResultState.ThreatLevel = ContentJson->GetNumberField("threat_level");
                ResultState.OpportunityLevel = ContentJson->GetNumberField("opportunity_level");
                
                // 解析意图
                ResultState.PrimaryIntent = ContentJson->GetStringField("primary_intent");
                ResultState.IntentConfidence = ContentJson->GetNumberField("intent_confidence");
                
                // 解析行为偏好
                TSharedPtr<FJsonObject> PreferencesObj = ContentJson->GetObjectField("action_preferences");
                for (auto& Pair : PreferencesObj->Values)
                {
                    ResultState.ActionPreferences.Add(Pair.Key, Pair.Value->AsNumber());
                }
                
                // 解析情绪
                TSharedPtr<FJsonObject> EmotionsObj = ContentJson->GetObjectField("emotions");
                ResultState.Anger = EmotionsObj->GetNumberField("anger");
                ResultState.Fear = EmotionsObj->GetNumberField("fear");
                ResultState.Confidence = EmotionsObj->GetNumberField("confidence");
                
                // 解析推理
                ResultState.Reasoning = ContentJson->GetStringField("reasoning");
                ResultState.Timestamp = FPlatformTime::Seconds();
                
                UE_LOG(LogTemp, Warning, TEXT("[LLM Cognitive] SUCCESS!"));
                UE_LOG(LogTemp, Log, TEXT("  Intent: %s (Confidence: %.2f)"), 
                       *ResultState.PrimaryIntent, ResultState.IntentConfidence);
                UE_LOG(LogTemp, Log, TEXT("  Reasoning: %s"), *ResultState.Reasoning);
                
                Callback.ExecuteIfBound(true, ResultState);
                return;
            }
        }
    }
    
    Callback.ExecuteIfBound(false, ResultState);
}
```

---

### ✅ Phase 3: Utility AI重构 (2-3小时)

#### 3.1 更新UtilityActionBase的计算逻辑

**文件:** `Source/AINPC/Private/Base/UtilityActionBase.cpp`

```cpp
float UUtilityActionBase::CalculateScore(AAIController* Controller)
{
    if (!Controller) return 0.0f;
    
    // 冷却检查
    UWorld* World = Controller->GetWorld();
    if (!World) return 0.0f;
    
    float CurrentTime = World->GetTimeSeconds();
    if (CurrentTime - LastExecutedTime < CooldownTime)
    {
        return 0.0f;
    }
    
    // 获取认知状态
    AUtilityAIController* UtilityController = Cast<AUtilityAIController>(Controller);
    if (!UtilityController) return 0.0f;
    
    UCognitionComponent* CognitionComp = UtilityController->FindComponentByClass<UCognitionComponent>();
    if (!CognitionComp) return 0.0f;
    
    const FCognitiveState& CogState = CognitionComp->CurrentCognitiveState;
    
    float FinalScore = 0.0f;
    
    // ========== 1. LLM意图权重（核心！）==========
    float IntentWeight = 0.0f;
    
    if (CogState.ActionPreferences.Contains(ActionName))
    {
        IntentWeight = CogState.ActionPreferences[ActionName];
        IntentWeight *= CogState.IntentConfidence;  // 乘以LLM的确定性
        
        UE_LOG(LogTemp, Log, TEXT("    [%s] LLM Intent: %.2f (Confidence: %.2f)"),
               *ActionName, CogState.ActionPreferences[ActionName], CogState.IntentConfidence);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("    [%s] No intent preference from LLM!"), *ActionName);
    }
    
    // ========== 2. 环境可行性评估（Utility AI的职责）==========
    float FeasibilityScore = EvaluateFeasibility(Controller);
    
    // ========== 3. 综合计算 ==========
    // 意图权重占70%，可行性占30%
    float IntentContribution = IntentWeight * 0.7f;
    float FeasibilityContribution = FeasibilityScore * BaseWeight * 0.3f;
    
    FinalScore = IntentContribution + FeasibilityContribution;
    
    // ========== 4. 惯性奖励 ==========
    if (UUtilityAIComponent* UtilityComp = Controller->FindComponentByClass<UUtilityAIComponent>())
    {
        if (UtilityComp->GetCurrentAction() == this)
        {
            FinalScore += InertiaBonus;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("    [%s] Intent=%.2f (70%%), Feasibility=%.2f (30%%), Final=%.2f"),
           *ActionName, IntentContribution, FeasibilityContribution, FinalScore);
    
    return FinalScore;
}

float UUtilityActionBase::EvaluateFeasibility(AAIController* Controller)
{
    // 子类可以重写这个函数来添加特定的可行性检查
    // 默认返回1.0（完全可行）
    return 1.0f;
}
```

#### 3.2 为具体动作添加可行性检查

**文件:** `Source/AINPC/Private/Test/TestAction_Attack.cpp`

```cpp
float UTestAction_Attack::EvaluateFeasibility(AAIController* Controller)
{
    float Feasibility = 1.0f;
    
    APawn* ControlledPawn = Controller->GetPawn();
    if (!ControlledPawn) return 0.0f;
    
    // 检查是否有目标
    UWorld* World = Controller->GetWorld();
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
    if (!PlayerPawn) return 0.0f;
    
    // 距离检查
    float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
    
    if (Distance > AttackRange * 3.0f)
    {
        // 太远了，降低可行性
        Feasibility *= 0.3f;
    }
    else if (Distance > AttackRange * 1.5f)
    {
        // 有点远，但还可以
        Feasibility *= 0.7f;
    }
    // 在范围内，保持1.0
    
    UE_LOG(LogTemp, Verbose, TEXT("      [Attack Feasibility] Distance=%.1f, Score=%.2f"), Distance, Feasibility);
    
    return Feasibility;
}
```

---

### ✅ Phase 4: 向后兼容 (1小时)

#### 4.1 保留旧接口

```cpp
// 在CognitionComponent中添加适配器
void UCognitionComponent::ProcessStimulus(const FString& SituationDescription)
{
    if (bUseLegacyMode)
    {
        // 旧模式：使用FMentalState
        LLMService->SendRequest(
            SituationDescription,
            FOnLLMResponse::CreateUObject(this, &UCognitionComponent::OnLLMReply)
        );
    }
    else
    {
        // 新模式：使用FCognitiveState
        LLMService->SendCognitiveRequest(
            SituationDescription,
            FOnCognitiveResponse::CreateUObject(this, &UCognitionComponent::OnCognitiveReply)
        );
    }
}
```

---

## 🧪 测试计划

### 测试场景1：玩家接近受伤的NPC

**预期LLM输出：**
```json
{
  "primary_intent": "escape",
  "action_preferences": {
    "Test_Flee": 0.9,
    "Test_Attack": 0.1
  },
  "reasoning": "I'm wounded and the player is dangerous"
}
```

**预期Utility AI选择：** Flee

### 测试场景2：玩家远离，NPC满血

**预期LLM输出：**
```json
{
  "primary_intent": "observe",
  "action_preferences": {
    "Test_Idle": 0.7,
    "Test_Attack": 0.3
  },
  "reasoning": "No immediate threat, can relax"
}
```

**预期Utility AI选择：** Idle

---

## 📊 成功指标

1. **可解释性：** 每个决策都能追溯到LLM的推理
2. **灵活性：** 可以通过修改Prompt改变NPC性格
3. **性能：** LLM调用频率不变（仍然是事件驱动）
4. **兼容性：** 旧系统仍然可以工作

---

**预计总时间：** 6-9小时  
**优先级：** High  
**风险：** Medium（需要大量测试）
