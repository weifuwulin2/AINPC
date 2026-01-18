#include "Components/MetabolismComponent.h"
#include "Controller/UtilityAIController.h"
#include "UtilityAI/UNPCMentalState.h"
#include "Components/CognitionComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/EmotionDisplayComponent.h"
#include "UtilityAI/EmotionEvaluator.h"
#include "UtilityAI/MentalStateInterpolation.h"

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
        UE_LOG(LogTemp, Log, TEXT("[Metabolism] Component initialized for %s"), *CachedController->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Metabolism] ⚠️ Could not find UtilityAIController! Metabolism will not work."));
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
        UE_LOG(LogTemp, Warning, TEXT("[Metabolism] %s - Hg:%.2f Ft:%.2f Bd:%.2f Ln:%.2f"), 
               *CachedController->GetName(), State->Hunger, State->Fatigue, State->Boredom, State->Loneliness);
        LastDebugLogTime = CurrentTime;
        
        // Note: Emotion calculation is now only done by LLM + score decay
        // EmotionEvaluator is NOT called here - LLM controls emotion, score just decays
    }


    // === 2. 情绪冷却 (Cool Down) ===
    // 只有当 LLM 没有设定"高目标值"时，情绪才会消退
    // Only decay if LLM is not actively pushing it higher

    auto ApplyDecay = [&](float& CurrentValue, const FString& VariableName, float DecayRate)
    {
        float TargetValue = 0.0f;
        if (Interpolator)
        {
            TargetValue = Interpolator->GetTargetValue(VariableName);
        }

        // 如果当前值比目标值大（说明正在气头上，或者 LLM 已经撤销了目标），就开始衰减
        // 或者是 LLM 设定的目标值很低
        // 关键逻辑：我们只做"向下衰减"。如果 LLM 想要拉高，由 Interpolator 负责。
        if (CurrentValue > TargetValue)
        {
            // 使用 FInterpTo 模拟指数衰减 (自然冷却)
            CurrentValue = FMath::FInterpTo(CurrentValue, TargetValue, DeltaTime, DecayRate);
        }
    };

    // 屈辱 (Indignity) -> 愤怒
    ApplyDecay(State->Indignity, TEXT("Indignity"), EmotionalDecayRate);

    // 威胁感 (Perceived_Threat) - 消失得比较快，因为如果没有持续威胁，你就安全了
    ApplyDecay(State->Perceived_Threat, TEXT("Perceived_Threat"), ThreatDecayRate);

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
            
            UE_LOG(LogTemp, Warning, TEXT("[Metabolism] 🎭 Emotion Score Decay: %.2f -> %.2f"), OldScore, State->CurrentEmotionScore);
            
            // 当分数降到阈值以下，变回 Neutral
            if (State->CurrentEmotionScore < 0.3f && State->CurrentEmotion != EEmotionState::Neutral)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Metabolism] 🎭 Emotion Score %.2f < 0.3, resetting to Neutral"), 
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
