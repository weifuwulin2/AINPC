# Bug修复补丁指南

## 🔧 P0级别修复 (必须立即修复)

### 修复 #1: MemoryComponent 初始化错误

**文件:** `CognitionComponent.h` 和 `CognitionComponent.cpp`

**步骤1:** 在头文件中添加构造函数声明
```cpp
// CognitionComponent.h
class AINPC_API UCognitionComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UCognitionComponent();  // ✅ 添加这一行
    virtual void BeginPlay() override;
    // ...
};
```

**步骤2:** 修改 `.cpp` 文件
```cpp
// CognitionComponent.cpp

// ✅ 在构造函数中创建子组件
UCognitionComponent::UCognitionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // ✅ 移到这里
    MemoryComp = CreateDefaultSubobject<UMemoryComponent>(TEXT("Memory"));
}

void UCognitionComponent::BeginPlay()
{
    Super::BeginPlay();

    // 1. 初始化 LLM 服务对象
    LLMService = NewObject<ULLMCommunicator>(this);

    // ❌ 删除这一行
    // MemoryComp = CreateDefaultSubobject<UMemoryComponent>(TEXT("Memory"));
    
    // ... 其余代码保持不变
}
```

---

### 修复 #2: Prompt 传递错误

**文件:** `CognitionComponent.cpp`

**修改 `ProcessStimulus` 函数:**
```cpp
void UCognitionComponent::ProcessStimulus(FString SituationDescription)
{
    // 1. 存入新记忆
    if (MemoryComp)
    {
        MemoryComp->AddMemory(SituationDescription);
    }

    // 2. 检索旧记忆
    FString ContextMemory = "";
    if (MemoryComp)
    {
        ContextMemory = MemoryComp->RetrieveRelevantMemories(SituationDescription);
    }

    // 3. 构建完整Prompt
    FString Prompt = FString::Printf(TEXT(
        "Memories:\n%s\n"
        "Event: %s\n"
        "Task: 1. Analyze mental state (Anger, Fear, Confidence, SocialBattery, Hunger).\n"
        "      2. Rate the 'Importance' of this event (0.0 to 1.0).\n"
        "Output JSON: {\"Anger\":..., \"Fear\":..., \"Confidence\":..., \"SocialBattery\":..., \"Hunger\":..., \"Importance\":...}"
    ), *ContextMemory, *SituationDescription);
    
    // ✅ 修复：使用 Prompt 而不是 SituationDescription
    LLMService->SendRequest(
        Prompt,  // ✅ 改为这里
        FOnLLMResponse::CreateUObject(this, &UCognitionComponent::OnLLMReply)
    );
}
```

---

## 🔧 P1级别修复 (强烈建议)

### 修复 #3: 统一数据结构

**方案A: 删除 FMentalState，统一使用 UNPCMentalState**

这需要大量重构，暂时不推荐。

**方案B: 添加转换函数 (推荐)**

**文件:** `UNPCMentalState.h`

```cpp
// UNPCMentalState.h
#include "LLM/LLMCommunicator.h"  // 引入 FMentalState

UCLASS()
class AINPC_API UNPCMentalState : public UObject
{
    GENERATED_BODY()
    
public:
    UNPCMentalState();
    
    // ... 现有变量 ...
    
    // ✅ 新增：从结构体更新
    UFUNCTION(BlueprintCallable, Category = "Mental State")
    void UpdateFromStruct(const FMentalState& NewState);
    
    // ✅ 新增：转换为结构体
    UFUNCTION(BlueprintCallable, Category = "Mental State")
    FMentalState ToStruct() const;
    
    void ResetState();
};
```

**文件:** `UNPCMentalState.cpp`

```cpp
// UNPCMentalState.cpp

void UNPCMentalState::UpdateFromStruct(const FMentalState& NewState)
{
    Anger = FMath::Clamp(NewState.Anger, 0.0f, 1.0f);
    Fear = FMath::Clamp(NewState.Fear, 0.0f, 1.0f);
    Confidence = FMath::Clamp(NewState.Confidence, 0.0f, 1.0f);
    SocialBattery = FMath::Clamp(NewState.SocialBattery, 0.0f, 1.0f);
    Hunger = FMath::Clamp(NewState.Hunger, 0.0f, 1.0f);
}

FMentalState UNPCMentalState::ToStruct() const
{
    FMentalState Result;
    Result.Anger = Anger;
    Result.Fear = Fear;
    Result.Confidence = Confidence;
    Result.SocialBattery = SocialBattery;
    Result.Hunger = Hunger;
    return Result;
}
```

**文件:** `UtilityAIController.cpp`

```cpp
// UtilityAIController.cpp

void AUtilityAIController::OnMindUpdated(const FMentalState& NewState)
{
    if (MentalState)
    {
        // ✅ 使用新的转换函数
        MentalState->UpdateFromStruct(NewState);
        
        // 可选：打印日志
        UE_LOG(LogTemp, Log, TEXT("[Controller] Mental State Updated: Anger=%.2f, Fear=%.2f"), 
               MentalState->Anger, MentalState->Fear);
    }
}
```

---

### 修复 #4: 并发请求冲突

**文件:** `LLMCommunicator.h`

```cpp
// LLMCommunicator.h
#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "UObject/NoExportTypes.h"
#include "LLMCommunicator.generated.h"

// ... 现有结构体和委托 ...

UCLASS()
class AINPC_API ULLMCommunicator : public UObject
{
    GENERATED_BODY()

public:
    void Init(const FString& InApiKey, const FString& InUrl = TEXT("https://api.deepseek.com/chat/completions"));
    void SendRequest(const FString& UserInput, FOnLLMResponse OnComplete);
    void SendRequestRaw(const FString& Prompt, FOnLLMResponseRaw OnComplete);
    
private:
    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnResponseReceivedRaw(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    
    FString ApiKey;
    FString ApiUrl;
    
    // ✅ 改为使用 TMap 存储多个回调
    TMap<FHttpRequestPtr, FOnLLMResponse> PendingCallbacks;
    TMap<FHttpRequestPtr, FOnLLMResponseRaw> PendingRawCallbacks;
};
```

**文件:** `LLMCommunicator.cpp`

```cpp
// LLMCommunicator.cpp

void ULLMCommunicator::SendRequest(const FString& UserInput, FOnLLMResponse OnComplete)
{
    // ✅ 不再直接覆盖 CurrentCallback
    
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ApiUrl);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    
    if (ApiKey.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] API Key is empty!"));
        FMentalState EmptyState;
        OnComplete.ExecuteIfBound(false, EmptyState);
        return;
    }
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *ApiKey));

    // ... 构建JSON的代码保持不变 ...

    Request->SetContentAsString(RequestBody);

    // ✅ 存储回调到 Map
    PendingCallbacks.Add(Request, OnComplete);
    
    // ✅ 绑定回调时传递 Request 指针
    Request->OnProcessRequestComplete().BindUObject(this, &ULLMCommunicator::OnResponseReceived);
    Request->ProcessRequest();
}

void ULLMCommunicator::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FMentalState ResultState;
    
    // ✅ 从 Map 中获取对应的回调
    FOnLLMResponse* CallbackPtr = PendingCallbacks.Find(Request);
    if (!CallbackPtr)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] Callback not found for request!"));
        return;
    }
    
    FOnLLMResponse Callback = *CallbackPtr;
    PendingCallbacks.Remove(Request);  // ✅ 清理

    // --- 原有的验证逻辑 ---
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] Network Connection Failed."));
        Callback.ExecuteIfBound(false, ResultState);
        return;
    }

    if (Response->GetResponseCode() != 200)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] API Error Code: %d"), Response->GetResponseCode());
        Callback.ExecuteIfBound(false, ResultState);
        return;
    }

    // ... 解析JSON的代码保持不变 ...
    
    // ✅ 使用局部变量 Callback 而不是 CurrentCallback
    if (FJsonObjectConverter::JsonObjectStringToUStruct(InnerContentString, &ResultState, 0, 0))
    {
        Callback.ExecuteIfBound(true, ResultState);
    }
    else
    {
        Callback.ExecuteIfBound(false, ResultState);
    }
}

// ✅ 同样修改 SendRequestRaw 和 OnResponseReceivedRaw
void ULLMCommunicator::SendRequestRaw(const FString& Prompt, FOnLLMResponseRaw OnComplete)
{
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    // ... 设置请求的代码 ...
    
    // ✅ 存储回调
    PendingRawCallbacks.Add(Request, OnComplete);
    
    Request->OnProcessRequestComplete().BindUObject(this, &ULLMCommunicator::OnResponseReceivedRaw);
    Request->ProcessRequest();
}

void ULLMCommunicator::OnResponseReceivedRaw(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // ✅ 从 Map 获取回调
    FOnLLMResponseRaw* CallbackPtr = PendingRawCallbacks.Find(Request);
    if (!CallbackPtr)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM Raw] Callback not found!"));
        return;
    }
    
    FOnLLMResponseRaw Callback = *CallbackPtr;
    PendingRawCallbacks.Remove(Request);
    
    // ... 原有逻辑，使用 Callback 而不是 CurrentRawCallback ...
}
```

---

## 🔧 P2级别修复 (可选优化)

### 修复 #5: 启用 Dreaming 功能

**文件:** `UtilityAIController.h`

```cpp
// UtilityAIController.h
class AINPC_API AUtilityAIController : public AAIController
{
    // ... 现有代码 ...
    
protected:
    // ✅ 新增：Dreaming 定时器
    FTimerHandle DreamingTimerHandle;
};
```

**文件:** `UtilityAIController.cpp`

```cpp
// UtilityAIController.cpp

void AUtilityAIController::BeginPlay()
{
    Super::BeginPlay();
    
    // ... 现有初始化代码 ...
    
    // ✅ 新增：启动 Dreaming 定时器
    if (CognitionComp)
    {
        GetWorldTimerManager().SetTimer(
            DreamingTimerHandle,
            [this]() 
            { 
                if (CognitionComp)
                {
                    CognitionComp->StartDreaming();
                    UE_LOG(LogTemp, Log, TEXT("[Controller] Dreaming cycle triggered."));
                }
            },
            300.0f,  // 每5分钟执行一次
            true     // 循环
        );
    }
}
```

---

### 修复 #6: 惯性奖励配置生效

**文件:** `UtilityActionBase.cpp`

```cpp
// UtilityActionBase.cpp

float UUtilityActionBase::CalculateScore(UNPCMentalState* MentalState, AAIController* Controller)
{
    // ... 现有的冷却和多因子计算代码 ...
    
    // ❌ 删除被注释的代码块
    
    // ✅ 惯性奖励已经在 UtilityAIComponent::EvaluateAndDecide 中处理
    // 如果需要在这里处理，可以添加一个参数 bIsCurrentAction
    
    return FinalScore;
}
```

**或者，如果要在Action内部处理:**

```cpp
// UtilityActionBase.h
class AINPC_API UUtilityActionBase : public UObject
{
    // ...
    
    // ✅ 新增参数
    virtual float CalculateScore(UNPCMentalState* MentalState, AAIController* Controller, bool bIsCurrentAction = false);
};

// UtilityActionBase.cpp
float UUtilityActionBase::CalculateScore(UNPCMentalState* MentalState, AAIController* Controller, bool bIsCurrentAction)
{
    // ... 现有计算 ...
    
    // ✅ 应用惯性奖励
    if (bIsCurrentAction && InertiaBonus > 0.0f)
    {
        FinalScore += InertiaBonus;
    }
    
    return FinalScore;
}

// UtilityAIComponent.cpp
void UUtilityAIComponent::EvaluateAndDecide()
{
    // ...
    for (UUtilityActionBase* Action : AvailableActions)
    {
        // ✅ 传递是否为当前Action
        float Score = Action->CalculateScore(State, OwnerController, Action == CurrentAction);
        
        // ❌ 删除这里的惯性奖励
        // if (Action == CurrentAction) Score += Action->InertiaBonus;
        
        // ...
    }
}
```

---

## 📋 修复检查清单

- [ ] **P0-1:** 修复 `MemoryComponent` 初始化
- [ ] **P0-2:** 修复 `Prompt` 传递错误
- [ ] **P1-3:** 添加数据结构转换函数
- [ ] **P1-4:** 修复并发请求冲突
- [ ] **P2-5:** 启用 Dreaming 功能
- [ ] **P2-6:** 修复惯性奖励配置
- [ ] **测试:** 编译项目确认无错误
- [ ] **测试:** 运行游戏验证AI行为
- [ ] **测试:** 检查日志确认数据流转正常

---

## 🧪 测试建议

### 1. 单元测试 (可选)
创建测试用例验证:
- `UNPCMentalState::UpdateFromStruct()` 正确转换数据
- `LLMCommunicator` 能处理多个并发请求
- `MemoryComponent` 正确存储和检索记忆

### 2. 集成测试
1. 在编辑器中放置一个AI角色
2. 触发感知事件 (靠近AI、攻击AI)
3. 检查日志输出:
   ```
   [Sensory] Stimulus: Saw Player
   [Cognition] Processing: Saw Player
   [Memory] Added memory: Saw Player
   [LLM] Request Sent: Memories:...
   [LLM] Success! Parsed: Anger=0.30, Fear=0.20
   [Controller] Mental State Updated: Anger=0.30
   [UtilityAI] Switch Action: Attack (Score: 0.45)
   ```

### 3. 性能测试
- 监控 LLM 请求频率 (不应超过每秒1次)
- 检查内存泄漏 (TMap 是否正确清理)
- 验证 Utility AI Tick 性能 (应 < 1ms)

---

**修复完成后，建议提交 Git commit:**
```bash
git add .
git commit -m "Fix critical bugs in LLM+Utility AI data flow

- Fixed MemoryComponent initialization in BeginPlay
- Fixed Prompt parameter in ProcessStimulus
- Added FMentalState conversion functions
- Resolved concurrent request callback conflicts
- Enabled Dreaming timer
- Fixed inertia bonus configuration"
```
