// LLMCommunicator.cpp

#include "LLMCommunicator.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "UtilityAI/SentimentMapping.h"

void ULLMCommunicator::Init(const FString& InApiKey, const FString& InUrl)
{
    ApiKey = InApiKey;
    ApiUrl = InUrl;
    
    // 初始化语义映射器 / Initialize Sentiment Mapper
    SentimentMapper = NewObject<USentimentMapper>(this);
    SentimentMapper->Initialize();
    UE_LOG(LogTemp, Log, TEXT("[LLMCommunicator] SentimentMapper initialized"));
}

void ULLMCommunicator::SendRequest(const FString& UserInput, FOnLLMResponse OnComplete)
{
    // 1. Create the HTTP Request first
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ApiUrl);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    
    // Safety check for API Key
    if (ApiKey.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] API Key is empty! Call Init() first."));
        FMentalState EmptyState;
        OnComplete.ExecuteIfBound(false, EmptyState);
        return;
    }
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *ApiKey));

    // 2. Construct the JSON Payload
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    RootObject->SetStringField("model", "deepseek-chat"); // Or "deepseek-reasoner"
    RootObject->SetNumberField("temperature", 0.7);

    // Enable JSON Mode (Crucial for stability)
    TSharedPtr<FJsonObject> ResponseFormatObj = MakeShareable(new FJsonObject());
    ResponseFormatObj->SetStringField("type", "json_object");
    RootObject->SetObjectField("response_format", ResponseFormatObj);

    // Construct Messages Array
    TArray<TSharedPtr<FJsonValue>> MessagesArray;

    // --- SYSTEM MESSAGE (The Rules) ---
    TSharedPtr<FJsonObject> SystemMsg = MakeShareable(new FJsonObject());
    SystemMsg->SetStringField("role", "system");
    
    // ✅ 使用宏自动生成字段列表
    FString FieldsList;
    #define ADD_FIELD_TO_PROMPT(Name, DefaultValue, DisplayName, Description) \
        FieldsList += FString::Printf(TEXT("  \"%s\": float,\n"), TEXT(#Name));
    
    MENTAL_STATE_FIELDS(ADD_FIELD_TO_PROMPT)
    
    #undef ADD_FIELD_TO_PROMPT
    
    // 移除最后的逗号和换行
    if (FieldsList.Len() > 0)
    {
        FieldsList = FieldsList.Left(FieldsList.Len() - 2) + TEXT("\n");
    }
    
    // 构建完整的 System Prompt
    FString SystemPrompt = FString::Printf(TEXT(
        "You are an AI game engine. Analyze the input and output a STRICT JSON object.\n"
        "Return specific float values (0.0 to 1.0) for these fields:\n"
        "{\n"
        "%s"
        "}\n"
        "Do not include markdown formatting (```json). Just raw JSON."
    ), *FieldsList);
    
    SystemMsg->SetStringField("content", SystemPrompt);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(SystemMsg)));

    // --- USER MESSAGE (The Input) ---
    TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject());
    UserMsg->SetStringField("role", "user");
    UserMsg->SetStringField("content", UserInput);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMsg)));

    RootObject->SetArrayField("messages", MessagesArray);

    // 3. Serialize to String
    FString RequestBody;
    // TJsonWriterFactory is used to safely write to FString
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    Request->SetContentAsString(RequestBody);

    // Detailed logging: Print full request information
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("[LLM] >>> Sending Request"));
    UE_LOG(LogTemp, Log, TEXT("[LLM] URL: %s"), *ApiUrl);
    UE_LOG(LogTemp, Log, TEXT("[LLM] User Input: %s"), *UserInput);
    UE_LOG(LogTemp, Verbose, TEXT("[LLM] Full Request Body:\n%s"), *RequestBody);
    UE_LOG(LogTemp, Warning, TEXT("========================================"));

    // ✅ 4. Store callback in TMap (instead of overwriting CurrentCallback)
    PendingCallbacks.Add(Request, OnComplete);

    // 5. Bind Callback & Send
    Request->OnProcessRequestComplete().BindUObject(this, &ULLMCommunicator::OnResponseReceived);
    Request->ProcessRequest();
}

void ULLMCommunicator::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FMentalState ResultState; // Default initialized to 0.0f
    
    // ✅ 1. Retrieve the callback from TMap
    FOnLLMResponse* CallbackPtr = PendingCallbacks.Find(Request);
    if (!CallbackPtr)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] Callback not found for request %p! This should never happen."), Request.Get());
        return;
    }
    
    // Copy the callback and remove from map immediately
    FOnLLMResponse Callback = *CallbackPtr;
    PendingCallbacks.Remove(Request);
    
    // Detailed logging: Start processing response
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("[LLM] <<< Response Received"));

    // --- Network & Protocol Validation ---
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] ERROR: Network Connection Failed."));
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        Callback.ExecuteIfBound(false, ResultState);
        return;
    }

    int32 ResponseCode = Response->GetResponseCode();
    FString ResponseString = Response->GetContentAsString();
    
    UE_LOG(LogTemp, Log, TEXT("[LLM] Response Code: %d"), ResponseCode);
    UE_LOG(LogTemp, Verbose, TEXT("[LLM] Full Response:\n%s"), *ResponseString);
    
    if (ResponseCode != 200)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] ERROR: API Error Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Error, TEXT("[LLM] Error Response: %s"), *ResponseString);
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        Callback.ExecuteIfBound(false, ResultState);
        return;
    }

    // --- Outer JSON Parsing (The API Envelope) ---
    TSharedPtr<FJsonObject> JsonResponse;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

    if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] Failed to deserialize API Root JSON."));
        Callback.ExecuteIfBound(false, ResultState);
        return;
    }

    // Navigate: choices[0] -> message -> content
    const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
    if (JsonResponse->TryGetArrayField("choices", ChoicesArray) && ChoicesArray->Num() > 0)
    {
        TSharedPtr<FJsonObject> FirstChoice = (*ChoicesArray)[0]->AsObject();
        TSharedPtr<FJsonObject> MessageObj = FirstChoice->GetObjectField("message");
        FString InnerContentString = MessageObj->GetStringField("content");

        // Cleanup Markdown if present (e.g. ```json ... ```)
        InnerContentString = InnerContentString.Replace(TEXT("```json"), TEXT("")).Replace(TEXT("```"), TEXT("")).TrimStartAndEnd();

        // 🔍 详细日志：打印 LLM 返回的内容
        UE_LOG(LogTemp, Log, TEXT("[LLM] Content from LLM:\n%s"), *InnerContentString);

        // --- Manual JSON Parsing (More Reliable) ---
        TSharedPtr<FJsonObject> InnerJsonObject;
        TSharedRef<TJsonReader<>> InnerReader = TJsonReaderFactory<>::Create(InnerContentString);
        
        if (FJsonSerializer::Deserialize(InnerReader, InnerJsonObject) && InnerJsonObject.IsValid())
        {
            // 手动提取每个字段，支持标签和数值两种格式
            // Manually extract each field, supporting both tags and numerical values
            #define EXTRACT_FIELD(Name, DefaultValue, DisplayName, Description) \
                if (InnerJsonObject->HasField(TEXT(#Name))) \
                { \
                    TSharedPtr<FJsonValue> FieldValue = InnerJsonObject->TryGetField(TEXT(#Name)); \
                    if (FieldValue->Type == EJson::String) \
                    { \
                        /* LLM 返回标签 (新格式) */ \
                        FString Tag = FieldValue->AsString(); \
                        if (SentimentMapper) \
                        { \
                            float MappedValue = SentimentMapper->TagToValue(Tag); \
                            ResultState.Name = (MappedValue >= 0.0f) ? MappedValue : DefaultValue; \
                            UE_LOG(LogTemp, Verbose, TEXT("[LLM] %s: Tag '%s' -> %.2f"), TEXT(#Name), *Tag, ResultState.Name); \
                        } \
                        else \
                        { \
                            UE_LOG(LogTemp, Warning, TEXT("[LLM] SentimentMapper not initialized!")); \
                            ResultState.Name = DefaultValue; \
                        } \
                    } \
                    else if (FieldValue->Type == EJson::Number) \
                    { \
                        /* LLM 返回数值 (旧格式，兼容) */ \
                        ResultState.Name = FieldValue->AsNumber(); \
                        UE_LOG(LogTemp, Verbose, TEXT("[LLM] %s: Number %.2f"), TEXT(#Name), ResultState.Name); \
                    } \
                    else \
                    { \
                        ResultState.Name = DefaultValue; \
                    } \
                } \
                else \
                { \
                    ResultState.Name = DefaultValue; \
                }
            
            MENTAL_STATE_FIELDS(EXTRACT_FIELD)
            
            #undef EXTRACT_FIELD
            
            // SUCCESS!
            UE_LOG(LogTemp, Warning, TEXT("[LLM] SUCCESS! Parsed MentalState:"));
            
            // 生理层 (Physiological)
            UE_LOG(LogTemp, Log, TEXT("  [Physiological] Hunger=%.2f, Energy=%.2f"), 
                   ResultState.Hunger, ResultState.Energy);
            
            // 安全层 (Safety)
            UE_LOG(LogTemp, Log, TEXT("  [Safety] Perceived_Threat=%.2f, Resource_Anxiety=%.2f"), 
                   ResultState.Perceived_Threat, ResultState.Resource_Anxiety);
            
            // 社交层 (Love/Belonging)
            UE_LOG(LogTemp, Log, TEXT("  [Social] Loneliness=%.2f, Trust=%.2f"), 
                   ResultState.Loneliness, ResultState.Trust);
            
            // 尊严层 (Esteem)
            UE_LOG(LogTemp, Log, TEXT("  [Esteem] Anger=%.2f, Social_Status=%.2f"), 
                   ResultState.Anger, ResultState.Social_Status);
            
            // 自我实现层 (Self-Actualization)
            UE_LOG(LogTemp, Log, TEXT("  [Self-Actualization] Duty_Urgency=%.2f, Curiosity=%.2f"), 
                   ResultState.Duty_Urgency, ResultState.Curiosity);
            
            UE_LOG(LogTemp, Warning, TEXT("========================================"));
            Callback.ExecuteIfBound(true, ResultState);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[LLM] ERROR: Failed to parse inner JSON!"));
            UE_LOG(LogTemp, Error, TEXT("[LLM] Content was: %s"), *InnerContentString);
            UE_LOG(LogTemp, Warning, TEXT("========================================"));
            Callback.ExecuteIfBound(false, ResultState);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] 'choices' field missing or empty in response."));
        Callback.ExecuteIfBound(false, ResultState);
    }
}

// 在文件末尾添加以下两个函数的实现

void ULLMCommunicator::SendRequestRaw(const FString& Prompt, FOnLLMResponseRaw OnComplete)
{
    // 1. 创建请求
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ApiUrl);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *ApiKey));

    // 2. 构建 JSON
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    RootObject->SetStringField("model", "deepseek-chat");
    RootObject->SetNumberField("temperature", 0.7);

    // 【区别点】：这里不需要强制 JSON Mode (response_format)，给 LLM 更大的自由度
    // 除非你在 Prompt 里明确要求它输出 JSON（我们在 Dreaming 逻辑里确实要求了，但这由 Prompt 决定）

    TArray<TSharedPtr<FJsonValue>> MessagesArray;

    // --- SYSTEM MESSAGE ---
    // 【区别点】：使用通用的 System Prompt，而不是强制数值
    TSharedPtr<FJsonObject> SystemMsg = MakeShareable(new FJsonObject());
    SystemMsg->SetStringField("role", "system");
    SystemMsg->SetStringField("content", "You are an intelligent game AI assistant."); 
    MessagesArray.Add(MakeShareable(new FJsonValueObject(SystemMsg)));

    // --- USER MESSAGE ---
    TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject());
    UserMsg->SetStringField("role", "user");
    UserMsg->SetStringField("content", Prompt);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMsg)));

    RootObject->SetArrayField("messages", MessagesArray);

    // 3. 序列化发送
    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    Request->SetContentAsString(RequestBody);

    // ✅ 4. Store callback in TMap
    PendingRawCallbacks.Add(Request, OnComplete);

    // 5. 绑定到 OnResponseReceivedRaw
    Request->OnProcessRequestComplete().BindUObject(this, &ULLMCommunicator::OnResponseReceivedRaw);
    Request->ProcessRequest();
    
    //UE_LOG(LogTemp, Log, TEXT("[LLM Raw] Request Sent (ID: %p)"), Request.Get());
}

void ULLMCommunicator::OnResponseReceivedRaw(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // ✅ 1. Retrieve the callback from TMap
    FOnLLMResponseRaw* CallbackPtr = PendingRawCallbacks.Find(Request);
    if (!CallbackPtr)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM Raw] Callback not found for request %p!"), Request.Get());
        return;
    }
    
    // Copy the callback and remove from map immediately
    FOnLLMResponseRaw Callback = *CallbackPtr;
    PendingRawCallbacks.Remove(Request);
    
    UE_LOG(LogTemp, Log, TEXT("[LLM Raw] Processing response for request %p"), Request.Get());
    
    if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM Raw] Network Error"));
        Callback.ExecuteIfBound(false, "");
        return;
    }

    // 解析 JSON
    TSharedPtr<FJsonObject> JsonResponse;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

    if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
    {
        // 提取 choices[0].message.content
        const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
        if (JsonResponse->TryGetArrayField("choices", ChoicesArray) && ChoicesArray->Num() > 0)
        {
            TSharedPtr<FJsonObject> FirstChoice = (*ChoicesArray)[0]->AsObject();
            TSharedPtr<FJsonObject> MessageObj = FirstChoice->GetObjectField("message");
            FString Content = MessageObj->GetStringField("content");

            // 清理 Markdown (```json ... ```) 以防万一
            Content = Content.Replace(TEXT("```json"), TEXT("")).Replace(TEXT("```"), TEXT("")).TrimStartAndEnd();

            // 【成功】：直接返回字符串，不转 Struct
            UE_LOG(LogTemp, Log, TEXT("[LLM Raw] Success: %s"), *Content);
            Callback.ExecuteIfBound(true, Content);
            return;
        }
    }

    Callback.ExecuteIfBound(false, "");
}