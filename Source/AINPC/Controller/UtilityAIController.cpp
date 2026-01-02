#include "UtilityAIController.h"
#include "Components/UtilityAIComponent.h"
#include "Components/CognitionComponent.h"
#include "LLM/LLMCommunicator.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "UtilityAI/UNPCMentalState.h"

AUtilityAIController::AUtilityAIController()
{
    // 1. 在构造函数中创建所有组件
    UtilityComp = CreateDefaultSubobject<UUtilityAIComponent>(TEXT("UtilityComponent"));
    CognitionComp = CreateDefaultSubobject<UCognitionComponent>(TEXT("CognitionComponent"));
    
    // 感知相关
    SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent")));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    
    // 配置视力
    if (SightConfig) {
        SightConfig->SightRadius = 1000.0f;
        SightConfig->LoseSightRadius = 1200.0f;
        SightConfig->PeripheralVisionAngleDegrees = 90.0f;
        SightConfig->DetectionByAffiliation.bDetectEnemies = true;
        SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
        SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
        
        GetPerceptionComponent()->ConfigureSense(*SightConfig);
        GetPerceptionComponent()->SetDominantSense(SightConfig->GetSenseImplementation());
    }
}

void AUtilityAIController::BeginPlay()
{
    Super::BeginPlay();

    // 2. 初始化数据
    MentalState = NewObject<UNPCMentalState>(this);

    // 3. 绑定事件
    if (GetPerceptionComponent())
    {
        GetPerceptionComponent()->OnTargetPerceptionUpdated.AddDynamic(this, &AUtilityAIController::OnTargetPerceived);
    }

    if (CognitionComp)
    {
        CognitionComp->OnMentalStateChanged.AddDynamic(this, &AUtilityAIController::OnMindUpdated);
    }
}

void AUtilityAIController::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (Actor && Stimulus.WasSuccessfullySensed() && CognitionComp)
    {
        // 简单描述并传给认知
        FString Desc = FString::Printf(TEXT("Saw %s"), *Actor->GetName());
        CognitionComp->ProcessStimulus(Desc);
    }
}

// AUtilityAIController.cpp

void AUtilityAIController::OnMindUpdated(const FMentalState& NewState)
{
    // 安全检查：确保我们的数据容器 (MentalState) 已经初始化
    if (MentalState)
    {
        // ====================================================
        // 1. 完整数据同步 (Data Sync)
        //    将认知组件发过来的结构体数据，一一赋值给我们持有的 UObject
        // ====================================================
        
        MentalState->Anger          = NewState.Anger;
        MentalState->Fear           = NewState.Fear;
        MentalState->Confidence     = NewState.Confidence;
        MentalState->SocialBattery  = NewState.SocialBattery;
        MentalState->Hunger         = NewState.Hunger;

        // 如果以后你加了新的属性（比如 Stamina），记得在这里也加上：
        // MentalState->Stamina = NewState.Stamina;

        // ====================================================
        // 2. 调试日志 (可选)
        //    让你在 Output Log 里清楚看到情绪变了
        // ====================================================
        UE_LOG(LogTemp, Log, TEXT("[MindSync] State Updated -> Anger: %.2f | Fear: %.2f | Hunger: %.2f"), 
            MentalState->Anger, MentalState->Fear, MentalState->Hunger);

        // ====================================================
        // 3. 关于“是否需要立即打分”的说明
        // ====================================================
        // 目前你的 UtilityAIComponent 有自己的 Tick (每 0.1 秒跑一次)。
        // 所以这里不需要手动调用 Evaluate。
        // UtilityComponent 会在下一次 Tick 时，自动读到上面更新后的 MentalState 数据。
        
        // *高级用法*：
        // 如果这是一个“紧急突发状况”（比如Fear瞬间从0飙升到1），你可能不想等那 0.1 秒。
        // 你可以在 UtilityAIComponent 里写一个 public 函数叫 ForceUpdate()，然后在这里调用它。
        /*
        if (UtilityComp && NewState.Fear > 0.9f)
        {
             UtilityComp->EvaluateAndDecide(); // 立即反应，别等 Tick 了
        }
        */
    }
}