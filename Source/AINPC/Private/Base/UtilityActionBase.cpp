#include "Base/UtilityActionBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PersonalityComponent.h"



UUtilityActionBase::UUtilityActionBase()
{
    // 默认名字
    ActionName = "BaseAction";
}

void UUtilityActionBase::InitFromConfig(const FUtilityActionConfig& Config)
{
    Considerations = Config.Considerations;
    BaseReward = Config.BaseReward;
    CooldownTime = Config.CooldownTime;
    InertiaBonus = Config.InertiaBonus; // 接收配置的惯性值

    if (ActionName.Equals("BaseAction") || ActionName.IsEmpty())
    {
        ActionName = GetName();
    }
}

float UUtilityActionBase::CalculateScore(UNPCMentalState* MentalState, AAIController* Controller)
{
    // 0. 安全检查
    if (!Controller) return 0.0f;
    
    // ✅ 如果没有 Considerations，直接返回 BaseReward
    if (Considerations.Num() == 0) 
    {
        return BaseReward;
    }

    // 1. 冷却检查 (Cooldown)
    float CurrentTime = 0.0f;
    if (UWorld* World = Controller->GetWorld())
    {
        CurrentTime = World->GetTimeSeconds();
        // 如果还在冷却期内，直接返回 0 分
        if (CurrentTime - LastExecutedTime < CooldownTime)
        {
            return 0.0f; 
        }
    }

    // 2. 获取 PersonalityComponent (用于查询权重)
    UPersonalityComponent* PersonalityComp = nullptr;
    if (APawn* BotPawn = Controller->GetPawn())
    {
        PersonalityComp = BotPawn->FindComponentByClass<UPersonalityComponent>();
    }

    // 3. 两阶段计算 (Two-Phase Calculation)
    // 第一阶段：动机求和 Σ(Weight × Input)
    // 第二阶段：必要条件乘积 ∏(Context)
    
    float MotivationSum = 0.0f;      // 动机总和（加法）
    float ContextProduct = 1.0f;     // 必要条件乘积（乘法）

    // 🔍 调试：打印初始分数
    static bool bEnableDetailedLog = true; // 设为 true 来启用详细日志
    if (bEnableDetailedLog)
    {
        UE_LOG(LogTemp, Warning, TEXT("    [%s] Starting calculation: BaseReward=%.2f"), *ActionName, BaseReward);
    }

    for (int32 i = 0; i < Considerations.Num(); ++i)
    {
        const FUtilityConsideration& Factor = Considerations[i];
        
        // A. 获取原始数据 (0~1 或 实际值)
        float RawValue = GetConsiderationValue(Factor.InputType, MentalState, Controller);

        if (Factor.ConsiderationType == EConsiderationType::Motivation)
        {
            // === 动机类型：加法求和 ===
            
            // B. 获取性格权重 (从 PersonalityComponent)
            float PersonalityWeight = 1.0f;  // 默认权重
            
            if (PersonalityComp)
            {
                FString VariableName = GetVariableNameFromInputType(Factor.InputType);
                PersonalityWeight = PersonalityComp->GetWeightForVariable(VariableName);
            }

            // C. 计算动机得分并累加
            float MotivationScore = RawValue * PersonalityWeight;
            MotivationSum += MotivationScore;
            
            if (bEnableDetailedLog)
            {
                UE_LOG(LogTemp, Warning, TEXT("      [Motivation %d] %s: Raw=%.3f × Weight=%.3f = %.3f (Sum=%.3f)"), 
                       i, *GetVariableNameFromInputType(Factor.InputType), 
                       RawValue, PersonalityWeight, MotivationScore, MotivationSum);
            }
        }
        else // EConsiderationType::Context
        {
            // === 必要条件类型：乘法 ===
            
            // Context 直接使用原始值（0~1），不需要权重
            float OldProduct = ContextProduct;
            ContextProduct *= RawValue;
            
            if (bEnableDetailedLog)
            {
                UE_LOG(LogTemp, Warning, TEXT("      [Context %d] %s: %.3f × %.3f = %.3f"), 
                       i, *GetVariableNameFromInputType(Factor.InputType), 
                       OldProduct, RawValue, ContextProduct);
            }

            // 优化：如果必要条件已经为0，直接返回0
            if (ContextProduct <= UE_KINDA_SMALL_NUMBER)
            {
                if (bEnableDetailedLog)
                {
                    UE_LOG(LogTemp, Warning, TEXT("      ⚠️ Context became 0! Action is impossible."));
                }
                return 0.0f;
            }
        }
    }

    // 4. 最终得分计算 / Final Score Calculation
    // 公式 / Formula:
    // Score = BaseReward × (Σ Motivations) × (∏ Contexts)
    //       = 动作奖励 × 动机总和 × 必要条件乘积
    //       = ActionReward × MentalState × Personality × Contexts
    //
    // 例如 / Example:
    // Eat 动作: BaseReward=3.0 (吃饭很管饱)
    //   Motivation: Hunger(0.8) × HungerWeight(1.2) = 0.96
    //   Context: HasFood(1.0)
    //   Score = 3.0 × 0.96 × 1.0 = 2.88
    float FinalScore = BaseReward * MotivationSum * ContextProduct;

    // 🔍 调试：打印最终分数
    if (bEnableDetailedLog)
    {
        UE_LOG(LogTemp, Warning, TEXT("    [%s] Final Score: BaseReward(%.2f) × MotivationSum(%.2f) × ContextProduct(%.2f) = %.3f"), 
               *ActionName, BaseReward, MotivationSum, ContextProduct, FinalScore);
    }

    // 3. 惯性奖励 (Inertia / Momentum)
    // 这里的逻辑假设你会在 Controller 或 Component 里记录 CurrentAction
    // 如果没有这个机制，可以先注释掉下面这段
    /*
    if (UUtilityComponent* UtilityComp = Controller->FindComponentByClass<UUtilityComponent>())
    {
        if (UtilityComp->GetCurrentAction() == this)
        {
             // 加上惯性分（或者使用乘法 FinalScore *= 1.2f）
             FinalScore += InertiaBonus;
        }
    }
    */
    
    return FinalScore;
}

void UUtilityActionBase::MarkExecutionTime(float CurrentTime)
{
    LastExecutedTime = CurrentTime;
}

float UUtilityActionBase::GetConsiderationValue(EUtilityInputType InputType, UNPCMentalState* State, AAIController* Controller)
{
    APawn* BotPawn = Controller ? Controller->GetPawn() : nullptr;
    if (!BotPawn) return 0.0f;

    switch (InputType)
    {
        // === 马斯洛需求层次 (手动映射) ===
        // Maslow's Hierarchy (Manual Mapping)
        // 注意：枚举值是驼峰命名，但字段名可能有下划线
        
        // 生理层 (Physiological)
        case EUtilityInputType::Hunger:
            return State ? State->Hunger : 0.0f;
        case EUtilityInputType::Energy:
            return State ? State->Energy : 0.0f;
        
        // 安全层 (Safety)
        case EUtilityInputType::PerceivedThreat:
            return State ? State->Perceived_Threat : 0.0f;
        case EUtilityInputType::ResourceAnxiety:
            return State ? State->Resource_Anxiety : 0.0f;
        
        // 社交层 (Love/Belonging)
        case EUtilityInputType::Loneliness:
            return State ? State->Loneliness : 0.0f;
        case EUtilityInputType::Trust:
            return State ? State->Trust : 0.0f;
        
        // 尊严层 (Esteem)
        case EUtilityInputType::Anger:
            return State ? State->Anger : 0.0f;
        case EUtilityInputType::SocialStatus:
            return State ? State->Social_Status : 0.0f;
        
        // 自我实现层 (Self-Actualization)
        case EUtilityInputType::DutyUrgency:
            return State ? State->Duty_Urgency : 0.0f;
        case EUtilityInputType::Curiosity:
            return State ? State->Curiosity : 0.0f;

        // --- 自身状态 (Self Status) ---
        case EUtilityInputType::SelfHealth:
            // TODO: 请替换为实际获取血量的代码
            // if (auto* HealthComp = BotPawn->FindComponentByClass<UHealthComponent>())
            //     return HealthComp->GetHealthPercent();
            return 1.0f; // 默认满血

        case EUtilityInputType::AmmoCount:
             // TODO: 请替换为实际弹药逻辑
            return 1.0f; 

        // --- 环境感知 (Environment) ---
        case EUtilityInputType::DistanceToTarget:
        {
            AActor* Target = Controller->GetFocusActor(); // 或者你自己实现的 GetTarget()
            if (Target)
            {
                float Dist = FVector::Dist(BotPawn->GetActorLocation(), Target->GetActorLocation());
                // 归一化：假设 2000 厘米是最大考量距离，超过就算 1.0
                return FMath::Clamp(Dist / 2000.0f, 0.0f, 1.0f);
            }
            return 1.0f; // 没有目标通常意味着距离无限远
        }

        case EUtilityInputType::HasCover:
            // TODO: 这里需要接入 EQS (Environment Query System)
            // 简单示例：如果不被看到就算有掩体
            return 0.0f; 

        case EUtilityInputType::IsTargetPlayer:
            if (AActor* Target = Controller->GetFocusActor())
            {
                // 判断目标是否是玩家 Character
                return Target->IsA(APawn::StaticClass()) && Target->GetNetMode() != NM_Standalone ? 1.0f : 0.0f; 
            }
            return 0.0f;
        
        case EUtilityInputType::TargetHealth:
            // TODO: 获取目标血量
            return 1.0f;

        default:
            return 0.0f;
    }
}

FString UUtilityActionBase::GetVariableNameFromInputType(EUtilityInputType InputType) const
{
    // 将枚举值转换为字符串，用于查询 PersonalityComponent
    // Convert enum value to string for PersonalityComponent lookup
    switch (InputType)
    {
        // 马斯洛需求层次 (Maslow's Hierarchy)
        case EUtilityInputType::Hunger:            return TEXT("Hunger");
        case EUtilityInputType::Energy:            return TEXT("Energy");
        case EUtilityInputType::PerceivedThreat:   return TEXT("Perceived_Threat");
        case EUtilityInputType::ResourceAnxiety:   return TEXT("Resource_Anxiety");
        case EUtilityInputType::Loneliness:        return TEXT("Loneliness");
        case EUtilityInputType::Trust:             return TEXT("Trust");
        case EUtilityInputType::Anger:             return TEXT("Anger");
        case EUtilityInputType::SocialStatus:      return TEXT("Social_Status");
        case EUtilityInputType::DutyUrgency:       return TEXT("Duty_Urgency");
        case EUtilityInputType::Curiosity:         return TEXT("Curiosity");
        
        // 环境变量 (Environment variables) - 使用默认权重 1.0
        case EUtilityInputType::SelfHealth:
        case EUtilityInputType::TargetHealth:
        case EUtilityInputType::DistanceToTarget:
        case EUtilityInputType::AmmoCount:
        case EUtilityInputType::HasCover:
        case EUtilityInputType::IsTargetPlayer:
            return TEXT("Environment");  // 环境变量不需要性格权重
        
        default:
            return TEXT("Unknown");
    }
}