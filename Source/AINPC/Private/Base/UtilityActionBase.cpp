#include "Base/UtilityActionBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PersonalityComponent.h"
#include "Controller/UtilityAIController.h"
#include "Components/CognitionComponent.h"
#include "Components/SensoryComponent.h"
#include "Components/GoalComponent.h"
#include "EngineUtils.h"
#include "UtilityAI/MentalStateInterpolation.h"
#include "Social/SocialGameplayTags.h"
#include "Actions/Action_SmartObject.h"
#include "Social/SocialGameplayTags.h" // ✅ Native Tags
#include "Components/UtilityAIComponent.h"
#include "UtilityAI/MentalStateNames.h" // ✅ Use Constants
#include "UtilityAI/EmotionMatrixConfig.h"

// Initialize Game-wide Constants
const float UUtilityActionBase::IntentionMatchBonus = 0.3f;
const float UUtilityActionBase::DirectiveMatchMultiplier = 1.5f;
const float UUtilityActionBase::DirectiveMismatchMultiplier = 0.5f;

UUtilityActionBase::UUtilityActionBase()
{
	// Default weights
	// Weight_Motivation = 1.0f;"BaseAction";
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
	DirectiveTag = Config.DirectiveTag; // 用于 Goal Directive 匹配
    PersonalityInfluence = Config.PersonalityInfluence; // 数据驱动 PAM 配置
    
    UE_LOG(LogTemp, Warning, TEXT("[InitFromConfig] %s: PersonalityInfluence entries = %d"), 
           *Config.ActionName, PersonalityInfluence.Num());

    // Fix: Assign name from config if available
    if (!Config.ActionName.IsEmpty())
    {
        ActionName = Config.ActionName;
    }
    else if (ActionName.Equals("BaseAction") || ActionName.IsEmpty())
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
        if (bLogDebug) UE_LOG(LogTemp, Warning, TEXT("    [%s] ❌ Score=0: Pawn Invalid/Dead"), *ActionName);
        return 0.0f;
    }

    // ❌ 移除死亡动画检查 - 吃饭/睡觉动画也会触发 IsAnyMontagePlaying()
    // ❌ Removed death animation check - Eating/Sleeping animations also trigger IsAnyMontagePlaying()
    // TODO: 如果需要死亡检查，应该使用专门的死亡标记或动画标签
    // TODO: If death check is needed, use specific death flag or animation tags

    // ❌ 移除了错误的 Duration 检查
    // ❌ Removed broken Duration check
    // ShouldExit 返回 true 的意思是"允许切换"，不是"这个动作不能被选择"
    // ShouldExit returning true means "allow switching", NOT "disable this action"
    // 这个检查会导致 SmartObject 动作永远得分 0，因为非活动动作的 ShouldExit 总是返回 true
    
    // ✅ 如果没有 Considerations，直接返回 BaseReward
    // ✅ 如果没有 Considerations，直接返回 BaseReward
    if (Considerations.Num() == 0) 
    {
        if (bLogDebug) UE_LOG(LogTemp, Warning, TEXT("    [%s] ⚠️ No Considerations, returning BaseReward=%.2f"), *ActionName, BaseReward);
        return BaseReward;
    }

    // 1. 冷却检查 (Cooldown)
    float CurrentTime = 0.0f;
    if (UWorld* World = Controller->GetWorld())
    {
        CurrentTime = World->GetTimeSeconds();
        // ✅ Check if we are the currently running action
        bool bIsRunning = false;
        if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(Controller)) 
        {
             if (UAICon->UtilityComp && UAICon->UtilityComp->CurrentAction == this)
             {
                 bIsRunning = true;
             }
        }

        // 如果还在冷却期内 且 不是正在运行，直接返回 0 分
        // If in cooldown AND not currently running, return 0
        // (Prevents the running action from disqualifying itself)
        if (!bIsRunning && CurrentTime - LastExecutedTime < CooldownTime)
        {
            if (bLogDebug) UE_LOG(LogTemp, Warning, TEXT("    [%s] ❌ Score=0: Cooldown (%.1fs left)"), 
                                  *ActionName, CooldownTime - (CurrentTime - LastExecutedTime));
            return 0.0f; 
        }
    }

    // 2. 获取 PersonalityComponent (用于查询权重)
    UPersonalityComponent* PersonalityComp = nullptr;
    
    // ✅ Priority 1: Check Controller (PersonalityComp is typically on Controller)
    if (AUtilityAIController* UtilityController = Cast<AUtilityAIController>(Controller))
    {
        PersonalityComp = UtilityController->PersonalityComp;
    }
    
    // ✅ Priority 2: Fallback to Pawn (if not found on Controller)
    if (!PersonalityComp)
    {
        if (APawn* BotPawn = Controller->GetPawn())
        {
            PersonalityComp = BotPawn->FindComponentByClass<UPersonalityComponent>();
        }
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
        float RawValue = GetConsiderationValue(Factor.InputType, MentalState, Controller, bLogDebug);

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
    if (MentalState && IntentionTag.IsValid())
    {
        // 构造预期 Tag 名：Intention.<String>
        // Construct expected Tag Name: Intention.<String>
        FString CurrentIntention = MentalState->Intention;
        if (!CurrentIntention.IsEmpty())
        {
            // 例如：LLM 输出 "Attack" => "Intention.Attack"
            FString TagNameToCheck = TEXT("Intention.") + CurrentIntention;
            
            // 简单字符串匹配：检查配置的 Tag 是否与构造的 Tag 名字匹配
            // Simple string check: Check if configured Tag matches constructed Tag Name
            // (Using ToString() is safer than RequestGameplayTag for non-existent tags)
            if (IntentionTag.ToString().Equals(TagNameToCheck, ESearchCase::IgnoreCase))
            {
                IntentionBonus = 1.0f; // Additive Bonus
            
                // ✅ 调试：显示意图匹配
                if (bLogDebug)
                {
                    UE_LOG(LogTemp, Warning, TEXT("      [Intention] 🧠 LLM MATCH! ActionTag:'%s' matches MentalState:'%s' (via %s). Bonus:+1.0"), 
                           *IntentionTag.ToString(), *MentalState->Intention, *TagNameToCheck);
                }
            }
        }
    }

            
        
    

    // 5. 最终得分计算 / Final Score Calculation
    // 公式 / Formula:
    // Score = BaseReward × (MotivationSum + IntentionBonus) × ContextProduct
    float EffectiveMotivation = MotivationSum + IntentionBonus;
    float FinalScore = BaseReward * EffectiveMotivation * ContextProduct;

    // =========================================================
    // 5.5. 应用数据驱动性格修正 (Data-Driven PAM)
    // =========================================================
    // Personality Action Modifier (PAM)
    // 根据配置直接修正最终分数 (独立于 Maslow 权重)
    // Directly modify final score based on configuration (independent of Maslow weights)
    
    if (PersonalityComp && PersonalityInfluence.Num() > 0)
    {
        for (const auto& Pair : PersonalityInfluence)
        {
            EOCEANTrait Trait = Pair.Key;
            float InfluenceFactor = Pair.Value; // e.g., -1.0 for Neuroticism on Attack
            
            // 获取性格特质值 (0.0 - 1.0)
            float TraitValue = PersonalityComp->GetTraitValue(Trait);
            
            // 计算修正乘数
            // Calculate Modifier Multiplier
            // 范围: 0.2x - 2.0x (更强的性格区分)
            // Range: 0.2x - 2.0x (Stronger personality differentiation)
            float Modifier = 1.0f;
            
            // 逻辑: Influence > 0 表示正相关 (Trait越高分越高)
            //       Influence < 0 表示负相关 (Trait越低分越高)
            // Logic: Influence > 0 means positive correlation (Higher trait = higher score)
            //        Influence < 0 means negative correlation (Lower trait = higher score)
            
            if (InfluenceFactor > 0)
            {
                // 正相关: 0.5 + (Trait * Factor * 1.5)
                // e.g. Factor=1.0, Trait=1.0 -> Mod = 0.5 + 1.5 = 2.0x (High bonus)
                // e.g. Factor=1.0, Trait=0.0 -> Mod = 0.5 + 0.0 = 0.5x
                Modifier = 0.5f + (TraitValue * InfluenceFactor * 1.5f);
            }
            else
            {
                // 负相关: 0.5 + ((1.0 - Trait) * Abs(Factor) * 1.5)
                // e.g. Factor=-1.0, Trait=1.0 (High N) -> Mod = 0.5 + (0.0 * 1.5) = 0.5x (Penalty)
                // e.g. Factor=-1.0, Trait=0.2 (Low N)  -> Mod = 0.5 + (0.8 * 1.5) = 1.7x (Bonus)
                // e.g. Factor=-1.0, Trait=0.0 (Very Low N) -> Mod = 0.5 + (1.0 * 1.5) = 2.0x (Max bonus)
                Modifier = 0.5f + ((1.0f - TraitValue) * FMath::Abs(InfluenceFactor) * 1.5f);
            }
            
            // 钳制到合理范围 / Clamp to reasonable range
            Modifier = FMath::Clamp(Modifier, 0.2f, 2.0f);
            
            // 应用修正
            // Apply Modifier
            float OldScore = FinalScore;
            FinalScore *= Modifier;
            
            // ✅ Always log PAM (not dependent on bLogDebug)
            UE_LOG(LogTemp, Warning, TEXT("    ↳ [PAM] %s: %.2f | Factor: %.1f -> Mod: %.2fx (%.2f -> %.2f)"),
                   *UEnum::GetValueAsString(Trait), TraitValue, InfluenceFactor, Modifier, OldScore, FinalScore);
        }
    }
    else
    {
        // ✅ Debug: Why PAM not applied
        if (PersonalityInfluence.Num() > 0 && !PersonalityComp)
        {
            UE_LOG(LogTemp, Error, TEXT("    ❌ [PAM] PersonalityComponent is NULL for %s!"), *ActionName);
        }
    }

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

    // 7. 应用 Directive 加成/限制 / Apply Directive Bonus/Restriction
    // ---------------------------------------------------------
    // Directive 既是"加成"也是"开关"：
    // - 匹配时：2.0x 分数加成（鼓励在正确时间做正确的事）
    // - 不匹配时：0.0x 分数（完全禁止）
    // Directive acts as both "bonus" and "gate":
    // - When matched: 2.0x score bonus (encourage doing the right thing at the right time)
    // - When not matched: 0.0x score (completely forbidden)
    
    if (DirectiveTag.IsValid())
    {
        float DirectiveMultiplier = 0.0f; // Default: forbidden
        
        // Find GoalComponent (check Controller first, then Pawn)
        UGoalComponent* GoalComp = nullptr;
        if (Controller)
        {
            GoalComp = Controller->FindComponentByClass<UGoalComponent>();
        }
        
        if (!GoalComp)
        {
            APawn* BotPawn = Controller ? Controller->GetPawn() : nullptr;
            if (BotPawn)
            {
                GoalComp = BotPawn->FindComponentByClass<UGoalComponent>();
            }
        }
        
        if (GoalComp)
        {
            FGameplayTag CurrentDirective = GoalComp->GetCurrentDirective();
            
            // 4.5. 指令封锁与加成 / Directive Blocking & Bonus
            if (CurrentDirective.IsValid())
            {
               // 如果有指令，检查是否匹配
               if (DirectiveTag.MatchesTag(CurrentDirective))
               {
                   // 匹配指令：给予加成 (x1.5)
                   DirectiveMultiplier = DirectiveMatchMultiplier;
                   if (bLogDebug) UE_LOG(LogTemp, Log, TEXT("      [Directive] 🎯 Matches Directive '%s' -> Multiplier x%.1f"), *CurrentDirective.ToString(), DirectiveMatchMultiplier);
               }
               else if (DirectiveTag.IsValid())
               {
                   // 不匹配指令，且该动作本身属于某种指令类型：给予惩罚 (x0.5)
                   // 例如：指令是 Social，但这动作是 Work，则降权
                   DirectiveMultiplier = DirectiveMismatchMultiplier;
                   if (bLogDebug) UE_LOG(LogTemp, Log, TEXT("      [Directive] ⛔ Mismatch Directive '%s' (Action is '%s') -> Multiplier x%.1f"), *CurrentDirective.ToString(), *DirectiveTag.ToString(), DirectiveMismatchMultiplier);
               }
            }
        }
        else if (bLogDebug)
        {
            UE_LOG(LogTemp, Warning, TEXT("      [Directive] ⚠️ GoalComponent not found, no directive multiplier applied"));
        }
        
        FinalScore *= DirectiveMultiplier;
        
        if (bLogDebug && DirectiveMultiplier > 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("      👉 AFTER Directive Multiplier: %.3f * %.2f = %.3f"), 
                   FinalScore / DirectiveMultiplier, DirectiveMultiplier, FinalScore);
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

float UUtilityActionBase::GetConsiderationValue(EUtilityInputType InputType, UNPCMentalState* State, AAIController* Controller, bool bLogDebug)
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
            // Boredom grows passively via MetabolismComponent, not controlled by LLM
            // So we read directly from State, not Interpolator
            return State ? State->Boredom : 0.0f;

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
                AActor* Food = Sensory->FindBestSmartObject(AINPCTags::Interaction_Eat);
                return Food ? 1.0f : 0.0f;
            }
            return 0.0f;
        }

        // ✅ 新增：检查是否有床附近
        case EUtilityInputType::HasBedNearby:
        {
            if (USensoryComponent* Sensory = Controller->FindComponentByClass<USensoryComponent>())
            {
                 // 使用 AINPCTags::Interaction_Rest
                AActor* Bed = Sensory->FindBestSmartObject(AINPCTags::Interaction_Rest);
                
                // 暂时强制打印调试，确认为什么找不到床
                if (State->Fatigue > 0.4f)
                {
                     // UE_LOG(LogTemp, Warning, TEXT("💤 [Debug] Checking Bed... Found: %s (Radius: 150m)"), Bed ? *Bed->GetName() : TEXT("NULL"));
                }
                
                if (Bed)
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
        case EUtilityInputType::Hunger:            return MentalStateNames::Hunger;
        case EUtilityInputType::Fatigue:           return MentalStateNames::Fatigue;
        case EUtilityInputType::PerceivedThreat:   return MentalStateNames::Threat;
        case EUtilityInputType::Loneliness:        return MentalStateNames::Loneliness;
        case EUtilityInputType::Indignity:         return MentalStateNames::Indignity;
        case EUtilityInputType::Boredom:           return MentalStateNames::Boredom;
        
        // 环境变量 (Environment variables)
        case EUtilityInputType::SelfHealth:        return TEXT("SelfHealth");
        case EUtilityInputType::TargetHealth:      return TEXT("TargetHealth");
        case EUtilityInputType::DistanceToTarget:  return TEXT("DistanceToTarget");
        case EUtilityInputType::AmmoCount:         return TEXT("AmmoCount");
        case EUtilityInputType::HasCover:          return TEXT("HasCover");
        case EUtilityInputType::IsTargetPlayer:    return TEXT("IsTargetPlayer");
        case EUtilityInputType::HasAttackTarget:   return TEXT("HasAttackTarget");
        case EUtilityInputType::HasEnemyNearby:    return TEXT("HasEnemyNearby");
        case EUtilityInputType::HasFriendlyNearby: return TEXT("HasFriendlyNearby");
        case EUtilityInputType::HasFoodNearby:     return TEXT("HasFoodNearby");
        case EUtilityInputType::HasBedNearby:      return TEXT("HasBedNearby");
        
        default:
            return TEXT("Unknown");
    }
}