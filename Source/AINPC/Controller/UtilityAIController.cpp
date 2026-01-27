#include "UtilityAIController.h"

// 引入所有需要的组件和配置头文件
#include "Components/SensoryComponent.h"
#include "Components/CognitionComponent.h"
#include "Components/UtilityAIComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/EmotionDisplayComponent.h"
#include "Components/MetabolismComponent.h"
#include "Components/GoalComponent.h"
#include "Components/FactionReputationComponent.h"
#include "UtilityAI/UNPCMentalState.h" 
#include "UtilityAI/EmotionEvaluator.h" // Add this 
#include "Components/NPCDefinitionComponent.h" // Add this
#include "Components/MonsterComponent.h" // Add this

// 感知相关头文件
#include "AINPC.h"
#include "Engine/DamageEvents.h"
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
    MemoryComp = CreateDefaultSubobject<UMemoryComponent>(TEXT("MemoryComponent")); // The Brain
    CognitionComp = CreateDefaultSubobject<UCognitionComponent>(TEXT("CognitionComponent"));
    UtilityComp = CreateDefaultSubobject<UUtilityAIComponent>(TEXT("UtilityComponent"));
    PersonalityComp = CreateDefaultSubobject<UPersonalityComponent>(TEXT("PersonalityComponent"));
    EmotionDisplayComp = CreateDefaultSubobject<UEmotionDisplayComponent>(TEXT("EmotionDisplayComponent"));
    MetabolismComp = CreateDefaultSubobject<UMetabolismComponent>(TEXT("MetabolismComponent"));
    GoalComp = CreateDefaultSubobject<UGoalComponent>(TEXT("GoalComponent"));

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

void AUtilityAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    AINPC_LOG(Warning, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    AINPC_LOG(Warning, "OnPossess called for Pawn: %s", InPawn ? *InPawn->GetName() : TEXT("NULL"));
    
    if (!InPawn) return;

    // ✅ Initialization Arbitration Logic
    // 1. Check for MonsterComponent (Highest Priority - Overrides everything)
    UMonsterComponent* MonsterComp = InPawn->FindComponentByClass<UMonsterComponent>();
    AINPC_LOG(Warning, "MonsterComponent Found: %s", MonsterComp ? TEXT("YES") : TEXT("NO"));
    
    if (MonsterComp)
    {
        AINPC_LOG(Warning, "✅ Applying Monster Settings (Brainless).");
        
        // Apply Monster Settings Manually (since MonsterComponent::BeginPlay might have failed due to race condition)
        
        // A. Force Faction
        bool bFoundFactionComponent = false;
        if (UFactionReputationComponent* FacComp = InPawn->FindComponentByClass<UFactionReputationComponent>())
        {
            FacComp->CurrentFactionID = MonsterComp->MonsterFactionID;
            AINPC_LOG(Warning, "Set FactionReputationComponent: %s", *MonsterComp->MonsterFactionID.ToString());
            bFoundFactionComponent = true;
        }
        else if (UNPCDefinitionComponent* DefComp = InPawn->FindComponentByClass<UNPCDefinitionComponent>())
        {
            DefComp->FactionID = MonsterComp->MonsterFactionID;
            AINPC_LOG(Warning, "Set NPCDefinition FactionID: %s", *MonsterComp->MonsterFactionID.ToString());
            bFoundFactionComponent = true;
        }
        
        if (!bFoundFactionComponent)
        {
            AINPC_LOG_ERROR("❌ No FactionReputationComponent or NPCDefinitionComponent found!");
        }

        // B. Force Personality
        if (PersonalityComp)
        {
            PersonalityComp->Personality.RoleDescription = TEXT("[INSTINCTS] You are a MINDLESS MONSTER. Driven purely by hunger. No logic, no mercy. Attack on sight.");
            PersonalityComp->Personality.BehavioralGuidelines = TEXT("Attack any non-Monster instantly. Never retreat. Make guttural noises.");
            
            // Feral OCEAN Stats
            PersonalityComp->Personality.Openness = 0.1f;
            PersonalityComp->Personality.Conscientiousness = 0.1f;
            PersonalityComp->Personality.Extraversion = 0.1f;
            PersonalityComp->Personality.Agreeableness = 0.0f;
            PersonalityComp->Personality.Neuroticism = 0.1f;
            
            AINPC_LOG(Warning, "✅ Set Feral Personality on PersonalityComponent");
        }
        else
        {
            AINPC_LOG_ERROR("❌ PersonalityComp is NULL!");
        }

        // C. Disable Reasoning
        if (CognitionComp)
        {
            // CognitionComp->bEnableReasoning = false; // Re-enabled to allow LLM to generate 'grunts' and 'hisses'
            AINPC_LOG(Warning, "✅ Cognition Reasoning stays ENABLED for Monster (to allow speech generation). Amygdala Hijack handles threat.");
        }
        
        AINPC_LOG(Warning, "Monster initialization complete. Skipping NPCDefinition.");
        AINPC_LOG(Warning, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        return; // Skip NPCDefinition
    }

    // 2. Check for NPCDefinitionComponent (Standard Initialization)
    if (UNPCDefinitionComponent* DefComp = InPawn->FindComponentByClass<UNPCDefinitionComponent>())
    {
        AINPC_LOG(Log, "Found NPCDefinitionComponent. Applying Profile.");
        DefComp->ApplyDefinition(this);
    }
    else
    {
        AINPC_LOG(Warning, "⚠️ No MonsterComponent or NPCDefinitionComponent found!");
    }
    
    AINPC_LOG(Warning, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}

void AUtilityAIController::BeginPlay()
{
    Super::BeginPlay();

    // =========================================================
    // 3. 初始化共享数据 (MentalState)
    // =========================================================
    MentalState = NewObject<UNPCMentalState>(this);

    // =========================================================
    // 4. 神经接驳 (Wiring everything together)
    // =========================================================

    // --- 连接 A: 感知 -> 翻译 ---
    // 初始化 SensoryComponent，让它接管 AIPerception 的事件
    if (SensoryComp && AIPerception)
    {
        SensoryComp->InitializeSensorySystem(AIPerception);

        // ✅ Single Event Path: Semantic Events -> Memory + Cognition
        SensoryComp->OnSemanticEventSensed.AddDynamic(this, &AUtilityAIController::OnSemanticEventReceived);
    }

    // --- 连接 B: 认知 -> 数据存储 ---
    // 当 Cognition 算完情绪后，调用 OnMindUpdated 同步数据
    if (CognitionComp)
    {
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

    // =========================================================
    // 6. 配置 Metabolism (根据 Faction 决定是否启用)
    // Configure Metabolism (Enable/Disable based on Faction)
    // =========================================================
    if (MetabolismComp)
    {
        // ✅ REFACTORED: Use FactionReputationComponent instead of deprecated Personality.Faction
        // 重构：使用 FactionReputationComponent 替代已弃用的 Personality.Faction
        EFactionType Faction = EFactionType::Neutral;
        
        // Try to get faction from FactionReputationComponent
        if (UFactionReputationComponent* FacComp = FindComponentByClass<UFactionReputationComponent>())
        {
            FName FactionID = FacComp->CurrentFactionID;
            if (FactionID == "Monster") Faction = EFactionType::Monster;
            else if (FactionID == "Human") Faction = EFactionType::Human;
        }
        
        if (Faction == EFactionType::Monster)
        {
            // 怪物不需要吃饭睡觉
            // Monsters don't need to eat or sleep
            MetabolismComp->SetComponentTickEnabled(false);
            UE_LOG(LogTemp, Log, TEXT("[%s] Metabolism DISABLED (Faction: Monster)"), *GetName());
        }
        else
        {
            // 人类和中立 NPC 需要新陈代谢
            // Human and Neutral NPCs need metabolism
            MetabolismComp->SetComponentTickEnabled(true);
            UE_LOG(LogTemp, Log, TEXT("[%s] Metabolism ENABLED (Faction: %s)"), 
                   *GetName(), 
                   *UEnum::GetValueAsString(Faction));
        }
    }

    // =========================================================
    // 7. 验证 EmotionMatrixTable 配置
    // Verify EmotionMatrixTable configuration
    // =========================================================
    if (EmotionMatrixTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] ✅ EmotionMatrixTable VALID: %s"), *GetName(), *EmotionMatrixTable->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] ❌ EmotionMatrixTable is NULL! Please assign it in the Controller Blueprint."), *GetName());
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

void AUtilityAIController::OnSemanticEventReceived(const FSemanticEvent& Event)
{
    // The Brain receives the Event
    if (MemoryComp)
    {
        MemoryComp->CommitEvent(Event);
    }

    // ✅ Trigger Cognition to process this event (replaces legacy RelaySensoryToCognition)
    if (CognitionComp)
    {
        CognitionComp->ProcessStimulus(Event.Content);
    }

    // Request debug log
    if (UtilityComp)
    {
        UtilityComp->RequestDebugLog();
    }
}



void AUtilityAIController::OnMindUpdated(const FMentalState& NewState)
{
    // ✅ 使用转换函数，简化代码
    if (MentalState)
    {
        MentalState->UpdateFromStruct(NewState);
        
        // 重新计算情绪状态
        // Recalculate Emotion State
        // 优先使用 LLM 返回的显式 emotion (如果不是 Neutral)
        // Prioritize explicit emotion from LLM (if valid and not Neutral)
        bool bUseLLMEmotion = false;
        
        // 解析 LLM 返回的 Emotion 字符串
        // Parse Emotion string returned by LLM
        FString LLMEmotionStr = NewState.Emotion;
        if (!LLMEmotionStr.IsEmpty() && !LLMEmotionStr.Equals("Neutral", ESearchCase::IgnoreCase) && !LLMEmotionStr.Equals("None", ESearchCase::IgnoreCase))
        {
             // 尝试转换字符串为枚举
             // Try to convert string to enum
             // UE expects fully qualified name: "EEmotionState::Scared"
             const UEnum* EmotionEnum = StaticEnum<EEmotionState>();
             if (EmotionEnum)
             {
                 // Prepend enum scope - UE GetValueByName expects "EEmotionState::Scared" not just "Scared"
                 FString FullEnumName = FString::Printf(TEXT("EEmotionState::%s"), *LLMEmotionStr);
                 int64 EnumValue = EmotionEnum->GetValueByName(FName(*FullEnumName));
                 if (EnumValue != INDEX_NONE)
                 {
                     CurrentEmotion = (EEmotionState)EnumValue;
                     bUseLLMEmotion = true;
                     
                     // ✅ 设置 CurrentEmotionScore 用于衰减
                     // Set CurrentEmotionScore for decay system
                     if (MentalState)
                     {
                         MentalState->CurrentEmotionScore = 1.0f;
                         MentalState->CurrentEmotion = CurrentEmotion;
                     }
                     
                     UE_LOG(LogTemp, Warning, TEXT("[Controller] ⚡ Forced Emotion from LLM: %s (Score: 1.0)"), *LLMEmotionStr);
                 }
                 else
                 {
                     UE_LOG(LogTemp, Warning, TEXT("[Controller] ⚠️ Failed to parse Emotion: '%s' (tried '%s')"), *LLMEmotionStr, *FullEnumName);
                 }
             }
        }

        // 如果 LLM 没有强制指定，则使用 EmotionEvaluator 基于数值计算
        // If LLM didn't force emotion, calculate based on stats
        if (!bUseLLMEmotion && PersonalityComp)
        {
            // 使用 EmotionEvaluator 计算当前情绪
            CurrentEmotion = UEmotionEvaluator::CalculateEmotion(MentalState, PersonalityComp->MaslowWeights);
        }
        
        // ✅ Store the emotion in MentalState for persistence
        MentalState->CurrentEmotion = CurrentEmotion;

        // 更新情绪显示
        if (EmotionDisplayComp)
        {
            // 将枚举转换为字符串 (EEmotionState::Angry -> Angry)
            FString EmotionStr = UEnum::GetValueAsString(CurrentEmotion);
            FString CleanEmotionStr;
            if (EmotionStr.Split(TEXT("::"), nullptr, &CleanEmotionStr))
            {
                EmotionDisplayComp->ShowEmotion(CleanEmotionStr);
            }
            else
            {
                EmotionDisplayComp->ShowEmotion(EmotionStr);
            }
        }
        
        // UtilityComponent 不需要通知，它会在自己的 Tick 里自动读这个 MentalState
    }
}