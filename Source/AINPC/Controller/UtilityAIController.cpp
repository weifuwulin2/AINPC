#include "UtilityAIController.h"

// 引入所有需要的组件和配置头文件
#include "Components/SensoryComponent.h"
#include "Components/CognitionComponent.h"
#include "Components/UtilityAIComponent.h"
#include "UtilityAI/UNPCMentalState.h" 

// 感知相关头文件
#include "LLM/LLMCommunicator.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"

AUtilityAIController::AUtilityAIController()
{
    // =========================================================
    // 1. 创建核心逻辑组件
    // =========================================================
    SensoryComp = CreateDefaultSubobject<USensoryComponent>(TEXT("SensoryComponent"));
    CognitionComp = CreateDefaultSubobject<UCognitionComponent>(TEXT("CognitionComponent"));
    UtilityComp = CreateDefaultSubobject<UUtilityAIComponent>(TEXT("UtilityComponent"));

    // =========================================================
    // 2. 创建并配置感知组件 (Perception)
    // =========================================================
    
    // 创建 AIPerception
    AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
    SetPerceptionComponent(*AIPerception); // 告诉 Controller 使用这个组件作为感知源

    // --- 配置视觉 (Sight) ---
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    if (SightConfig)
    {
        SightConfig->SightRadius = 1500.0f;           // 看多远 (15米)
        SightConfig->LoseSightRadius = 1800.0f;       // 离开多远才看不见
        SightConfig->PeripheralVisionAngleDegrees = 90.0f; // 视野角度 (前方180度)
        
        // 记忆时间：在这个时间内即使被遮挡也认为还在那里
        SightConfig->SetMaxAge(5.0f); 

        // 极其重要：必须配置这三个布尔值，否则 GameMode 如果不设置队伍关系，可能什么都看不见
        SightConfig->DetectionByAffiliation.bDetectEnemies = true;
        SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
        SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

        // 注册视觉
        AIPerception->ConfigureSense(*SightConfig);
    }

    // --- 配置听觉 (Hearing) ---
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    if (HearingConfig)
    {
        HearingConfig->HearingRange = 2500.0f; // 听多远 (25米)
        
        // 同样需要配置阵营检测
        HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
        HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
        HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

        // 注册听觉
        AIPerception->ConfigureSense(*HearingConfig);
    }

    // 设置视觉为主感官
    if (SightConfig)
    {
        AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
    }
}

void AUtilityAIController::BeginPlay()
{
    Super::BeginPlay();

    // =========================================================
    // 3. 初始化共享数据 (MentalState)
    // =========================================================
    MentalState = NewObject<UNPCMentalState>(this);
    // ✅ 不需要手动设置默认值，构造函数已经使用宏自动初始化所有字段
    // 所有字段的默认值在 MentalStateFields.h 中统一配置

    // =========================================================
    // 4. 神经接驳 (Wiring everything together)
    // =========================================================

    // --- 连接 A: 感知 -> 翻译 ---
    // 初始化 SensoryComponent，让它接管 AIPerception 的事件
    if (SensoryComp && AIPerception)
    {
        SensoryComp->InitializeSensorySystem(AIPerception);
        
        // 当 Sensory 翻译好信号后，调用 RelaySensoryToCognition
        SensoryComp->OnStimulusProduced.AddDynamic(this, &AUtilityAIController::RelaySensoryToCognition);
    }

    // --- 连接 B: 认知 -> 数据存储 ---
    // 当 Cognition 算完情绪后，调用 OnMindUpdated 同步数据
    if (CognitionComp)
    {
        // 假设 CognitionComp 有这个委托
        CognitionComp->OnMentalStateChanged.AddDynamic(this, &AUtilityAIController::OnMindUpdated);
    }

    // =========================================================
    // 5. 启动 Dreaming 定时器 (Memory Consolidation)
    // =========================================================
    
    // 每5分钟触发一次记忆整理，将短期记忆转化为长期洞察
    if (CognitionComp)
    {
        GetWorldTimerManager().SetTimer(
            DreamingTimerHandle,
            [this]() 
            { 
                if (CognitionComp)
                {
                    CognitionComp->StartDreaming();
                    UE_LOG(LogTemp, Log, TEXT("[%s] Dreaming cycle triggered - consolidating memories..."), 
                           *GetName());
                }
            },
            300.0f,  // 每5分钟 (300秒)
            true     // 循环执行
        );
        
        UE_LOG(LogTemp, Log, TEXT("[%s] Dreaming timer initialized (interval: 5 minutes)"), *GetName());
    }
}

// =========================================================
// 5. 运行时逻辑 (Runtime Logic)
// =========================================================

void AUtilityAIController::ReceiveSpeech(AActor* Speaker, FString Message)
{
    // 直接转发给感官翻译组件
    if (SensoryComp)
    {
        SensoryComp->ReceiveSpeech(Speaker, Message);
    }
}

void AUtilityAIController::RelaySensoryToCognition(const FString& StimulusDescription)
{
    // 中转：把翻译好的话传给大脑
    if (CognitionComp)
    {
        // 这里的 Log 可以帮你调试“到底有没有把信号传进去”
        // UE_LOG(LogTemp, Log, TEXT("[Controller] Relaying Stimulus: %s"), *StimulusDescription);
        
        CognitionComp->ProcessStimulus(StimulusDescription);
    }
}

void AUtilityAIController::OnMindUpdated(const FMentalState& NewState)
{
    // ✅ 使用转换函数，简化代码
    if (MentalState)
    {
        MentalState->UpdateFromStruct(NewState);
        
        // 可选：打印日志
        UE_LOG(LogTemp, Log, TEXT("[Controller] Mental State Updated: Anger=%.2f, Fear=%.2f"), 
               MentalState->Anger, MentalState->Fear);
        
        // UtilityComponent 不需要通知，它会在自己的 Tick 里自动读这个 MentalState
    }
}