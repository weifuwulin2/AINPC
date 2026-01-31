#include "Components/UtilityAIComponent.h"
#include "Controller/UtilityAIController.h"
#include "UtilityAI/UNPCMentalState.h"
#include "Components/PersonalityComponent.h"
#include "Components/CognitionComponent.h"
#include "LLM/LLMCommunicator.h"
#include "Actions/Action_SmartObject.h"
#include "AINPC.h"

UUtilityAIComponent::UUtilityAIComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // 这里的 Tick 间隔决定了 AI 反应速度，设为 0.1f (每秒10次) 可以优化性能
    PrimaryComponentTick.TickInterval = 0.2f; 
}

void UUtilityAIComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerController = Cast<AUtilityAIController>(GetOwner());
    if (!OwnerController)
    {
        UTILITY_LOG(Error, "UtilityComp must be attached to UtilityAIController!");
        return;
    }

    LoadActionsFromTable();

    // ✅ Startup Delay: Wait 1.0s before starting AI Logic to prevent initialization race conditions
    // (e.g. Leader spawning before SmartObjects are ready, or before Slaves spawn)
    SetComponentTickEnabled(false);
    
    FTimerHandle WarmupTimerHandle;
    TWeakObjectPtr<UUtilityAIComponent> WeakThis(this);
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(WarmupTimerHandle, [WeakThis]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->SetComponentTickEnabled(true);
                // Log via the static logger macro if possible, or simple UE_LOG
                UE_LOG(LogTemp, Log, TEXT("[%s] 🧠 AI Warmup Complete (1.0s). Logic Started."), *WeakThis->GetName());
            }
        }, 1.0f, false);
    }
}

// ✅ Explicit Setter for Initialization Flow
void UUtilityAIComponent::SetProfession(FName NewProfessionID)
{
    if (CurrentProfessionID == NewProfessionID && AvailableActions.Num() > 0)
    {
        return; // Already set and loaded
    }

    CurrentProfessionID = NewProfessionID;
    UTILITY_LOG(Log, "SetProfession called: %s. Reloading actions...", *CurrentProfessionID.ToString());
    
    // Reload actions with new profession filter
    LoadActionsFromTable();
    
    // Force debug log for next tick to see why actions are scored/rejected
    bPendingDebugLog = true;
}

void UUtilityAIComponent::LoadActionsFromTable()
{
    if (!ActionDataTable) return;

    // Clear existing actions (important for re-initialization)
    AvailableActions.Empty();

    static const FString ContextString(TEXT("UtilityAI Actions Load"));
    TArray<FUtilityActionConfig*> Rows;
    ActionDataTable->GetAllRows(ContextString, Rows);

    // Note: CurrentProfessionID is now set via SetProfession() by NPCDefinitionComponent
    // Fallback: If None (e.g. old blueprint setup), try to assume from Personality (Legacy)
    if (CurrentProfessionID.IsNone() && OwnerController && OwnerController->PersonalityComp)
    {
        // Warn about legacy fallback
        // UE_LOG(LogTemp, Warning, TEXT("[UtilityComp] ProfessionID not set! Falling back to PersonalityID. Please update initialization flow."));
        // CurrentProfessionID = OwnerController->PersonalityComp->PersonalityID;
    }

    UTILITY_LOG(Log, "Loading Actions for Profession: %s", *CurrentProfessionID.ToString());

    for (FUtilityActionConfig* Row : Rows)
    {
        if (Row && Row->ActionClass)
        {
            // --- 职业过滤逻辑 (Profession Filtering) ---
            // Check if NPC's profession is in the allowed list
            if (Row->RequiredProfessionIDs.Num() > 0)
            {
                if (!Row->RequiredProfessionIDs.Contains(CurrentProfessionID))
                {
                    // Build allowed list string for logging
                    FString AllowedList;
                    for (const FName& ProfID : Row->RequiredProfessionIDs)
                    {
                        if (!AllowedList.IsEmpty()) AllowedList += ", ";
                        AllowedList += ProfID.ToString();
                    }
                    
                    UTILITY_LOG(Warning, "🚫 Skipped Action %s (Requires: [%s], Current: %s)", 
                           *Row->ActionName, *AllowedList, *CurrentProfessionID.ToString());
                    continue;
                }
            }
            
            // 注意：Action 的 Outer 设为 this (Component)，方便管理生命周期
            UUtilityActionBase* NewAction = NewObject<UUtilityActionBase>(this, Row->ActionClass);
            NewAction->InitFromConfig(*Row);

            // ✅ 如果是 Action_SmartObject，传递动画配置
            // If it's Action_SmartObject, pass animation configuration
            if (UAction_SmartObject* SmartObjectAction = Cast<UAction_SmartObject>(NewAction))
            {
                SmartObjectAction->InteractionMontage = Row->InteractionMontage;
                SmartObjectAction->bLoopAnimation = Row->bLoopAnimation;
                SmartObjectAction->ActionDuration = Row->ActionDuration;
                
            }

            AvailableActions.Add(NewAction);
            UTILITY_LOG(Warning, "+ Loaded Action: %s", *Row->ActionName);
        }
    }
}

void UUtilityAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerController || !OwnerController->MentalState) return;

    // ✅ DISTANCE LOD SYSTEM - Dynamic Performance Optimization
    // 根据到玩家距离动态调整更新频率
    AAIController* AIController = Cast<AAIController>(GetOwner());
    if (AIController && AIController->GetPawn())
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC && PC->GetPawn())
        {
            float Distance = FVector::Dist(AIController->GetPawn()->GetActorLocation(), 
                                          PC->GetPawn()->GetActorLocation());
            
            // 距离分级：
            // > 5000单位：休眠（完全跳过更新）
            if (Distance > 5000.0f)
            {
                return; // Save ~0.09ms per NPC
            }
            
            // 2000-5000单位：降级（每2帧更新一次）
            if (Distance > 2000.0f)
            {
                if (GFrameCounter % 2 != 0) return; // 50% update rate
            }
            
            // < 2000单位：全速更新（正常逻辑）
        }
    }

    // 1. 执行决策逻辑 (Evaluate)
    EvaluateAndDecide();

    // 2. 驱动当前动作 (Execute)
    if (CurrentAction)
    {
        CurrentAction->Execute(OwnerController);
    }

    // 🔍 定期打印当前动作状态（每 5 秒）
    // Periodic debug logging (every 5 seconds, per-instance)
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastStatusLogTime > 5.0f)
    {
        if (CurrentAction)
        {
            UTILITY_LOG(Warning, "%s - Current Action: %s", 
                   *OwnerController->GetName(), *CurrentAction->ActionName);
        }
        else
        {
            UTILITY_LOG(Warning, "%s - No Action", 
                   *OwnerController->GetName());
        }
        LastStatusLogTime = CurrentTime;
        
        // Force a detailed log on next evaluation
        bPendingDebugLog = true;
    }
}

void UUtilityAIComponent::EvaluateAndDecide()
{
    if (AvailableActions.Num() == 0) return;

    UUtilityActionBase* BestAction = nullptr;
    float BestScore = -1.0f;
    
    UUtilityActionBase* RunnerUpAction = nullptr;
    float RunnerUpScore = -1.0f;

    UNPCMentalState* State = OwnerController->MentalState;
    
    // 获取 PersonalityID 用于日志
    FString PersonalityID = "Unknown";
    if (OwnerController && OwnerController->PersonalityComp)
    {
        PersonalityID = OwnerController->PersonalityComp->PersonalityID.ToString();
    }

    // 🔍 调试：打印当前 MentalState
    // 只有在收到刺激请求时才打印，或者你也可以保留定时的 Summary
    // Only log when requested (by stimulus), or maintain periodic summary if preferred.
    // User requested "Only log calculation after stimulus", so we prioritize bPendingDebugLog.
    bool bShouldLog = bPendingDebugLog;
    
    // 如果想要保留偶尔的心跳日志，可以取消下面这行的注释，但 CalculateScore 的详细日志只会由 bPendingDebugLog 控制
    // if (!bShouldLog) { static float LastLog = 0; if (GetWorld()->GetTimeSeconds() - LastLog > 5.0f) { bShouldLog = true; LastLog = GetWorld()->GetTimeSeconds(); } }

    if (bShouldLog)
    {
        UTILITY_LOG(Warning, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        UTILITY_LOG(Warning, "[%s] Evaluating Actions (Count: %d) [Triggered]", *PersonalityID, AvailableActions.Num());
        
        // 显示当前正在执行的 Action
        if (CurrentAction)
        {
            UTILITY_LOG(Log, "[%s] Currently Running: %s", *PersonalityID, *CurrentAction->ActionName);
        }
        else
        {
            UTILITY_LOG(Log, "[%s] Currently Running: None", *PersonalityID);
        }
    }

    // --- 遍历打分 ---
    float CurrentActionScore = 0.0f; // Track score of the currently active action

    for (UUtilityActionBase* Action : AvailableActions)
    {
        // 调用 Action 自身的算分逻辑
        // Pass bShouldLog (which is true if bPendingDebugLog is true) to enable detailed calculation logs
        float Score = Action->CalculateScore(State, OwnerController, bShouldLog);
        
        // Capture Current Action Score
        if (Action == CurrentAction)
        {
            CurrentActionScore = Score;
        }

        // 🔍 调试：只在触发时打印分数
        // Only log action scores when triggered
        if (bShouldLog)
        {
            UTILITY_LOG(Log, "  [%s|%s] Score: %.3f (BaseReward: %.2f, Considerations: %d)", 
                   *PersonalityID, *Action->ActionName, Score, Action->BaseReward, Action->Considerations.Num());
        }

        // 惯性奖励 (Momentum) - 百分比加成而非固定值
        // Percentage-based bonus instead of fixed value
        // 这样可以避免在极端分数时的不均衡影响
        if (Action == CurrentAction && Score > 0.0f)
        {
            float OldScore = Score;
            Score *= (1.0f + Action->InertiaBonus * 0.2f); // 20% of InertiaBonus value
            
            // Update the tracked score to include inertia
            CurrentActionScore = Score;
            
            if (bShouldLog)
            {
                UTILITY_LOG(Log, "    ↳ [%s] Inertia Bonus: +%.1f%% (%.3f -> %.3f)", 
                       *PersonalityID, Action->InertiaBonus * 20.0f, OldScore, Score);
            }
        }

        // 🧠 LLM 意图加成 (Intention Guidance)
        // LLM 提供"建议"，Utility AI 依然做最终决策
        // LLM provides "suggestion", Utility AI still makes final decision
        if (State && !State->ToStruct().Intention.IsEmpty())
        {
            FString LLMIntention = State->ToStruct().Intention;
            
            // 检查 Action 名称是否包含 Intention 关键词
            // Check if Action name contains the Intention keyword
            // 例如：Intention="Attack" 匹配 ActionName="Test_Attack"
            if (Action->ActionName.Contains(LLMIntention))
            {
                float OldScore = Score;
                float IntentionBonus = 0.3f; // 可配置的加成值
                Score += IntentionBonus;
                
                // Update tracked score if this is current action
                if (Action == CurrentAction) CurrentActionScore = Score;
                
                if (bShouldLog)
                {
                    UTILITY_LOG(Log, "    ↳ [%s] 🧠 LLM Intention Bonus: +%.2f (Intention: %s, %.3f -> %.3f)", 
                           *PersonalityID, IntentionBonus, *LLMIntention, OldScore, Score);
                }
            }
        }

        if (Score > BestScore)
        {
            // Demote current Best to RunnerUp
            RunnerUpScore = BestScore;
            RunnerUpAction = BestAction;

            BestScore = Score;
            BestAction = Action;
        }
        else if (Score > RunnerUpScore)
        {
            // New RunnerUp found
            RunnerUpScore = Score;
            RunnerUpAction = Action;
        }
    }

    // 🧠 Report Decision Context (Conflict) to Cognition
    if (OwnerController && BestAction && RunnerUpAction && BestScore > 0.0f)
    {
        if (UCognitionComponent* CognitionComp = OwnerController->FindComponentByClass<UCognitionComponent>())
        {
            float ConflictMetric = (BestScore - RunnerUpScore) / BestScore;
            CognitionComp->ReportDecisionContext(BestAction->ActionName, RunnerUpAction->ActionName, ConflictMetric);
        }
    }

    if (bShouldLog)
    {
        if (BestAction)
        {
            AINPC_LOG(Warning, "[%s] 🏆 Best Action: %s (Score: %.3f)", 
                   *PersonalityID, *BestAction->ActionName, BestScore);
        }
        else
        {
            AINPC_LOG_ERROR("[%s] ⚠️ No valid action found!", *PersonalityID);
        }
        UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
        // Reset the request flag
        bPendingDebugLog = false;
    }

    // --- 状态切换 ---
    // 只有当最佳动作不同于当前动作时才考虑切换
    if (BestAction && BestAction != CurrentAction)
    {
        // ✅ Use centralized Transition System
        // Pass CurrentActionScore
        if (!CanTransition(CurrentAction, BestAction, BestScore, CurrentActionScore))
        {
            // Transition denied - stay on current action
            return;
        }
        
        // 退出旧的
        if (CurrentAction)
        {
            CurrentAction->Exit(OwnerController);
        }

        // 进入新的
        CurrentAction = BestAction;
        CurrentAction->Enter(OwnerController);
        
        // Store score for inertia comparison
        CurrentAction->LastScore = BestScore;
        
        // 记录时间用于冷却计算
        CurrentAction->MarkExecutionTime(GetWorld()->GetTimeSeconds());

        // 打印切换日志
        AINPC_LOG(Warning, "[%s] ✅ Switch Action: %s (Score: %.2f, Priority: %d)", 
                  *PersonalityID, *CurrentAction->ActionName, BestScore, (int32)CurrentAction->Priority);
    }
}

// =========================================================
// Transition System: CanTransition ("The Constitution")
// =========================================================
bool UUtilityAIComponent::CanTransition(UUtilityActionBase* Current, UUtilityActionBase* Candidate, float CandidateScore, float CurrentScore)
{
    if (!Candidate) return false;
    if (!Current) return true; // No current action = always allow

    float CurrentTime = GetWorld()->GetTimeSeconds();

    // Rule 0: Current Action Invalid (Score 0)
    // If current action computes to ~0 score, it implies it cannot run.
    // Always yield regardless of priority.
    if (CurrentScore <= 0.1f)
    {
        UE_LOG(LogTemp, Log, TEXT("[Transition] ⚠️ Current Action %s Score is low (%.3f) - Forcing Yield to %s"),
            *Current->ActionName, CurrentScore, *Candidate->ActionName);
        return true;
    }

    // Rule 1: Priority Gate
    // Lower priority cannot interrupt higher priority
    if (Candidate->Priority < Current->Priority)
    {
        // ✅ EXCEPTION: Performance-based Priority Decay
        // Relaxed rule: If Current Action is weak (< 0.4) OR Candidate is overwhelming (> 2x Current),
        // allow the transition despite priority.
        
        bool bCurrentIsWeak = CurrentScore < 0.4f;
        bool bCandidateIsOverwhelming = CandidateScore > (CurrentScore * 2.0f);

        if (bCurrentIsWeak || bCandidateIsOverwhelming)
        {
             UE_LOG(LogTemp, Warning, TEXT("[Transition] ⚠️ Priority Shield Broken: %s(%.2f) yields to %s(%.2f) [Weak:%d, Overwhelming:%d]"),
                *Current->ActionName, CurrentScore, *Candidate->ActionName, CandidateScore, bCurrentIsWeak, bCandidateIsOverwhelming);
             return true;
        }

        UE_LOG(LogTemp, Verbose, TEXT("[Transition] Denied: %s(%d) < %s(%d) Priority"),
               *Candidate->ActionName, (int32)Candidate->Priority,
               *Current->ActionName, (int32)Current->Priority);
        return false;
    }
    
    // Rule 2: Commitment Lock
    // If current action is committed, only Threat+ can break it
    if (Current->IsCommitted(CurrentTime))
    {
        if (Candidate->Priority < EActionPriority::Threat)
        {
            UE_LOG(LogTemp, Log, TEXT("[Transition] Denied: %s blocked by %s Commitment (%.1fs left)"),
                   *Candidate->ActionName, *Current->ActionName,
                   Current->CommitmentTime - (CurrentTime - Current->CommitmentStartTime));
            return false;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[Transition] ⚡ Emergency: %s(Threat+) breaks %s Commitment!"),
                   *Candidate->ActionName, *Current->ActionName);
            return true;
        }
    }
    
    // Rule 3: Same-Priority Inertia
    // Must beat current score by inertia margin
    if (Candidate->Priority == Current->Priority)
    {
        float InertiaThreshold = Current->LastScore * (1.0f + Current->InertiaBonus);
        if (CandidateScore <= InertiaThreshold)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[Transition] Denied: %s(%.2f) <= %s(%.2f + %.0f%% inertia)"),
                   *Candidate->ActionName, CandidateScore,
                   *Current->ActionName, Current->LastScore, Current->InertiaBonus * 100.0f);
            return false;
        }
    }
    
    // Rule 4: Data-Driven Exit Conditions
    // Check if current action should exit based on its configured conditions
    if (OwnerController && OwnerController->MentalState)
    {
        if (Current->CheckExitConditions(OwnerController->MentalState, OwnerController))
        {
            UE_LOG(LogTemp, Log, TEXT("[Transition] %s exit conditions met -> allowing switch to %s"),
                   *Current->ActionName, *Candidate->ActionName);
            return true;
        }
    }
    
    // Rule 5: Higher priority always wins (unless blocked by Rules 1-2)
    if (Candidate->Priority > Current->Priority)
    {
        UE_LOG(LogTemp, Log, TEXT("[Transition] Allowed: %s(%d) > %s(%d) Priority"),
               *Candidate->ActionName, (int32)Candidate->Priority,
               *Current->ActionName, (int32)Current->Priority);
        return true;
    }
    
    // Default: Allow if we passed all checks
    return true;
}