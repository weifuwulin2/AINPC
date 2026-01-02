// LLMCommunicator.cpp

#include "LLMCommunicator.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Policies/CondensedJsonPrintPolicy.h"

void ULLMCommunicator::Init(const FString& InApiKey, const FString& InUrl)
{
    ApiKey = InApiKey;
    ApiUrl = InUrl;
}

void ULLMCommunicator::SendRequest(const FString& UserInput, FOnLLMResponse OnComplete)
{
    // 1. Store the callback to execute later
    CurrentCallback = OnComplete;

    // 2. Create the HTTP Request
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ApiUrl);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    
    // Safety check for API Key
    if (ApiKey.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] API Key is empty! Call Init() first."));
        FMentalState EmptyState;
        CurrentCallback.ExecuteIfBound(false, EmptyState);
        return;
    }
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *ApiKey));

    // 3. Construct the JSON Payload
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
    // We strictly define the fields here to match FMentalState struct
    FString SystemPrompt = TEXT(
        "You are an AI game engine. Analyze the input and output a STRICT JSON object.\n"
        "Return specific float values (0.0 to 1.0) for these fields:\n"
        "{\n"
        "  \"Anger\": float,\n"
        "  \"Fear\": float,\n"
        "  \"Confidence\": float,\n"
        "  \"SocialBattery\": float,\n"
        "  \"Hunger\": float\n"
        "}\n"
        "Do not include markdown formatting (```json). Just raw JSON."
    );
    SystemMsg->SetStringField("content", SystemPrompt);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(SystemMsg)));

    // --- USER MESSAGE (The Input) ---
    TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject());
    UserMsg->SetStringField("role", "user");
    UserMsg->SetStringField("content", UserInput);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMsg)));

    RootObject->SetArrayField("messages", MessagesArray);

    // 4. Serialize to String
    FString RequestBody;
    // TJsonWriterFactory is used to safely write to FString
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    Request->SetContentAsString(RequestBody);

    // 5. Bind Callback & Send
    Request->OnProcessRequestComplete().BindUObject(this, &ULLMCommunicator::OnResponseReceived);
    Request->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("[LLM] Request Sent: %s"), *UserInput);
}

void ULLMCommunicator::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FMentalState ResultState; // Default initialized to 0.0f

    // --- Network & Protocol Validation ---
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] Network Connection Failed."));
        CurrentCallback.ExecuteIfBound(false, ResultState);
        return;
    }

    if (Response->GetResponseCode() != 200)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] API Error Code: %d. Response: %s"), Response->GetResponseCode(), *Response->GetContentAsString());
        CurrentCallback.ExecuteIfBound(false, ResultState);
        return;
    }

    // --- Outer JSON Parsing (The API Envelope) ---
    FString ResponseString = Response->GetContentAsString();
    TSharedPtr<FJsonObject> JsonResponse;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

    if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] Failed to deserialize API Root JSON."));
        CurrentCallback.ExecuteIfBound(false, ResultState);
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

        // --- Inner JSON Parsing (The Actual Data) ---
        // Convert the string content directly into the UE Struct
        if (FJsonObjectConverter::JsonObjectStringToUStruct(InnerContentString, &ResultState, 0, 0))
        {
            // SUCCESS!
            UE_LOG(LogTemp, Log, TEXT("[LLM] Success! Parsed: Anger=%.2f, Fear=%.2f"), ResultState.Anger, ResultState.Fear);
            CurrentCallback.ExecuteIfBound(true, ResultState);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[LLM] Struct Conversion Failed. Content was: %s"), *InnerContentString);
            CurrentCallback.ExecuteIfBound(false, ResultState);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] 'choices' field missing or empty in response."));
        CurrentCallback.ExecuteIfBound(false, ResultState);
    }
}