#include "Components/MetabolismComponent.h"
#include "Controller/UtilityAIController.h"
#include "UtilityAI/NPCMentalState.h"
#include "Components/CognitionComponent.h"
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

    // === 1. 生理需求 (总是自动增长) ===
    // Physiological needs (Always increase)
    
    // 饥饿 (Hunger)
    State->Hunger = FMath::Clamp(State->Hunger + (HungerRate * DeltaTime), 0.0f, 1.0f);
    
    // 疲劳 (Energy) - 注意：这里我们将 Energy 定义为"疲劳度" (0=精力充沛, 1=累死)
    // 如果你的定义是能量值 (1=满电)，请改为减法
    // 假设 Energy 代表 "Fatigue/Tiredness" 为了统一逻辑 (值越高越想睡觉)
    // 如果 Energy 代表 "Battery", 请告诉我，我改为 -= 
    // 根据 Maslow 里的定义，Energy 通常指"需要能量"，所以越高越缺能量。
    State->Energy = FMath::Clamp(State->Energy + (EnergyRate * DeltaTime), 0.0f, 1.0f);


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

    // 愤怒 (Anger)
    ApplyDecay(State->Anger, TEXT("Anger"), EmotionalDecayRate);

    // 威胁感 (Perceived_Threat) - 消失得比较快，因为如果没有持续威胁，你就安全了
    ApplyDecay(State->Perceived_Threat, TEXT("Perceived_Threat"), ThreatDecayRate);

    // 资源焦虑 (Resource_Anxiety)
    ApplyDecay(State->Resource_Anxiety, TEXT("Resource_Anxiety"), EmotionalDecayRate * 0.5f); // 焦虑消失得慢一点

    // 好奇心 (Curiosity) - 也会随时间消退
    ApplyDecay(State->Curiosity, TEXT("Curiosity"), EmotionalDecayRate);

    // === 3. 长期状态 (Long-term States) ===
    // Trust, Social_Status, Loneliness, Duty_Urgency
    // 这些属性通常具有高惯性，或者在这个版本中我们不让它们自动衰减，完全由 LLM 控制。
}
