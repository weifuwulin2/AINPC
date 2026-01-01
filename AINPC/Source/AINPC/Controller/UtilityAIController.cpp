// Fill out your copyright notice in the Description page of Project Settings.
#include "Controller/UtilityAIController.h"

#include "JsonObjectConverter.h"
#include "Base/UtilityActionBase.h"
#include "UtilityAI/UNPCMentalState.h"


AUtilityAIController::AUtilityAIController(): MentalState(nullptr), CurrentAction(nullptr)
{
    // 开启 Tick，让 AI 能实时反应
    PrimaryActorTick.bCanEverTick = true;
}

void AUtilityAIController::BeginPlay()
{
    Super::BeginPlay();
    
    MentalState = NewObject<UNPCMentalState>(this);
    if (MentalState)
    {
        MentalState->Anger = 0.9f;
        MentalState->Fear = 0.1f;
    }
    
    for (TSubclassOf<UUtilityActionBase> Class : ActionClasses)
    {
        if (Class)
        {
            UUtilityActionBase* NewAction = NewObject<UUtilityActionBase>(this, Class);
            if (NewAction)
            {
                AvailableActions.Add(NewAction);
                UE_LOG(LogTemp, Log, TEXT("Action Initialized: %s"), *NewAction->ActionName);
            }
        }
    }

    // 延迟 3 秒执行，避免游戏刚启动网络模块还没准备好，或者方便你切窗口看 Log
    FTimerHandle UnusedHandle;
    GetWorldTimerManager().SetTimer(UnusedHandle, [this]()
    {
        UE_LOG(LogTemp, Warning, TEXT("--- Test Starting ---"));
        RequestUpdateFromLLM(TEXT("I haven't eaten in 3 days and I see a zombie approaching!"));
    }, 3.0f, false);
}

void AUtilityAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 每帧思考 (优化建议：可以用 Timer 每 0.2s 跑一次)
    EvaluateUtilityLogic();
}

void AUtilityAIController::EvaluateUtilityLogic()
{
    if (!MentalState || AvailableActions.Num() == 0) return;

    UUtilityActionBase* BestAction = nullptr;
    float BestScore = -1.0f;

    // --- 1. 遍历打分 ---
    for (UUtilityActionBase* Action : AvailableActions)
    {
        float Score = Action->CalculateScore(MentalState);
        
        // 如果是当前动作，给点惯性分(Inertia)，防止频繁抖动
        if (Action == CurrentAction)
        {
            Score += 5.0f; 
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestAction = Action;
        }
    }

    // --- 2. 执行最优解 ---
    if (BestAction && BestAction != CurrentAction)
    {
        // 退出旧动作
        if (CurrentAction) CurrentAction->Exit(this);

        // 切换新动作
        CurrentAction = BestAction;
        CurrentAction->Execute(this);

        UE_LOG(LogTemp, Warning, TEXT("Switching to Action: %s (Score: %f)"), *CurrentAction->ActionName, BestScore);
    }
    else if (CurrentAction)
    {
        // 保持当前动作
        CurrentAction->Execute(this);
    }
}


// =================================================================
// 1. 发送请求函数
// =================================================================
void AUtilityAIController::RequestUpdateFromLLM(FString UserInput)
{
    // 创建 HTTP 请求
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL("https://api.deepseek.com/chat/completions");
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");

    // ⚠️ 安全警告：请务必去后台生成新Key，并在这里替换。
    // 你的旧 Key (sk-89...49) 已经泄露，请作废它！
    FString ApiKey = TEXT("sk-2ed20447f53f4cfa9bcde9105ee850f2"); 
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *ApiKey));

    // 构建 JSON Root
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    RootObject->SetStringField("model", "deepseek-chat");
    RootObject->SetNumberField("temperature", 0.7);

    // 开启 JSON Mode (强制 LLM 返回合法 JSON)
    TSharedPtr<FJsonObject> ResponseFormatObj = MakeShareable(new FJsonObject());
    ResponseFormatObj->SetStringField("type", "json_object");
    RootObject->SetObjectField("response_format", ResponseFormatObj);

    // 构建 Messages
    TArray<TSharedPtr<FJsonValue>> MessagesArray;

    // --- System Prompt ---
    TSharedPtr<FJsonObject> SystemMsg = MakeShareable(new FJsonObject());
    SystemMsg->SetStringField("role", "system");
    // 明确要求字段，防止幻觉
    FString SystemPrompt = TEXT(
        "You are an emotion analysis engine. "
        "Analyze the user input and output a STRICT JSON object matching this schema:\n"
        "{\n"
        "  \"Anger\": float (0.0-1.0),\n"
        "  \"Fear\": float (0.0-1.0),\n"
        "  \"Confidence\": float (0.0-1.0),\n"
        "  \"SocialBattery\": float (0.0-1.0),\n"
        "  \"Hunger\": float (0.0-1.0)\n"
        "}\n"
        "Do NOT include markdown formatting (like ```json). Just the raw JSON string."
    );
    SystemMsg->SetStringField("content", SystemPrompt);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(SystemMsg)));

    // --- User Prompt ---
    TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject());
    UserMsg->SetStringField("role", "user");
    UserMsg->SetStringField("content", UserInput);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMsg)));

    RootObject->SetArrayField("messages", MessagesArray);

    // 序列化
    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    Request->SetContentAsString(RequestBody);

    // 绑定回调
    Request->OnProcessRequestComplete().BindUObject(this, &AUtilityAIController::OnResponseReceived);

    // 发送
    Request->ProcessRequest();
    UE_LOG(LogTemp, Log, TEXT("[LLM] Request Sent: %s"), *UserInput);
}

// =================================================================
// 2. 接收响应函数 (包含由外向内的解析逻辑)
// =================================================================
void AUtilityAIController::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // 1. 基础网络检查
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] HTTP Request Failed or Network Error"));
        return;
    }

    if (Response->GetResponseCode() != 200)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] Error Code: %d, Message: %s"), Response->GetResponseCode(), *Response->GetContentAsString());
        return;
    }

    FString ResponseString = Response->GetContentAsString();
    // 调试打印：看看 DeepSeek 到底回了什么 (你会看到它是一个包含 choices 的大 JSON)
    // UE_LOG(LogTemp, Log, TEXT("[LLM] Raw Response: %s"), *ResponseString);

    // 2. 解析外层 API 响应 (Standard OpenAI Format)
    TSharedPtr<FJsonObject> JsonResponse;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

    if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
    {
        // 3. 提取 choices[0].message.content
        const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
        if (JsonResponse->TryGetArrayField("choices", ChoicesArray) && ChoicesArray->Num() > 0)
        {
            TSharedPtr<FJsonObject> FirstChoice = (*ChoicesArray)[0]->AsObject();
            TSharedPtr<FJsonObject> MessageObj = FirstChoice->GetObjectField("message");
            FString ContentString = MessageObj->GetStringField("content");

            // 4. 清理可能存在的 Markdown 标记 (DeepSeek 偶尔会加 ```json)
            ContentString = ContentString.Replace(TEXT("```json"), TEXT("")).Replace(TEXT("```"), TEXT("")).TrimStartAndEnd();

            UE_LOG(LogTemp, Log, TEXT("[LLM] Extracted Content: %s"), *ContentString);

            // 5. 将内部 Content 字符串转换为 Struct
            FMentalStateJson NewStateData;
            if (FJsonObjectConverter::JsonObjectStringToUStruct(ContentString, &NewStateData, 0, 0))
            {
                // === 成功解析！应用到游戏逻辑 ===
                if (MentalState) // 确保你的类成员变量 MentalState 已经初始化
                {
                    MentalState->Anger = NewStateData.Anger;
                    MentalState->Fear = NewStateData.Fear;
                    MentalState->Confidence = NewStateData.Confidence;
                    MentalState->SocialBattery = NewStateData.SocialBattery;
                    MentalState->Hunger = NewStateData.Hunger;

                    UE_LOG(LogTemp, Warning, TEXT(">>> AI BRAIN UPDATED <<< Anger: %.2f | Fear: %.2f | Social: %.2f"), 
                        MentalState->Anger, MentalState->Fear, MentalState->SocialBattery);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[LLM] MentalState pointer is NULL!"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[LLM] Failed to convert Content String to Struct. Content was: %s"), *ContentString);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] Failed to deserialize Root JSON response."));
    }
}