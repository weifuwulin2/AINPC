#include "Controller/UtilityAIController.h"
#include "JsonObjectConverter.h" // 如果还需要手动处理JSON才引用，否则可删
#include "Base/UtilityActionBase.h"
#include "Components/CognitionComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "UtilityAI/UNPCMentalState.h"

AUtilityAIController::AUtilityAIController()
    : MentalState(nullptr), CurrentAction(nullptr)
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. 创建认知组件 (标准组件创建方式)
    CognitionComp = CreateDefaultSubobject<UCognitionComponent>(TEXT("CognitionComponent"));
}

void AUtilityAIController::BeginPlay()
{
    Super::BeginPlay();
    
    // 2. 初始化数据对象
    MentalState = NewObject<UNPCMentalState>(this);
    if (MentalState)
    {
        MentalState->Anger = 0.0f;
        MentalState->Fear = 0.0f;
    }

    // 1. 初始化感知组件
    AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    CognitionComp = CreateDefaultSubobject<UCognitionComponent>(TEXT("Cognition"));

    // 配置视力参数
    SightConfig->SightRadius = 1000.0f;       // 看多远
    SightConfig->LoseSightRadius = 1200.0f;   // 离开多远看不见
    SightConfig->PeripheralVisionAngleDegrees = 90.0f; // 视野角度
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

    AIPerception->ConfigureSense(*SightConfig);
    AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());

    // 3. 绑定组件事件
    // 当认知组件想明白时，调用我的 OnMindUpdated
    if (CognitionComp)
    {
        CognitionComp->OnMentalStateChanged.AddDynamic(this, &AUtilityAIController::OnMindUpdated);
    }

    // 4. 加载 Actions (保持原样)
    if (ActionDataTable)
    {
        static const FString ContextString(TEXT("UtilityAI Actions Context"));
        TArray<FUtilityActionConfig*> Rows;
        ActionDataTable->GetAllRows(ContextString, Rows);

        for (FUtilityActionConfig* Row : Rows)
        {
            if (Row && Row->ActionClass)
            {
                UUtilityActionBase* NewAction = NewObject<UUtilityActionBase>(this, Row->ActionClass);
                NewAction->InitFromConfig(*Row);
                AvailableActions.Add(NewAction);

                UE_LOG(LogTemp, Log, TEXT("Loaded Action: %s | Weight: %.2f"), *NewAction->ActionName, Row->Weight);
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
    
    // 只负责执行当前动作
    if (CurrentAction)
    {
        CurrentAction->Execute(this);
    }
}

// 这依然是你的对外接口，但现在它只是个中间人
void AUtilityAIController::UpdateMind(FString SituationDescription)
{
    if (CognitionComp)
    {
        // 委托给组件去处理
        CognitionComp->ProcessStimulus(SituationDescription);
    }
}

// 接收组件传回来的数据
void AUtilityAIController::OnMindUpdated(const FMentalState& NewState)
{
    if (MentalState)
    {
        // 1. 数据同步
        MentalState->Anger = NewState.Anger;
        MentalState->Fear = NewState.Fear;
        MentalState->Confidence = NewState.Confidence;
        MentalState->SocialBattery = NewState.SocialBattery;
        MentalState->Hunger = NewState.Hunger;

        UE_LOG(LogTemp, Warning, TEXT(">>> Controller Sync: Anger: %.2f, Fear: %.2f <<<"), 
            MentalState->Anger, MentalState->Fear);

        // 2. 数据变了，重新跑分
        EvaluateUtilityLogic(); 
    }
}

void AUtilityAIController::EvaluateUtilityLogic()
{
    if (!MentalState || AvailableActions.Num() == 0) return;

    UUtilityActionBase* BestAction = nullptr;
    float BestScore = -1.0f;

    // --- 遍历打分 ---
    for (UUtilityActionBase* Action : AvailableActions)
    {
        float Score = Action->CalculateScore(MentalState);
        
        // 惯性分
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

    // --- 切换动作 ---
    if (BestAction && BestAction != CurrentAction)
    {
        if (CurrentAction) CurrentAction->Exit(this);

        CurrentAction = BestAction;
        CurrentAction->Enter(this); 
        
        UE_LOG(LogTemp, Warning, TEXT("DECISION: Switching to %s (Score: %.2f)"), *CurrentAction->ActionName, BestScore);

        // === 告诉大脑我正在做什么 ===
        if (CognitionComp)
        {
            FString ActionDesc = FString::Printf(TEXT("I decided to perform action: %s"), *CurrentAction->ActionName);
        
            // 注意：这里可能只 AddMemory 但不一定非要立刻 ProcessStimulus(发请求给LLM)
            // 为了省钱，我们可以给 CognitionComp 加一个 "AddMemoryOnly" 的函数
            // 或者就在 ProcessStimulus 里加个 bool bTriggerLLM = true
            CognitionComp->ProcessStimulus(ActionDesc); 
        }
    }
}