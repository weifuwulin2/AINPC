#include "Components/MetabolismComponent.h"
#include "Controller/UtilityAIController.h"
#include "UtilityAI/UNPCMentalState.h"
#include "Components/MemoryComponent.h"
#include "Components/CognitionComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/EmotionDisplayComponent.h"
#include "UtilityAI/EmotionEvaluator.h"
#include "UtilityAI/MentalStateInterpolation.h"
#include "AINPC.h"

UMetabolismComponent::UMetabolismComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
    // 确保在 CognitionComponent 之后运行，这样我们是在修改"已插值"后的值
    PrimaryComponentTick.TickGroup = TG_PrePhysics; 
}

void UMetabolismComponent::BeginPlay()
{
	Super::BeginPlay();

    //尝试获取 Controller
    if (AActor* Owner = GetOwner())
    {
        if (AUtilityAIController* Controller = Cast<AUtilityAIController>(Owner))
        {
            CachedController = Controller;
        }
        else if (APawn* Pawn = Cast<APawn>(Owner))
        {
            CachedController = Cast<AUtilityAIController>(Pawn->GetController());
        }
    }

    if (CachedController)
    {
        AINPC_LOG(Log, "Component initialized for %s", *CachedController->GetName());
    }
    else
    {
        AINPC_LOG_WARNING("⚠️ Could not find UtilityAIController! Metabolism will not work.");
    }
}

UNPCMentalState* UMetabolismComponent::GetMentalState() const
{
    if (CachedController)
    {
        return CachedController->MentalState;
    }
    return nullptr;
}

void UMetabolismComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UNPCMentalState* State = GetMentalState();
    if (!State) return;

    // 获取 Interpolator (我们需要检查是否有来自 LLM 的活跃目标)
    UMentalStateInterpolator* Interpolator = nullptr;
    if (CachedController && CachedController->CognitionComp)
    {
        Interpolator = CachedController->CognitionComp->Interpolator;
    }

    // === 1. 生理需求 (时间流逝导致需求增加) ===
    // Physiological needs (Time passing increases needs)
    
    // 饥饿 (Hunger = 饥饿度，时间流逝增加饥饿度)
    // Hunger (Hunger = hunger level, time passing increases hunger)
    float OldHunger = State->Hunger;
    State->Hunger = FMath::Clamp(State->Hunger + (HungerRate * DeltaTime), 0.0f, 1.0f);
    
    // 疲劳 (Fatigue = 疲劳度，时间流逝增加疲劳度)
    // Fatigue (Fatigue = fatigue level, time passing increases fatigue)
    // 疲劳 (Fatigue = 疲劳度，时间流逝增加疲劳度)
    // Fatigue (Fatigue = fatigue level, time passing increases fatigue)
    State->Fatigue = FMath::Clamp(State->Fatigue + (EnergyRate * DeltaTime), 0.0f, 1.0f);
    
    // 无聊 (Boredom = 缺乏刺激，时间流逝增加无聊感)
    // Boredom (Boredom = lack of stimulation, time passing increases boredom)
    State->Boredom = FMath::Clamp(State->Boredom + (BoredomRate * DeltaTime), 0.0f, 1.0f);
    
    // 孤独 (Loneliness = 缺乏社交，时间流逝增加孤独感)
    // Loneliness (Loneliness = lack of social, time passing increases loneliness)
    State->Loneliness = FMath::Clamp(State->Loneliness + (LonelinessRate * DeltaTime), 0.0f, 1.0f);

    // 🔍 调试日志：每 5 秒打印一次状态
    // Debug logging: Print status every 5 seconds
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastDebugLogTime > 5.0f)
    {
        float HungerDelta = State->Hunger - OldHunger;
        AINPC_LOG(Warning, "%s - Hg:%.2f Ft:%.2f Bd:%.2f Ln:%.2f", 
               *CachedController->GetName(), State->Hunger, State->Fatigue, State->Boredom, State->Loneliness);
        LastDebugLogTime = CurrentTime;
        
        // Note: Emotion calculation is now only done by LLM + score decay
        // EmotionEvaluator is NOT called here - LLM controls emotion, score just decays
    }


    // === 2. 情绪冷却与平滑过渡 (Cool Down & Smooth Transition) ===
    // 负责将当前状态平滑过渡到 LLM 设定的目标值（向上增长或向下衰减）
    // Responsible for smoothly transitioning current state to LLM's target value (growth or decay)

    auto ApplyStateTransition = [&](float& CurrentValue, const FString& VariableName, float DecayRate)
    {
        float TargetValue = 0.0f;
        if (Interpolator)
        {
            TargetValue = Interpolator->GetTargetValue(VariableName);
        }

        // 修正后的逻辑：
        // 1. 只有当目标值 > 当前值时（新的刺激），才去追赶目标。
        // 2. 否则（没有刺激，或者刺激比当前状态弱），就自然向 0 衰减。
        // This fixes the "stuck at high value" or "decay too fast" issue.
        
        if (TargetValue > CurrentValue)
        {
            // 上升期：极速追赶目标 (Reaction)
            // Rising: Rush to target instantly
            CurrentValue = FMath::FInterpTo(CurrentValue, TargetValue, DeltaTime, 10.0f);
        }
        else
        {
            // 下降期：忽略那个可能还卡在高位的 Target，直接向 0 衰减 (Recovery)
            // Falling: Ignore stale target, decay naturally to 0.0
            CurrentValue = FMath::FInterpTo(CurrentValue, 0.0f, DeltaTime, DecayRate);
        }
    };

    // 屈辱 (Indignity) -> 愤怒
    ApplyStateTransition(State->Indignity, TEXT("Indignity"), EmotionalDecayRate);

    // 威胁感 (Perceived_Threat) - 消失得比较快，因为如果没有持续威胁，你就安全了
    ApplyStateTransition(State->Perceived_Threat, TEXT("Perceived_Threat"), ThreatDecayRate);

    // ✅ CHECK SAFETY RESTORATION
    // 检查安全恢复：如果威胁从高降到低，清理敌对记忆
    // Check Safety Restoration: If threat drops from high to low, clean up hostile memories
    if (State->Perceived_Threat > 0.4f)
    {
        bWasThreatened = true;
    }
    else if (bWasThreatened && State->Perceived_Threat < 0.1f)
    {
        // Safe again! Resolve hostile memories on the Pawn's MemoryComponent
        if (CachedController && CachedController->GetPawn())
        {
             if (UMemoryComponent* MemComp = CachedController->GetPawn()->FindComponentByClass<UMemoryComponent>())
             {
                 MemComp->MarkAllHostileMemoriesResolved();
             }
        }
        bWasThreatened = false;
    }

    // === 3. 情绪分数衰减（简化版）===
    // Emotion Score Decay (Simplified)
    // 每 10 秒衰减 0.2，当 Score < 0.3 时变回 Neutral
    static float LastEmotionDecayTime = 0.0f;
    float EmotionDecayTime = GetWorld()->GetTimeSeconds();
    
    // 5秒迭代一次，每次 -0.2，总共约20秒衰减完毕 (1.0 -> 0.2 需要4次)
    // Iterate every 5 seconds, -0.2 each time, total decay ~20s
    if (EmotionDecayTime - LastEmotionDecayTime > 5.0f)
    {
        LastEmotionDecayTime = EmotionDecayTime;
        
        // 衰减情绪分数
        if (State->CurrentEmotionScore > 0.0f)
        {
            float OldScore = State->CurrentEmotionScore;
            State->CurrentEmotionScore -= 0.2f;
            State->CurrentEmotionScore = FMath::Max(State->CurrentEmotionScore, 0.0f);
            
            AINPC_LOG(Warning, "🎭 Emotion Score Decay: %.2f -> %.2f", OldScore, State->CurrentEmotionScore);
            
            // 当分数降到阈值以下，变回 Neutral
            if (State->CurrentEmotionScore < 0.3f && State->CurrentEmotion != EEmotionState::Neutral)
            {
                AINPC_LOG(Warning, "🎭 Emotion Score %.2f < 0.3, resetting to Neutral", 
                       State->CurrentEmotionScore);
                
                State->CurrentEmotion = EEmotionState::Neutral;
                if (CachedController)
                {
                    CachedController->CurrentEmotion = EEmotionState::Neutral;
                    if (CachedController->EmotionDisplayComp)
                    {
                        CachedController->EmotionDisplayComp->ShowEmotion("Neutral");
                    }
                }
            }
        }
    }

    // === 4. 长期状态 (Long-term States) ===
    // Trust, Social_Status, Loneliness, Duty_Urgency
    // 这些属性通常具有高惯性，完全由 LLM 控制。
}
