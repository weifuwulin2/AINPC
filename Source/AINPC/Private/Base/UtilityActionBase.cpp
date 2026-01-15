#include "Base/UtilityActionBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PersonalityComponent.h"
#include "Controller/UtilityAIController.h"
#include "Components/CognitionComponent.h"
#include "Components/SensoryComponent.h"
#include "EngineUtils.h"
#include "UtilityAI/MentalStateInterpolation.h"
#include "Social/SocialGameplayTags.h"
#include "Actions/Action_SmartObject.h"
#include "UtilityAI/EmotionMatrixConfig.h"


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
    InertiaBonus = Config.InertiaBonus;
    SmartObjectTag = Config.SmartObjectTag;
    ActivityTag = Config.ActivityTag; // 用于 Emotion Matrix 查表
    IntentionTag = Config.IntentionTag; // 用于 LLM Intention 匹配

    if (ActionName.Equals("BaseAction") || ActionName.IsEmpty())
    {
        ActionName = GetName();
    }
}

float UUtilityActionBase::CalculateScore(UNPCMentalState* MentalState, AAIController* Controller, bool bLogDebug)
{
    // 0. 安全检查
    if (!Controller) return 0.0f;
    
    // ✅ 死亡检查：如果 Pawn 已死亡或无效，所有动作得分为 0
    // Death Check: If Pawn is dead or invalid, all actions score 0
    APawn* ControlledPawn = Controller->GetPawn();
    if (!ControlledPawn || !IsValid(ControlledPawn) || ControlledPawn->IsPendingKillPending())
    {
        return 0.0f;
    }

    // ❌ 移除死亡动画检查 - 吃饭/睡觉动画也会触发 IsAnyMontagePlaying()
    // ❌ Removed death animation check - Eating/Sleeping animations also trigger IsAnyMontagePlaying()
    // TODO: 如果需要死亡检查，应该使用专门的死亡标记或动画标签
    // TODO: If death check is needed, use specific death flag or animation tags

    // ✅ 持续时长检查：如果是 Action_SmartObject 且持续时长已到期，得分为 0
    // Duration Check: If Action_SmartObject and duration expired, score is 0
    if (UAction_SmartObject* SmartObjectAction = Cast<UAction_SmartObject>(this))
    {
        if (SmartObjectAction->ShouldExit(Controller))
        {
            if (bLogDebug)
            {
                UE_LOG(LogTemp, Warning, TEXT("    [%s] ⏱️ Duration expired, forcing score to 0"), *ActionName);
            }
            return 0.0f;
        }
    }
    
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
    if (bLogDebug)
    {
        UE_LOG(LogTemp, Warning, TEXT("    [%s] Starting calculation: BaseReward=%.2f"), *ActionName, BaseReward);
    }

    for (int32 i = 0; i < Considerations.Num(); ++i)
    {
        const FUtilityConsideration& Factor = Considerations[i];
        
        // A. 获取原始数据 (0~1 或 实际值)
        float RawValue = GetConsiderationValue(Factor.InputType, MentalState, Controller);

        // A2. 应用响应曲线 (Response Curve)
        float CurveValue = RawValue;
        
        // 优先使用自定义曲线资源 (Advanced)
        if (Factor.ResponseCurve)
        {
            // 假设输入都在 0~1 之间，直接采样
            CurveValue = Factor.ResponseCurve->GetFloatValue(RawValue);
        }
        else
        {
            // 使用预设的数学曲线 (Built-in)
            switch (Factor.CurveType)
            {
                case EUtilityCurveType::Linear:
                    CurveValue = RawValue;
                    break;
                case EUtilityCurveType::Quadratic:
                    CurveValue = RawValue * RawValue; // y = x^2 (Low values get suppressed)
                    break;
                case EUtilityCurveType::InverseQuadratic:
                    CurveValue = 1.0f - (1.0f - RawValue) * (1.0f - RawValue); // y = 1-(1-x)^2 (Fast rise)
                    break;
                case EUtilityCurveType::Logistic:
                    // Simple sigmoid-like: y = 1 / (1 + e^(-10*(x-0.5)))
                    CurveValue = 1.0f / (1.0f + FMath::Exp(-10.0f * (RawValue - 0.5f)));
                    break;
                case EUtilityCurveType::Step:
                    CurveValue = (RawValue >= 0.5f) ? 1.0f : 0.0f;
                    break;
                 case EUtilityCurveType::TargetThreshold:
                    CurveValue = (RawValue >= 0.1f) ? 1.0f : 0.0f; // 简单的阈值过滤
                    break;
                 case EUtilityCurveType::Inverse:
                    CurveValue = 1.0f - RawValue; // 反向：有敌人(1.0) -> 0.0, 没敌人(0.0) -> 1.0
                    break;
                default:
                    CurveValue = RawValue;
                    break;
            }
        }
        
        // 确保输出在合理范围 (0~1)
        CurveValue = FMath::Clamp(CurveValue, 0.0f, 1.0f);
        
        // 使用曲线后的值继续计算
        // Use the curved value for further calculation
        float EffectiveValue = CurveValue;

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
            float MotivationScore = EffectiveValue * PersonalityWeight;
            MotivationSum += MotivationScore;
            
            if (bLogDebug)
            {
                // 如果用了曲线，打印一下原始值和曲线值
                FString CurveInfo;
                if (Factor.ResponseCurve)
                    CurveInfo = FString::Printf(TEXT("(CustomCurve: %.2f->%.2f)"), RawValue, CurveValue);
                else if (Factor.CurveType != EUtilityCurveType::Linear)
                    CurveInfo = FString::Printf(TEXT("(%s: %.2f->%.2f)"), 
                        *UEnum::GetValueAsString(Factor.CurveType), RawValue, CurveValue);
                
                UE_LOG(LogTemp, Warning, TEXT("      [Motivation %d] %s: %s Raw=%.3f × Weight=%.3f = %.3f (Sum=%.3f)"), 
                       i, *GetVariableNameFromInputType(Factor.InputType), 
                       *CurveInfo,
                       EffectiveValue, PersonalityWeight, MotivationScore, MotivationSum);
            }
        }
        else // EConsiderationType::Context
        {
            // === 必要条件类型：乘法 ===
            
            // Context 直接使用原始值（0~1），不需要权重
            float OldProduct = ContextProduct;
            ContextProduct *= EffectiveValue;
            
            if (bLogDebug)
            {
                UE_LOG(LogTemp, Warning, TEXT("      [Context %d] %s: %.3f × %.3f = %.3f"), 
                       i, *GetVariableNameFromInputType(Factor.InputType), 
                       OldProduct, EffectiveValue, ContextProduct);
            }

            // 优化：如果必要条件已经为0，直接返回0
            if (ContextProduct <= UE_KINDA_SMALL_NUMBER)
            {
                if (bLogDebug)
                {
                    UE_LOG(LogTemp, Error, TEXT("      ⛔ ABORTING: Context '%s' is 0! Final Score = 0.0"), 
                           *GetVariableNameFromInputType(Factor.InputType));
                }
                return 0.0f;
            }
        }
    }

    // 4. 计算 LLM 意图加成 (Intention Bonus) - [CHANGED to Additive]
    // ---------------------------------------------------------
    // 将意图作为一种"额外的动机" (Extra Motivation)，而不是最终乘数。
    // Treat Intention as "Extra Motivation" that is added, not multiplied.
    // 这样做的优点：
    // 1. Agency: 即使 MotivationSum 为 0 (不饿/不怒)，LLM 也能无中生有驱动行为 ((0 + 1) * Context > 0)。
    // 2. Safety: 仍然受 Context 制约。如果 Context 为 0 (没子弹)，(Mot + 1) * 0 还是 0。
    
    float IntentionBonus = 0.0f;
    if (MentalState && !IntentionTag.IsEmpty())
    {
        if (MentalState->Intention.Contains(IntentionTag, ESearchCase::IgnoreCase))
        {
            IntentionBonus = 1.0f; // Additive Bonus as requested
            
            // ✅ 调试：显示意图匹配 (Debug: Show Intention Match)
            if (bLogDebug)
            {
                UE_LOG(LogTemp, Warning, TEXT("      [Intention] 🧠 LLM MATCH! ActionTag:'%s' matches MentalState:'%s'. Bonus:+1.0"), 
                       *IntentionTag, *MentalState->Intention);
            }
        }
    }

    // 5. 最终得分计算 / Final Score Calculation
    // 公式 / Formula:
    // Score = BaseReward × (MotivationSum + IntentionBonus) × ContextProduct
    float EffectiveMotivation = MotivationSum + IntentionBonus;
    float FinalScore = BaseReward * EffectiveMotivation * ContextProduct;

    // 6. 应用情绪矩阵乘数 / Apply Emotion Matrix Multiplier
    // ---------------------------------------------------------
    
    float EmotionMultiplier = 1.0f;
    FString EmotionLogInfo = TEXT("None (1.0)");
    
    if (AUtilityAIController* UtilityController = Cast<AUtilityAIController>(Controller))
    {
        // 🔍 诊断日志: 检查 EmotionMatrixTable 是否存在
        if (bLogDebug)
        {
            UE_LOG(LogTemp, Log, TEXT("      [Emotion Debug] Controller=%s, EmotionMatrixTable=%s, CurrentEmotion=%s, ActivityTag=%s"),
                   *UtilityController->GetName(),
                   UtilityController->EmotionMatrixTable ? TEXT("VALID") : TEXT("NULL"),
                   *UEnum::GetValueAsString(UtilityController->CurrentEmotion),
                   *ActivityTag.ToString());
        }
        
        if (UtilityController->EmotionMatrixTable)
        {
            // 获取当前情绪名 (e.g., "Angry", "Scared")
            FString EmotionName = UEnum::GetValueAsString(UtilityController->CurrentEmotion);
            FString CleanEmotionName;
            EmotionName.Split(TEXT("::"), nullptr, &CleanEmotionName);
            FName RowName = FName(*CleanEmotionName);

            // 查找矩阵行
            static const FString ContextString(TEXT("UtilityActionBase::CalculateScore"));
            FEmotionMatrixRow* MatrixRow = UtilityController->EmotionMatrixTable->FindRow<FEmotionMatrixRow>(RowName, ContextString);
            
            if (MatrixRow)
            {
                // 获取特定 Activity 的乘数
                EmotionMultiplier = MatrixRow->GetMultiplier(ActivityTag);
                FinalScore *= EmotionMultiplier;
                
                if (bLogDebug && FMath::Abs(EmotionMultiplier - 1.0f) > KINDA_SMALL_NUMBER)
                {
                    EmotionLogInfo = FString::Printf(TEXT("%s->%s (x%.2f)"), *CleanEmotionName, *ActivityTag.ToString(), EmotionMultiplier);
                    UE_LOG(LogTemp, Warning, TEXT("      [Emotion] 🎭 Multiplier Applied: %s"), *EmotionLogInfo);
                }
            }
        }
    }

    // 🔍 调试：打印最终分数详情
    if (bLogDebug)
    {
        UE_LOG(LogTemp, Warning, TEXT("    [%s] 📊 Calculation Summary:"), *ActionName);
        UE_LOG(LogTemp, Log, TEXT("      • Base Reward: %.2f"), BaseReward);
        UE_LOG(LogTemp, Log, TEXT("      • Motivation Sum: %.2f (Sum of inputs * weights)"), MotivationSum);
        UE_LOG(LogTemp, Log, TEXT("      • Intention Bonus: %.2f %s"), IntentionBonus, IntentionBonus > 0 ? TEXT("(✅ APPLIED)") : TEXT(""));
        UE_LOG(LogTemp, Log, TEXT("      • Context Product: %.2f (Multiplier)"), ContextProduct);
        UE_LOG(LogTemp, Log, TEXT("      • Emotion Multiplier: %.2f (%s)"), EmotionMultiplier, *EmotionLogInfo);
        UE_LOG(LogTemp, Warning, TEXT("      👉 FINAL SCORE = %.2f * (%.2f + %.2f) * %.2f * %.2f = %.3f"), 
               BaseReward, MotivationSum, IntentionBonus, ContextProduct, EmotionMultiplier, FinalScore);
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

    // 🔍 尝试获取 MentalStateInterpolator 来使用 Target 值
    // Try to get MentalStateInterpolator to use Target values
    UMentalStateInterpolator* Interpolator = nullptr;
    if (AUtilityAIController* UtilityController = Cast<AUtilityAIController>(Controller))
    {
        if (UtilityController->CognitionComp)
        {
            Interpolator = UtilityController->CognitionComp->Interpolator;
        }
    }

    switch (InputType)
    {
        // === 马斯洛需求层次 (使用 Target 值) ===
        // Maslow's Hierarchy (Use Target Values)
        // 简化后的 6 个核心字段
        
        // ✅ 生理层 (Physiological) - ENGINE 独裁
        case EUtilityInputType::Hunger:
            return State ? State->Hunger : 0.0f;
        case EUtilityInputType::Fatigue:
            return State ? State->Fatigue : 0.0f;
        
        // ✅ 安全层 (Safety)
        case EUtilityInputType::PerceivedThreat:
            return Interpolator ? Interpolator->GetTargetValue(TEXT("Perceived_Threat")) : (State ? State->Perceived_Threat : 0.0f);
        
        // ✅ 社交层 (Belonging)
        case EUtilityInputType::Loneliness:
            return Interpolator ? Interpolator->GetTargetValue(TEXT("Loneliness")) : (State ? State->Loneliness : 0.0f);
        
        // ✅ 尊严层 (Esteem)
        case EUtilityInputType::Indignity:
            return Interpolator ? Interpolator->GetTargetValue(TEXT("Indignity")) : (State ? State->Indignity : 0.0f);
        
        // ✅ 自我实现层 (Self-Actualization)
        case EUtilityInputType::Boredom:
            return Interpolator ? Interpolator->GetTargetValue(TEXT("Boredom")) : (State ? State->Boredom : 0.0f);

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

        // ✅ 新增：检查是否有攻击目标
        case EUtilityInputType::HasAttackTarget:
        {
            AActor* Target = Controller->GetFocusActor();
            
            // 检查目标是否存在、有效、且未被销毁
            if (Target && IsValid(Target) && !Target->IsPendingKillPending())
            {
                // 排除已标记为死亡的目标
                if (Target->ActorHasTag("Dead")) return 0.0f;

                // 排除如果是布娃娃状态（物理模拟通常意味着死亡）
                if (ACharacter* CharTarget = Cast<ACharacter>(Target))
                {
                    if (CharTarget->GetMesh() && CharTarget->GetMesh()->IsSimulatingPhysics()) return 0.0f;
                }

                return 1.0f; // 有有效目标
            }
            
            return 0.0f; // 没有目标
        }

        // ✅ 新增：检查附近是否有敌人（不依赖 FocusActor）
        case EUtilityInputType::HasEnemyNearby:
        {
            if (!BotPawn || !Controller->GetWorld()) return 0.0f;
            
            // 获取自己的阵营
            EFactionType MyFaction = USensoryComponent::GetFaction(BotPawn);
            float CheckRadiusSq = FMath::Square(1500.0f); // 15m 范围内

            // 扫描周围所有 Pawn (使用 TActorIterator 代替 GetPawnIterator)
            if (UWorld* World = Controller->GetWorld())
            {
                for (TActorIterator<APawn> It(World); It; ++It)
                {
                    APawn* TestPawn = *It;
                    if (!TestPawn || TestPawn == BotPawn) continue;
                    
                    // 1. 距离检查 (优化：先检查距离，再做昂贵的逻辑)
                    if (FVector::DistSquared(BotPawn->GetActorLocation(), TestPawn->GetActorLocation()) > CheckRadiusSq)
                    {
                        continue;
                    }

                    if (!IsValid(TestPawn) || TestPawn->IsPendingKillPending()) continue;

                    if (TestPawn->ActorHasTag("Dead")) continue;
                    if (ACharacter* CharTest = Cast<ACharacter>(TestPawn))
                    {
                        if (CharTest->GetMesh() && CharTest->GetMesh()->IsSimulatingPhysics()) continue;
                    }

                    // 阵营检查 logic check
                    EFactionType TargetFaction = USensoryComponent::GetFaction(TestPawn);
                    
                    bool bIsHostile = false;

                    // Reuse the logic: Different Faction = Enemy (ignoring Neutral)
                    if (MyFaction != EFactionType::Neutral && TargetFaction != EFactionType::Neutral)
                    {
                        if (MyFaction != TargetFaction)
                        {
                            bIsHostile = true;
                        }
                    }
                    
                    if (bIsHostile)
                    {
                        return 1.0f;
                    }
                }
            }
            
            return 0.0f; // 没有敌人
        }

        // ✅ 新增：检查附近是否有友军 (非 Enemy)
        case EUtilityInputType::HasFriendlyNearby:
        {
            if (!BotPawn || !Controller->GetWorld()) return 0.0f;

            // 获取自己的阵营
            EFactionType MyFaction = USensoryComponent::GetFaction(BotPawn);
            float CheckRadiusSq = FMath::Square(1500.0f); // 15m 范围内

            // 这里我们简单起见，利用 UGameplayStatics::GetAllActorsOfClass 检查所有 Character
            // 然后过滤掉 Enemy 和 Self
            TArray<AActor*> AllChars;
            UGameplayStatics::GetAllActorsOfClass(Controller->GetWorld(), ACharacter::StaticClass(), AllChars);

            for (AActor* Actor : AllChars)
            {
                if (Actor == BotPawn) continue;
                
                // 1. 距离检查
                if (FVector::DistSquared(BotPawn->GetActorLocation(), Actor->GetActorLocation()) > CheckRadiusSq)
                {
                    continue;
                }

                if (!IsValid(Actor) || Actor->IsPendingKillPending()) continue;
                if (Actor->ActorHasTag("Dead")) continue;  // 排除死人

                // 排除布娃娃
                if (ACharacter* CharActor = Cast<ACharacter>(Actor))
                {
                    if (CharActor->GetMesh() && CharActor->GetMesh()->IsSimulatingPhysics()) continue;
                }
                
                // 阵营检查
                EFactionType TargetFaction = USensoryComponent::GetFaction(Actor);
                
                // 友军定义：同阵营 OR (Neutral vs Neutral - not really friendly but safe)
                // 严格友军：MyFaction == TargetFaction (且不是Neutral)
                // 或者简单定义：Not Hostile
                
                bool bIsHostile = false;
                if (MyFaction != EFactionType::Neutral && TargetFaction != EFactionType::Neutral)
                {
                    if (MyFaction != TargetFaction) bIsHostile = true;
                }

                if (!bIsHostile)
                {
                    // 找到一个不是敌人的活人 (友军或路人)
                    return 1.0f;
                }
            }

            return 0.0f;
        }

        // ✅ 新增：检查是否有食物附近
        case EUtilityInputType::HasFoodNearby:
        {
            if (USensoryComponent* Sensory = Controller->FindComponentByClass<USensoryComponent>())
            {
                AActor* Food = Sensory->FindBestSmartObject(AINPCTags::Activity_Eat);
                return Food ? 1.0f : 0.0f;
            }
            return 0.0f;
        }

        // ✅ 新增：检查是否有床附近
        case EUtilityInputType::HasBedNearby:
        {
            if (USensoryComponent* Sensory = Controller->FindComponentByClass<USensoryComponent>())
            {
                 // 使用 AINPCTags::Activity_Rest
                if (Sensory->FindBestSmartObject(AINPCTags::Activity_Rest))
                {
                    return 1.0f;
                }
            }
            return 0.0f;
        }

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
        // 马斯洛需求层次 - 简化后的 6 个核心字段
        case EUtilityInputType::Hunger:            return TEXT("Hunger");
        case EUtilityInputType::Fatigue:           return TEXT("Fatigue");
        case EUtilityInputType::PerceivedThreat:   return TEXT("Perceived_Threat");
        case EUtilityInputType::Loneliness:        return TEXT("Loneliness");
        case EUtilityInputType::Indignity:         return TEXT("Indignity");
        case EUtilityInputType::Boredom:           return TEXT("Boredom");
        
        // 环境变量 (Environment variables) - 使用默认权重 1.0
        case EUtilityInputType::SelfHealth:
        case EUtilityInputType::TargetHealth:
        case EUtilityInputType::DistanceToTarget:
        case EUtilityInputType::AmmoCount:
        case EUtilityInputType::HasCover:
        case EUtilityInputType::IsTargetPlayer:
        case EUtilityInputType::HasAttackTarget:
        case EUtilityInputType::HasEnemyNearby:
        case EUtilityInputType::HasFriendlyNearby:
        case EUtilityInputType::HasFoodNearby:
        case EUtilityInputType::HasBedNearby:
            return TEXT("Environment");  // 环境变量不需要性格权重
        
        default:
            return TEXT("Unknown");
    }
}