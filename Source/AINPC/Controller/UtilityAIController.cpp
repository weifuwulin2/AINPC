// Fill out your copyright notice in the Description page of Project Settings.
#include "Controller/UtilityAIController.h"
#include "JsonObjectConverter.h"
#include "Base/UtilityActionBase.h"
#include "UtilityAI/UNPCMentalState.h"


AUtilityAIController::AUtilityAIController()
    : MentalState(nullptr), CurrentAction(nullptr)
{
    PrimaryActorTick.bCanEverTick = true;
}

void AUtilityAIController::BeginPlay()
{
    Super::BeginPlay();
    
    // 1. 创建大脑数据对象 (这是 Action 真正读取的地方)
    MentalState = NewObject<UNPCMentalState>(this);
    
    if (MentalState)
    {
        // 给个初始值，防止一开始全是0
        MentalState->Anger = 0.0f;
        MentalState->Fear = 0.0f;
    }

    // 3. 初始化 LLM 服务
    LLMService = NewObject<ULLMCommunicator>(this);
    // 1. 定义变量来接收读取的结果
    FString ConfigApiKey;
    FString ConfigApiUrl;
    
    // 2. 从 DefaultGame.ini 读取
    // 参数说明: (Section名, Key名, 接收变量, 配置文件名宏)
    GConfig->GetString(TEXT("LLM.Settings"), TEXT("ApiKey"), ConfigApiKey, GGameIni);
    GConfig->GetString(TEXT("LLM.Settings"), TEXT("ApiUrl"), ConfigApiUrl, GGameIni);

    // 3. 安全检查：如果没读到，给个报错或者默认值
    if (ConfigApiKey.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FATAL: API Key not found in DefaultGame.ini!"));
        return;
    }

    // 如果配置文件里没填 URL，就用默认的
    if (ConfigApiUrl.IsEmpty())
    {
        ConfigApiUrl = TEXT("https://api.deepseek.com/chat/completions");
    }

    // 4. 传给 LLMService
    LLMService->Init(ConfigApiKey, ConfigApiUrl);
    
    UE_LOG(LogTemp, Log, TEXT("LLM Service Initialized via Config."));

    // === 从 DataTable 加载 Actions ===
    if (ActionDataTable)
    {
        static const FString ContextString(TEXT("UtilityAI Actions Context"));
        TArray<FUtilityActionConfig*> Rows;
        ActionDataTable->GetAllRows(ContextString, Rows);

        for (FUtilityActionConfig* Row : Rows)
        {
            if (Row && Row->ActionClass)
            {
                // 1. 创建 Action 实例
                UUtilityActionBase* NewAction = NewObject<UUtilityActionBase>(this, Row->ActionClass);
                
                // 2. 注入表格里的配置 (曲线、权重、输入类型)
                NewAction->InitFromConfig(*Row);
                
                // 3. 加入列表
                AvailableActions.Add(NewAction);

                UE_LOG(LogTemp, Log, TEXT("Loaded Action: %s | Driven By: %d | Weight: %.2f"), 
                    *NewAction->ActionName, (int)Row->InputType, Row->Weight);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ActionDataTable is MISSING! Please assign it in Blueprint."));
    }
}

void AUtilityAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 优化建议：不要每帧都去计算分数(EvaluateUtilityLogic)，非常耗性能。
    // 分数只需要在“大脑更新”或“每0.5秒”算一次即可。
    // Tick 里应该只负责执行当前动作。
    
    if (CurrentAction)
    {
        CurrentAction->Execute(this); // 持续执行动作 (比如移动)
    }
}

// 这个函数负责“做决策”，不需要每帧调用，只需要在需要变的时候调用
void AUtilityAIController::EvaluateUtilityLogic()
{
    if (!MentalState || AvailableActions.Num() == 0) return;

    UUtilityActionBase* BestAction = nullptr;
    float BestScore = -1.0f;

    // --- 1. 遍历打分 ---
    for (UUtilityActionBase* Action : AvailableActions)
    {
        // Action 读的是 MentalState (UObject)
        float Score = Action->CalculateScore(MentalState);
        
        // 惯性分：防止 AI 在两个分数相近的动作间鬼畜切换
        if (Action == CurrentAction)
        {
            Score += 0.1f; 
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestAction = Action;
        }
    }

    // --- 2. 只有当选出了新动作，才切换 ---
    if (BestAction && BestAction != CurrentAction)
    {
        // 退出旧动作
        if (CurrentAction) 
        {
            CurrentAction->Exit(this);
        }

        // 切换新动作
        CurrentAction = BestAction;
        // Enter 通常只做一次初始化，Execute 放在 Tick 里持续跑
        CurrentAction->Enter(this); 
        
        UE_LOG(LogTemp, Warning, TEXT("DECISION: Switching to %s (Score: %.2f)"), *CurrentAction->ActionName, BestScore);
    }
}

void AUtilityAIController::UpdateMind(FString SituationDescription)
{
    if (LLMService)
    {
        UE_LOG(LogTemp, Log, TEXT("[Mind] Thinking about: %s"), *SituationDescription);
        LLMService->SendRequest(
            SituationDescription,
            FOnLLMResponse::CreateUObject(this, &AUtilityAIController::OnMindUpdated)
        );
    }
}

// 4. 关键修复：数据同步
void AUtilityAIController::OnMindUpdated(bool bSuccess, const FMentalState& NewState)
{
    if (bSuccess && MentalState)
    {
        // === 核心修复 ===
        // 将 LLM 返回的 Struct 数据，拷贝给 Action 读取的 UObject
        MentalState->Anger = NewState.Anger;
        MentalState->Fear = NewState.Fear;
        MentalState->Confidence = NewState.Confidence;
        MentalState->SocialBattery = NewState.SocialBattery;
        MentalState->Hunger = NewState.Hunger;

        UE_LOG(LogTemp, Warning, TEXT(">>> Mind Sync Complete! Anger: %.2f, Fear: %.2f <<<"), 
            MentalState->Anger, MentalState->Fear);

        // 数据变了，立刻重新思考！
        EvaluateUtilityLogic(); 
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Mind] Update Failed."));
    }
}