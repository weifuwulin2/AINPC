#include "Base/UtilityActionBase.h"

#include "AINPC.h"
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
#include "Components/FactionReputationComponent.h"
#include "Utilities/FactionHelpers.h" // ✅ Faction utilities

#include "Config/AINPCSettings.h"


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

    // Target Selection Config
    bNeedsTarget = Config.bNeedsTarget;
    TargetContext = Config.TargetContext;
    TargetConfigOverride = Config.TargetConfigOverride;

    // ✅ Transition System fields
    Priority = Config.Priority;
    CommitmentTime = Config.CommitmentTime;
    ExitConditions = Config.ExitConditions;
    
    // Debug Log (Simplified)
    // UE_LOG(LogAINPCUtility, Warning, TEXT("[InitFromConfig] %s: Priority=%d, Commitment=%.1fs"), 
    //        *Config.ActionName, (int32)Priority, CommitmentTime);

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
    // =========================================================
    // Step 0: Safety Checks
    // =========================================================
    if (!Controller) return 0.0f;
    
    APawn* ControlledPawn = Controller->GetPawn();
    if (!ControlledPawn || !IsValid(ControlledPawn) || ControlledPawn->IsPendingKillPending())
    {
        if (bLogDebug) UE_LOG(LogAINPCUtility, Warning, TEXT("    [%s] ❌ Score=0: Pawn Invalid/Dead"), *ActionName);
        return 0.0f;
    }

    // No Considerations = Just return BaseReward (simple actions)
    if (Considerations.Num() == 0) 
    {
        if (bLogDebug) UE_LOG(LogAINPCUtility, Warning, TEXT("    [%s] ⚠️ No Considerations, returning BaseReward=%.2f"), *ActionName, BaseReward);
        return BaseReward;
    }

    // =========================================================
    // Step 1: Cooldown Check (blocks scoring entirely if on cooldown)
    // =========================================================
    float CurrentTime = Controller->GetWorld() ? Controller->GetWorld()->GetTimeSeconds() : 0.0f;
    float CooldownResult = CheckCooldown(CurrentTime, bLogDebug);
    if (CooldownResult <= 0.0f) return 0.0f;

    // =========================================================
    // Step 2: Get PersonalityComponent (needed for multiple steps)
    // =========================================================
    UPersonalityComponent* PersonalityComp = nullptr;
    if (AUtilityAIController* UtilityController = Cast<AUtilityAIController>(Controller))
    {
        PersonalityComp = UtilityController->PersonalityComp;
    }
    if (!PersonalityComp && Controller->GetPawn())
    {
        PersonalityComp = Controller->GetPawn()->FindComponentByClass<UPersonalityComponent>();
    }

    // =========================================================
    // Step 3: Calculate Considerations (Motivation + Context)
    // =========================================================
    float MotivationSum = 0.0f;
    float ContextProduct = 1.0f;
    CalculateConsiderations(MentalState, Controller, PersonalityComp, MotivationSum, ContextProduct, bLogDebug);
    
    // Early exit if Context product is zero (necessary conditions not met)
    if (ContextProduct <= UE_KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }

    // =========================================================
    // Step 4: Calculate Additive Bonuses (Intention + Schedule)
    // =========================================================
    float IntentionBonus = CalculateIntentionBonus(MentalState, bLogDebug);
    float ScheduleBonus = CalculateScheduleBonus(Controller, bLogDebug);

    // =========================================================
    // Step 5: Base Formula
    // Intention is multiplicative (amplifier, not engine) — LLM can only boost existing needs, not create them.
    // Schedule remains additive (time-based obligation can create motivation).
    // Score = BaseReward × (MotivationSum + ScheduleBonus) × (1 + IntentionBonus) × ContextProduct
    // =========================================================
    float EffectiveMotivation = (MotivationSum + ScheduleBonus) * (1.0f + IntentionBonus);
    float FinalScore = BaseReward * EffectiveMotivation * ContextProduct;

    // =========================================================
    // Step 6: Apply Multiplier Modifiers (PAM, Emotion, Directive)
    // =========================================================
    FinalScore = ApplyPersonalityModifier(FinalScore, PersonalityComp, bLogDebug);
    FinalScore = ApplyEmotionMatrix(FinalScore, Controller, bLogDebug);
    FinalScore = ApplyDirectiveModifier(FinalScore, Controller, bLogDebug);

    // =========================================================
    // Debug Summary
    // =========================================================
    if (bLogDebug)
    {
        UE_LOG(LogAINPCUtility, Warning, TEXT("    [%s] 📊 Calculation Summary:"), *ActionName);
        UE_LOG(LogAINPCUtility, Log, TEXT("      • Base Reward: %.2f"), BaseReward);
        UE_LOG(LogAINPCUtility, Log, TEXT("      • Motivation Sum: %.2f"), MotivationSum);
        UE_LOG(LogAINPCUtility, Log, TEXT("      • Intention Multiplier: x%.2f %s"), 1.0f + IntentionBonus, IntentionBonus > 0 ? TEXT("(✅ APPLIED)") : TEXT(""));
        UE_LOG(LogAINPCUtility, Log, TEXT("      • Schedule Bonus: +%.2f %s"), ScheduleBonus, ScheduleBonus > 0 ? TEXT("(✅ APPLIED)") : TEXT(""));
        UE_LOG(LogAINPCUtility, Log, TEXT("      • Context Product: %.2f"), ContextProduct);
        UE_LOG(LogAINPCUtility, Warning, TEXT("      👉 FINAL SCORE = %.3f"), FinalScore);
    }

    return FinalScore;
}

// =========================================================
// Helper: Check Cooldown
// =========================================================
float UUtilityActionBase::CheckCooldown(float CurrentTime, bool bLogDebug) const
{
    if (CooldownTime <= 0.0f) return 1.0f; // No cooldown configured
    
    // Check if this is the currently running action (don't penalize self)
    // Note: This requires access to UtilityAIComponent, which we check via a simple time comparison
    if (CurrentTime - LastExecutedTime < CooldownTime)
    {
        if (bLogDebug)
        {
            UE_LOG(LogAINPCUtility, Warning, TEXT("    [%s] ❌ Score=0: Cooldown (%.1fs left)"), 
                   *ActionName, CooldownTime - (CurrentTime - LastExecutedTime));
        }
        return 0.0f;
    }
    return 1.0f;
}

// =========================================================
// Helper: Calculate Considerations (Motivation + Context)
// =========================================================
void UUtilityActionBase::CalculateConsiderations(
    UNPCMentalState* MentalState, 
    AAIController* Controller, 
    UPersonalityComponent* PersonalityComp,
    float& OutMotivationSum, 
    float& OutContextProduct, 
    bool bLogDebug
)
{
    OutMotivationSum = 0.0f;
    OutContextProduct = 1.0f;

    if (bLogDebug)
    {
        UE_LOG(LogAINPCUtility, Warning, TEXT("    [%s] Starting calculation: BaseReward=%.2f"), *ActionName, BaseReward);
    }

    for (int32 i = 0; i < Considerations.Num(); ++i)
    {
        const FUtilityConsideration& Factor = Considerations[i];
        
        // A. Get raw value (0~1 or actual value)
        float RawValue = GetConsiderationValue(Factor.InputType, MentalState, Controller, bLogDebug);

        // B. Apply response curve
        float CurveValue = RawValue;
        
        if (Factor.ResponseCurve)
        {
            CurveValue = Factor.ResponseCurve->GetFloatValue(RawValue);
        }
        else
        {
            switch (Factor.CurveType)
            {
                case EUtilityCurveType::Linear:
                    CurveValue = RawValue;
                    break;
                case EUtilityCurveType::Quadratic:
                    CurveValue = RawValue * RawValue;
                    break;
                case EUtilityCurveType::InverseQuadratic:
                    CurveValue = 1.0f - (1.0f - RawValue) * (1.0f - RawValue);
                    break;
                case EUtilityCurveType::Logistic:
                    CurveValue = 1.0f / (1.0f + FMath::Exp(-10.0f * (RawValue - 0.5f)));
                    break;
                case EUtilityCurveType::Step:
                    CurveValue = (RawValue >= 0.5f) ? 1.0f : 0.0f;
                    break;
                case EUtilityCurveType::TargetThreshold:
                    CurveValue = (RawValue >= 0.1f) ? 1.0f : 0.0f;
                    break;
                case EUtilityCurveType::Inverse:
                    CurveValue = 1.0f - RawValue;
                    break;
                default:
                    CurveValue = RawValue;
                    break;
            }
        }
        
        CurveValue = FMath::Clamp(CurveValue, 0.0f, 1.0f);
        float EffectiveValue = CurveValue;

        if (Factor.ConsiderationType == EConsiderationType::Motivation)
        {
            // Motivation: Additive sum with personality weight
            float PersonalityWeight = 1.0f;
            if (PersonalityComp)
            {
                FString VariableName = GetVariableNameFromInputType(Factor.InputType);
                PersonalityWeight = PersonalityComp->GetWeightForVariable(VariableName);
            }

            float MotivationScore = EffectiveValue * PersonalityWeight;
            OutMotivationSum += MotivationScore;
            
            if (bLogDebug)
            {
                UE_LOG(LogAINPCUtility, Warning, TEXT("      [Motivation %d] %s: Raw=%.3f × Weight=%.3f = %.3f (Sum=%.3f)"), 
                       i, *GetVariableNameFromInputType(Factor.InputType), 
                       EffectiveValue, PersonalityWeight, MotivationScore, OutMotivationSum);
            }
        }
        else // EConsiderationType::Context
        {
            // Context: Multiplicative (necessary conditions)
            float OldProduct = OutContextProduct;
            OutContextProduct *= EffectiveValue;
            
            if (bLogDebug)
            {
                UE_LOG(LogAINPCUtility, Warning, TEXT("      [Context %d] %s: %.3f × %.3f = %.3f"), 
                       i, *GetVariableNameFromInputType(Factor.InputType), 
                       OldProduct, EffectiveValue, OutContextProduct);
            }

            // Early exit optimization
            if (OutContextProduct <= UE_KINDA_SMALL_NUMBER)
            {
                if (bLogDebug)
                {
                    UE_LOG(LogAINPCUtility, Error, TEXT("      ⛔ ABORTING: Context '%s' is 0!"), 
                           *GetVariableNameFromInputType(Factor.InputType));
                }
                return;
            }
        }
    }
}

// =========================================================
// Helper: Calculate Intention Bonus (LLM Integration)
// =========================================================
float UUtilityActionBase::CalculateIntentionBonus(UNPCMentalState* MentalState, bool bLogDebug) const
{
    if (!MentalState || !IntentionTag.IsValid()) return 0.0f;
    
    FString CurrentIntention = MentalState->Intention;
    if (CurrentIntention.IsEmpty()) return 0.0f;
    
    // Construct expected Tag name: Intention.<String>
    FString TagNameToCheck = TEXT("Intention.") + CurrentIntention;
    
    if (IntentionTag.ToString().Equals(TagNameToCheck, ESearchCase::IgnoreCase))
    {
        float Bonus = UAINPCSettings::Get()->IntentionMatchBonus;
        
        if (bLogDebug)
        {
            UE_LOG(LogAINPCUtility, Warning, TEXT("      [Intention] 🧠 LLM MATCH! ActionTag:'%s' matches MentalState:'%s'. Bonus:+%.2f"), 
                   *IntentionTag.ToString(), *MentalState->Intention, Bonus);
        }
        return Bonus;
    }
    
    return 0.0f;
}

// =========================================================
// Helper: Calculate Schedule Bonus (GoalComponent Integration)
// =========================================================
float UUtilityActionBase::CalculateScheduleBonus(AAIController* Controller, bool bLogDebug) const
{
    if (!ActivityTag.IsValid() || !Controller) return 0.0f;
    
    // Find GoalComponent
    UGoalComponent* GoalComp = Controller->FindComponentByClass<UGoalComponent>();
    if (!GoalComp && Controller->GetPawn())
    {
        GoalComp = Controller->GetPawn()->FindComponentByClass<UGoalComponent>();
    }
    
    if (!GoalComp) return 0.0f;
    
    FGameplayTag ScheduledActivity = GoalComp->GetScheduledActivity();
    if (ScheduledActivity.IsValid() && ScheduledActivity.MatchesTag(ActivityTag))
    {
        if (bLogDebug)
        {
            UE_LOG(LogAINPCUtility, Warning, TEXT("      [Schedule] 📅 SCHEDULE MATCH! ActivityTag:'%s' matches Schedule:'%s'. Bonus:+1.0"), 
                   *ActivityTag.ToString(), *ScheduledActivity.ToString());
        }
        return 1.0f;
    }
    
    return 0.0f;
}

// =========================================================
// Helper: Apply Personality Action Modifier (PAM)
// =========================================================
float UUtilityActionBase::ApplyPersonalityModifier(float Score, UPersonalityComponent* PersonalityComp, bool bLogDebug) const
{
    if (!PersonalityComp || PersonalityInfluence.Num() == 0) return Score;
    
    float ModifiedScore = Score;
    
    for (const auto& Pair : PersonalityInfluence)
    {
        EOCEANTrait Trait = Pair.Key;
        float InfluenceFactor = Pair.Value;
        
        float TraitValue = PersonalityComp->GetTraitValue(Trait);
        float Modifier = 1.0f;
        
        if (InfluenceFactor > 0)
        {
            // Positive correlation: Higher trait = higher score
            Modifier = 0.5f + (TraitValue * InfluenceFactor * 1.5f);
        }
        else
        {
            // Negative correlation: Lower trait = higher score
            Modifier = 0.5f + ((1.0f - TraitValue) * FMath::Abs(InfluenceFactor) * 1.5f);
        }
        
        const UAINPCSettings* Settings = UAINPCSettings::Get();
        Modifier = FMath::Clamp(Modifier, Settings->PAMModifierMin, Settings->PAMModifierMax);
        
        ModifiedScore *= Modifier;
    }
    
    return ModifiedScore;
}

// =========================================================
// Helper: Apply Emotion Matrix Multiplier
// =========================================================
float UUtilityActionBase::ApplyEmotionMatrix(float Score, AAIController* Controller, bool bLogDebug) const
{
    AUtilityAIController* UtilityController = Cast<AUtilityAIController>(Controller);
    if (!UtilityController || !UtilityController->EmotionMatrixTable) return Score;
    
    // Get current emotion name
    FString EmotionName = UEnum::GetValueAsString(UtilityController->CurrentEmotion);
    FString CleanEmotionName;
    EmotionName.Split(TEXT("::"), nullptr, &CleanEmotionName);
    FName RowName = FName(*CleanEmotionName);

    static const FString ContextString(TEXT("UtilityActionBase::ApplyEmotionMatrix"));
    FEmotionMatrixRow* MatrixRow = UtilityController->EmotionMatrixTable->FindRow<FEmotionMatrixRow>(RowName, ContextString);
    
    if (MatrixRow)
    {
        float EmotionMultiplier = MatrixRow->GetMultiplier(ActivityTag);
        
        if (bLogDebug && FMath::Abs(EmotionMultiplier - 1.0f) > KINDA_SMALL_NUMBER)
        {
            UE_LOG(LogAINPCUtility, Warning, TEXT("      [Emotion] 🎭 Multiplier Applied: %s->%s (x%.2f)"), 
                   *CleanEmotionName, *ActivityTag.ToString(), EmotionMultiplier);
        }
        
        return Score * EmotionMultiplier;
    }
    
    return Score;
}

// =========================================================
// Helper: Apply Directive Modifier (GoalComponent Integration)
// =========================================================
float UUtilityActionBase::ApplyDirectiveModifier(float Score, AAIController* Controller, bool bLogDebug) const
{
    if (!DirectiveTag.IsValid() || !Controller) return Score;
    
    // Find GoalComponent
    UGoalComponent* GoalComp = Controller->FindComponentByClass<UGoalComponent>();
    if (!GoalComp && Controller->GetPawn())
    {
        GoalComp = Controller->GetPawn()->FindComponentByClass<UGoalComponent>();
    }
    
    if (!GoalComp)
    {
        if (bLogDebug)
        {
            UE_LOG(LogAINPCUtility, Warning, TEXT("      [Directive] ⚠️ GoalComponent not found"));
        }
        return Score;
    }
    
    FGameplayTag CurrentDirective = GoalComp->GetCurrentDirective();
    if (!CurrentDirective.IsValid()) return Score;
    
    float DirectiveMultiplier = 0.0f;
    
    if (DirectiveTag.MatchesTag(CurrentDirective))
    {
        // Match: Apply bonus
        DirectiveMultiplier = UAINPCSettings::Get()->DirectiveMatchMultiplier;
        
        // Narrative boost for InScene
        if (APawn* P = Controller->GetPawn())
        {
            if (P->ActorHasTag("Status.InScene"))
            {
                float NarrativeBoost = UAINPCSettings::Get()->NarrativeDirectiveBoost;
                DirectiveMultiplier *= NarrativeBoost;
                
                if (bLogDebug)
                {
                    UE_LOG(LogAINPCUtility, Warning, TEXT("      [Scene] 🎬 IN SCENE: Directive Multiplier Boosted x%.1f"), NarrativeBoost);
                }
            }
        }
        
        if (bLogDebug)
        {
            UE_LOG(LogAINPCUtility, Log, TEXT("      [Directive] 🎯 Matches '%s' -> Multiplier x%.1f"), *CurrentDirective.ToString(), DirectiveMultiplier);
        }
    }
    else
    {
        // Mismatch: Apply penalty
        DirectiveMultiplier = UAINPCSettings::Get()->DirectiveMismatchMultiplier;
        
        if (bLogDebug)
        {
            UE_LOG(LogAINPCUtility, Log, TEXT("      [Directive] ⛔ Mismatch '%s' (Action is '%s') -> Multiplier x%.1f"), 
                   *CurrentDirective.ToString(), *DirectiveTag.ToString(), DirectiveMultiplier);
        }
    }
    
    return Score * DirectiveMultiplier;
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
            
            // ✅ Use TargetSelectionSubsystem to check strictly for perceived/squad-aware targets
            // Adapting to align with Action_Attack's target availability
            if (UTargetSelectionSubsystem* TargetSys = Controller->GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
            {
                FTargetSelectionConfig Config;
                Config.MaxDistance = 6000.0f; // Match the generous combat range
                
                // ✅ FIX: FactionReputationComponent lives on the Pawn, NOT the Controller.
                // Using Controller-> returned nullptr, skipping the entire attitude check
                // and making ALL actors (including friendly Player) pass as valid combat targets.
                UFactionReputationComponent* FactionComp = BotPawn->FindComponentByClass<UFactionReputationComponent>();
                TArray<AActor*> Candidates = TargetSys->GetTargetCandidates(Controller, ETargetSelectionContext::Combat, Config, BotPawn, FactionComp);
                


                if (Candidates.Num() > 0)
                {
                     // Found at least one valid hostile target in range/perception
                     return 1.0f;
                }
            }
            
            return 0.0f; // 没有敌人
        }

        // ✅ 新增：检查附近是否有友军 (非 Enemy)
        case EUtilityInputType::HasFriendlyNearby:
        {
            if (!BotPawn || !Controller->GetWorld()) return 0.0f;

            // 获取自己的阵营
            FName MyFactionID = UFactionReputationComponent::GetFactionID(BotPawn);
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

                // Check hostility using FactionHelpers
                bool bIsHostile = FactionHelpers::AreActorsHostile(BotPawn, Actor);

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
                     // UE_LOG(LogAINPCUtility, Warning, TEXT("💤 [Debug] Checking Bed... Found: %s (Radius: 150m)"), Bed ? *Bed->GetName() : TEXT("NULL"));
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
        case EUtilityInputType::GoalDirectiveMatch:return TEXT("GoalDirectiveMatch");

        default:
            return TEXT("Unknown");
    }
}

bool UUtilityActionBase::ShouldExit(AAIController* Controller) const
{
    // 默认行为：允许随时退出
    // Default behavior: Allow exit at any time
    return true;
}

// =========================================================
// Transition System: IsCommitted
// =========================================================
bool UUtilityActionBase::IsCommitted(float CurrentTime) const
{
    if (CommitmentTime <= 0.0f) return false;
    
    float ElapsedSinceCommit = CurrentTime - CommitmentStartTime;
    return ElapsedSinceCommit < CommitmentTime;
}

// =========================================================
// Transition System: CheckExitConditions (Data-Driven)
// =========================================================
bool UUtilityActionBase::CheckExitConditions(UNPCMentalState* MentalState, AAIController* Controller) const
{
    if (ExitConditions.Num() == 0) return false;

    for (const FActionExitCondition& Condition : ExitConditions)
    {
        // Skip if waiting for duration and duration hasn't elapsed
        if (Condition.bWaitForDuration && Controller && Controller->GetWorld())
        {
            float Elapsed = Controller->GetWorld()->GetTimeSeconds() - ActionStartTime;
            // Use ActionDuration from base class (requires it to be set)
            // For simplicity, use CommitmentTime as the "minimum duration" if ActionDuration is 0
            float MinDuration = CommitmentTime > 0.0f ? CommitmentTime : 0.0f;
            if (Elapsed < MinDuration) continue;
        }
        
        // Get the current value of the variable
        float CurrentValue = const_cast<UUtilityActionBase*>(this)->GetConsiderationValue(Condition.Variable, MentalState, Controller, false);
        
        // Evaluate condition
        bool bConditionMet = false;
        switch (Condition.Operator)
        {
            case EComparisonOperator::LessThan:
                bConditionMet = CurrentValue < Condition.Threshold;
                break;
            case EComparisonOperator::LessThanOrEqual:
                bConditionMet = CurrentValue <= Condition.Threshold;
                break;
            case EComparisonOperator::GreaterThan:
                bConditionMet = CurrentValue > Condition.Threshold;
                break;
            case EComparisonOperator::GreaterThanOrEqual:
                bConditionMet = CurrentValue >= Condition.Threshold;
                break;
            case EComparisonOperator::Equals:
                bConditionMet = FMath::IsNearlyEqual(CurrentValue, Condition.Threshold, 0.01f);
                break;
        }
        
        if (bConditionMet)
        {
            UE_LOG(LogAINPCUtility, Log, TEXT("[%s] Exit condition met: %s %s %.2f (Current: %.2f)"),
                   *ActionName,
                   *UEnum::GetValueAsString(Condition.Variable),
                   *UEnum::GetValueAsString(Condition.Operator),
                   Condition.Threshold,
                   CurrentValue);
            return true;
        }
    }
    
    return false; // No conditions met
}

// =========================================================
// Base Enter_Implementation (sets ActionStartTime, CommitmentStartTime)
// =========================================================
void UUtilityActionBase::Enter_Implementation(AAIController* Controller)
{
    if (Controller && Controller->GetWorld())
    {
        float CurrentTime = Controller->GetWorld()->GetTimeSeconds();
        ActionStartTime = CurrentTime;
        CommitmentStartTime = CurrentTime;
    }
}