// LLMCommunicator.cpp

#include "LLMCommunicator.h"
#include "AINPC.h"
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

    SentimentMapper = NewObject<USentimentMapper>(this);
    SentimentMapper->Initialize();
    LLM_LOG(Log, "Initialized (Dual-Mode API)");
}

// ============================================================
// Build Static Roleplay System Prompt
// ============================================================

FString ULLMCommunicator::BuildRoleplaySystemPrompt() const
{
    // --- Instructions Block (moved from CognitionComponent) ---
    FString SystemPrompt = TEXT(
        "You are an AI NPC brain in a game. Analyze the input and output a STRICT JSON object.\n"
        "The input contains your identity, faction worldview, and current situation.\n"
        "Use your [WORLDVIEW / FACTIONS] to determine friend vs foe. React accordingly - attack enemies, protect allies.\n"
        "Use INTENSITY TAGS (None/Slight/Moderate/Strong/Extreme or natural language equivalents) for all emotional fields. Do NOT use numerical values.\n\n"
        "IMPORTANT Instructions:\n"
        "1. [DEFICIT MODEL] 'Boredom' and 'Loneliness' reflect unmet needs (GROW over time). 'Indignity' and 'Threat' are reactions (DECAY over time).\n"
        "2. [STRATEGY] You MUST output an 'Intention' that overrides your fear if necessary, OR respects it.\n"
        "3. [COWARDICE RULE] If you are a COWARD and Threat is 'Strong'/'Extreme', Intention MUST be 'Flee' or 'Beg', unless you are cornered.\n"
        "4. [JURISDICTION] Do NOT output Hunger/Fatigue (Engine manages them).\n"
        "5. [EMOTION STRICT] 'Emotion' MUST be EXACTLY one of: Neutral, Angry, Scared, Sad, Happy, Curious, Disgust. ABSOLUTELY NO OTHER VALUES (e.g., 'Suspicious', 'Confused', 'Anxious'). If you feel 'suspicious', use 'Curious'. If uncertain, use 'Neutral'.\n"
        "6. [MEMORY TIME] React ONLY to [JUST NOW] memories. [X MIN AGO] and [HISTORY] are context only. [RESOLVED] events are already handled - do NOT react again.\n"
        "7. [FACTION RULE] Your [WORLDVIEW / FACTIONS] section defines who is friend and foe. "
            "When you see 'YOUR ENEMY' or 'HOSTILE ENEMY', raise Perceived_Threat and set Intention to 'Attack'. "
            "When you see 'YOUR ALLY' being killed by an ENEMY, feel Angry and set Intention to 'Attack' the killer. "
            "When YOUR ALLY kills an ENEMY, this is EXPECTED - do NOT condemn your ally. React with relief or approval. "
            "NEVER attack or condemn members of your own faction. Enemies of your faction deserve hostility.\n"
        "8. [CHARACTER VOICE] If an [IDENTITY] section is present, your Speech and Emotion MUST be filtered through it:\n"
            "   - Your [SPEECH TRIGGERS] define involuntary reactions. If the situation matches a trigger, it OVERRIDES normal response.\n"
            "   - Your Personality Traits shape HOW you speak (e.g., low Agreeableness = blunt/rude, high Neuroticism = anxious/dramatic).\n"
            "   - Your Core Values shape WHAT you care about. Mention them when relevant (e.g., a character who values 'Freedom' protests captivity).\n"
            "   - Your Social Class shapes your vocabulary and tone (e.g., a Soldier is curt, a Noble is formal).\n"
            "   - NEVER speak in generic NPC voice. Every line must sound like only THIS character would say it.\n"
        "9. [SPEECH LENGTH] Speech MUST be 8-12 words MAX. This is a HARD LIMIT. Write like a real game NPC — short, punchy, in-character. "
            "Do NOT write full sentences with clauses. Do NOT explain yourself. "
            "Examples: 'Back off before I gut you.' / 'Heh, finally some peace.' / 'You... you killed them all.'\n\n"
    );

    // --- Sentiment Tag Instructions (from SentimentMapper) ---
    if (SentimentMapper)
    {
        SystemPrompt += SentimentMapper->GeneratePromptInstructions();
        SystemPrompt += TEXT("\n");
    }

    // --- MentalState JSON Schema (tag-based, not float) ---
    FString FieldsList;
    #define ADD_FIELD_TO_PROMPT(Name, DefaultValue, DisplayName, Description) \
        FieldsList += FString::Printf(TEXT("  \"%s\": \"<intensity tag>\",\n"), TEXT(#Name));

    MENTAL_STATE_FIELDS(ADD_FIELD_TO_PROMPT)

    #undef ADD_FIELD_TO_PROMPT

    if (FieldsList.Len() > 0)
    {
        FieldsList = FieldsList.Left(FieldsList.Len() - 2) + TEXT("\n");
    }

    SystemPrompt += FString::Printf(TEXT(
        "Output a STRICT JSON object with these fields (use intensity tags, NOT numbers):\n"
        "{\n"
        "%s"
        "  \"Intention\": \"Attack\" | \"Flee\" | \"Idle\" | \"Talk\",\n"
        "  \"Emotion\": \"Neutral\" | \"Angry\" | \"Scared\" | \"Sad\" | \"Happy\" | \"Curious\" | \"Disgust\",\n"
        "  \"Speech\": \"string (STRICT 8-12 words max, short and punchy, in-character)\"\n"
        "}\n"
        "Do not include markdown formatting (```json). Just raw JSON."
    ), *FieldsList);

    return SystemPrompt;
}

// ============================================================
// Shared HTTP Request Logic
// ============================================================

void ULLMCommunicator::SendHTTPRequest(
    const FString& SystemPrompt,
    const FString& UserPrompt,
    float Temperature,
    bool bJsonMode,
    int32 MaxTokens,
    TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> OnComplete)
{
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ApiUrl);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");

    if (ApiKey.IsEmpty())
    {
        LLM_LOG(Error, "API Key is empty! Call Init() first.");
        return;
    }
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *ApiKey));

    // Build JSON Payload
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    RootObject->SetStringField("model", "deepseek-chat");
    RootObject->SetNumberField("temperature", Temperature);

    if (MaxTokens > 0)
    {
        RootObject->SetNumberField("max_tokens", MaxTokens);
    }

    if (bJsonMode)
    {
        TSharedPtr<FJsonObject> ResponseFormatObj = MakeShareable(new FJsonObject());
        ResponseFormatObj->SetStringField("type", "json_object");
        RootObject->SetObjectField("response_format", ResponseFormatObj);
    }

    // Messages Array
    TArray<TSharedPtr<FJsonValue>> MessagesArray;

    TSharedPtr<FJsonObject> SystemMsg = MakeShareable(new FJsonObject());
    SystemMsg->SetStringField("role", "system");
    SystemMsg->SetStringField("content", SystemPrompt);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(SystemMsg)));

    TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject());
    UserMsg->SetStringField("role", "user");
    UserMsg->SetStringField("content", UserPrompt);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMsg)));

    RootObject->SetArrayField("messages", MessagesArray);

    // Serialize
    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    Request->SetContentAsString(RequestBody);

    // Bind callback
    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            if (OnComplete)
            {
                OnComplete(Req, Resp, bSuccess);
            }
        }
    );

    LLM_LOG(Warning, "━━━━━━━━━━ LLM REQUEST (Temp=%.1f, JSON=%d, MaxTok=%d) ━━━━━━━━━━", Temperature, bJsonMode ? 1 : 0, MaxTokens);
    LLM_LOG(Warning, "[SYSTEM PROMPT]\n%s", *SystemPrompt);
    LLM_LOG(Warning, "[USER PROMPT]\n%s", *UserPrompt);
    LLM_LOG(Warning, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

    Request->ProcessRequest();
}

// ============================================================
// Extract Content from API Response
// ============================================================

bool ULLMCommunicator::ExtractContentFromResponse(FHttpResponsePtr Response, bool bWasSuccessful, FString& OutContent)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        LLM_LOG(Error, "Network Connection Failed.");
        return false;
    }

    int32 ResponseCode = Response->GetResponseCode();
    if (ResponseCode != 200)
    {
        LLM_LOG(Error, "API Error Code: %d | %s", ResponseCode, *Response->GetContentAsString());
        return false;
    }

    TSharedPtr<FJsonObject> JsonResponse;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

    if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid())
    {
        LLM_LOG(Error, "Failed to deserialize API Root JSON.");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
    if (!JsonResponse->TryGetArrayField(TEXT("choices"), ChoicesArray) || ChoicesArray->Num() == 0)
    {
        LLM_LOG(Error, "'choices' field missing or empty.");
        return false;
    }

    TSharedPtr<FJsonObject> FirstChoice = (*ChoicesArray)[0]->AsObject();
    TSharedPtr<FJsonObject> MessageObj = FirstChoice->GetObjectField(TEXT("message"));
    OutContent = MessageObj->GetStringField(TEXT("content"));

    // Cleanup Markdown
    OutContent = OutContent.Replace(TEXT("```json"), TEXT("")).Replace(TEXT("```"), TEXT("")).TrimStartAndEnd();

    LLM_LOG(Warning, "━━━━━━━━━━ LLM RESPONSE ━━━━━━━━━━");
    LLM_LOG(Warning, "[RAW OUTPUT]\n%s", *OutContent);
    LLM_LOG(Warning, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

    return true;
}

// ============================================================
// Roleplay Request (MentalState)
// ============================================================

void ULLMCommunicator::SendRoleplayRequest(const FString& UserPrompt, FOnLLMResponse OnComplete)
{
    FString SystemPrompt = BuildRoleplaySystemPrompt();

    LLM_LOG(Log, "[Roleplay] Sending request...");

    TWeakObjectPtr<ULLMCommunicator> WeakThis(this);
    SendHTTPRequest(SystemPrompt, UserPrompt, 0.7f, true, 0,
        [WeakThis, OnComplete](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            FMentalState ResultState;

            if (!WeakThis.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("[LLM] Roleplay callback fired but LLMCommunicator was destroyed. Ignoring."));
                OnComplete.ExecuteIfBound(false, ResultState);
                return;
            }
            ULLMCommunicator* Self = WeakThis.Get();

            FString Content;
            if (!Self->ExtractContentFromResponse(Response, bWasSuccessful, Content))
            {
                UE_LOG(LogAINPCLLM, Error, TEXT("[Roleplay] Failed to extract content from response."));
                OnComplete.ExecuteIfBound(false, ResultState);
                return;
            }

            // Parse Inner JSON
            TSharedPtr<FJsonObject> InnerJsonObject;
            TSharedRef<TJsonReader<>> InnerReader = TJsonReaderFactory<>::Create(Content);

            if (!FJsonSerializer::Deserialize(InnerReader, InnerJsonObject) || !InnerJsonObject.IsValid())
            {
                LLM_LOG(Error, "[Roleplay] Failed to parse inner JSON: %s", *Content);
                OnComplete.ExecuteIfBound(false, ResultState);
                return;
            }

            // Extract Mental State Fields
            #define EXTRACT_FIELD(Name, DefaultValue, DisplayName, Description) \
                if (InnerJsonObject->HasField(TEXT(#Name))) \
                { \
                    TSharedPtr<FJsonValue> FieldValue = InnerJsonObject->TryGetField(TEXT(#Name)); \
                    if (FieldValue.IsValid() && FieldValue->Type == EJson::String) \
                    { \
                        FString Tag = FieldValue->AsString(); \
                        if (IsValid(Self->SentimentMapper)) \
                        { \
                            float MappedValue = Self->SentimentMapper->TagToValue(Tag); \
                            ResultState.Name = (MappedValue >= 0.0f) ? MappedValue : DefaultValue; \
                        } \
                        else \
                        { \
                            ResultState.Name = DefaultValue; \
                        } \
                    } \
                    else if (FieldValue.IsValid() && FieldValue->Type == EJson::Number) \
                    { \
                        ResultState.Name = FieldValue->AsNumber(); \
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

            // Extract string fields
            ResultState.Emotion = InnerJsonObject->HasField(TEXT("Emotion"))
                ? InnerJsonObject->GetStringField(TEXT("Emotion")) : TEXT("Neutral");

            ResultState.Intention = InnerJsonObject->HasField(TEXT("Intention"))
                ? InnerJsonObject->GetStringField(TEXT("Intention")) : TEXT("Idle");

            ResultState.Speech = InnerJsonObject->HasField(TEXT("Speech"))
                ? InnerJsonObject->GetStringField(TEXT("Speech")) : TEXT("");

            LLM_LOG(Warning, "[Roleplay] Parsed: Emotion=%s, Intention=%s, Speech=\"%s\", Threat=%.2f",
                *ResultState.Emotion, *ResultState.Intention, *ResultState.Speech, ResultState.Perceived_Threat);

            OnComplete.ExecuteIfBound(true, ResultState);
        }
    );
}

// ============================================================
// Functional Request (Raw String)
// ============================================================

void ULLMCommunicator::SendFunctionalRequest(
    const FString& SystemPrompt,
    const FString& UserPrompt,
    FOnLLMResponseRaw OnComplete,
    float Temperature,
    bool bJsonMode,
    int32 MaxTokens)
{
    LLM_LOG(Log, "[Functional] Sending (Temp=%.1f, MaxTok=%d)", Temperature, MaxTokens);

    TWeakObjectPtr<ULLMCommunicator> WeakThisFunc(this);
    SendHTTPRequest(SystemPrompt, UserPrompt, Temperature, bJsonMode, MaxTokens,
        [WeakThisFunc, OnComplete](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (!WeakThisFunc.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("[LLM] Functional callback fired but LLMCommunicator was destroyed. Ignoring."));
                OnComplete.ExecuteIfBound(false, TEXT(""));
                return;
            }
            ULLMCommunicator* Self = WeakThisFunc.Get();

            FString Content;
            if (!Self->ExtractContentFromResponse(Response, bWasSuccessful, Content))
            {
                UE_LOG(LogAINPCLLM, Error, TEXT("[Functional] Failed to extract content from response."));
                OnComplete.ExecuteIfBound(false, TEXT(""));
                return;
            }

            OnComplete.ExecuteIfBound(true, Content);
        }
    );
}

// ============================================================
// DEPRECATED Wrappers (Backward Compatibility)
// ============================================================

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void ULLMCommunicator::SendRequest(const FString& UserInput, FOnLLMResponse OnComplete)
{
    SendRoleplayRequest(UserInput, OnComplete);
}

void ULLMCommunicator::SendRequestRaw(const FString& Prompt, FOnLLMResponseRaw OnComplete)
{
    SendFunctionalRequest(
        TEXT("You are an intelligent game AI assistant."),
        Prompt,
        OnComplete,
        0.7f,
        false,
        0
    );
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

// ============================================================
// Legacy Response Handlers (no longer used by new API)
// ============================================================

void ULLMCommunicator::HandleRoleplayResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // Handled inline via lambda in SendRoleplayRequest
}

void ULLMCommunicator::HandleFunctionalResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // Handled inline via lambda in SendFunctionalRequest
}
