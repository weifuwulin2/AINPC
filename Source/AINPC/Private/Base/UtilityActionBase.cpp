#include "Base/UtilityActionBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"



UUtilityActionBase::UUtilityActionBase()
{
    // 默认名字
    ActionName = "BaseAction";
}

void UUtilityActionBase::InitFromConfig(const FUtilityActionConfig& Config)
{
    Considerations = Config.Considerations;
    BaseWeight = Config.Weight;
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
    if (!Controller || Considerations.Num() == 0) return 0.0f;

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

    // 2. 多因子计算 (Multi-Factor Scoring)
    float FinalScore = BaseWeight;

    for (const FUtilityConsideration& Factor : Considerations)
    {
        // A. 获取原始数据 (0~1 或 实际值)
        float RawValue = GetConsiderationValue(Factor.InputType, MentalState, Controller);

        // B. 曲线映射
        // 如果有曲线，用曲线查表；如果没有，直接用原始值截断到 0~1
        float FactorScore = (Factor.ResponseCurve) 
                            ? Factor.ResponseCurve->GetFloatValue(RawValue) 
                            : FMath::Clamp(RawValue, 0.0f, 1.0f);

        // C. 乘法累积 (核心：任何一个因子为0，总分为0)
        FinalScore *= FactorScore;

        // 优化：如果分数已经归零，直接退出，节省性能
        if (FinalScore <= UE_KINDA_SMALL_NUMBER) 
        {
            return 0.0f;
        }
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
        // ✅ 使用宏自动生成所有 MentalState 字段的 case 分支
        #define HANDLE_MENTAL_STATE_FIELD(Name, DefaultValue, DisplayName, Description) \
            case EUtilityInputType::Name: \
                return State ? State->Name : 0.0f;
        
        MENTAL_STATE_FIELDS(HANDLE_MENTAL_STATE_FIELD)
        
        #undef HANDLE_MENTAL_STATE_FIELD

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