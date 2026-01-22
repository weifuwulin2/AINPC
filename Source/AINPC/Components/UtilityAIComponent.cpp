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
            // 1. 如果配置了 RequiredProfessionID，且不为 None
            if (!Row->RequiredProfessionID.IsNone())
            {
                // 2. 如果当前 NPC 的 ID 与配置的不匹配，则跳过
                if (Row->RequiredProfessionID != CurrentProfessionID)
                {
                    UTILITY_LOG(Warning, "🚫 Skipped Action %s (Requires: %s, Current: %s)", 
                           *Row->ActionName, *Row->RequiredProfessionID.ToString(), *CurrentProfessionID.ToString());
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
    for (UUtilityActionBase* Action : AvailableActions)
    {
        // 调用 Action 自身的算分逻辑
        // Pass bShouldLog (which is true if bPendingDebugLog is true) to enable detailed calculation logs
        float Score = Action->CalculateScore(State, OwnerController, bShouldLog);

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
        // ✅ 如果当前 Action 不想退出（比如 SmartObject Duration 还没到），检查是否有紧急情况
        // If current action doesn't want to exit (e.g., SmartObject Duration not expired), check for emergencies
        if (CurrentAction && !CurrentAction->ShouldExit(OwnerController))
        {
            // 紧急打断条件：高威胁（被攻击时惊醒）
            // Emergency interrupt: High threat (wake up when attacked)
            bool bEmergencyInterrupt = false;
            
            AUtilityAIController* UtilController = Cast<AUtilityAIController>(OwnerController);
            if (UtilController && UtilController->MentalState)
            {
                // 如果威胁感知很高（Perceived_Threat > 0.5），允许打断
                if (UtilController->MentalState->Perceived_Threat > 0.5f)
                {
                    bEmergencyInterrupt = true;
                    UE_LOG(LogTemp, Warning, TEXT("[UtilityAI|%s] ⚠️ Emergency interrupt! Threat: %.2f"), 
                           *PersonalityID, UtilController->MentalState->Perceived_Threat);
                }
            }
            
            if (!bEmergencyInterrupt)
            {
                // 没有紧急情况，当前动作仍在进行中，不切换
                // No emergency, current action still in progress, don't switch
                return;
            }
        }
        
        // 退出旧的
        if (CurrentAction)
        {
            CurrentAction->Exit(OwnerController);
        }

        // 进入新的
        CurrentAction = BestAction;
        CurrentAction->Enter(OwnerController);
        
        // 记录时间用于冷却计算
        CurrentAction->MarkExecutionTime(GetWorld()->GetTimeSeconds());

        // 打印切换日志
        AINPC_LOG(Warning, "[%s] ✅ Switch Action: %s (Score: %.2f)", *PersonalityID, *CurrentAction->ActionName, BestScore);
    }
}