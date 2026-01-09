#include "Components/UtilityAIComponent.h"
#include "Controller/UtilityAIController.h"
#include "UtilityAI/UNPCMentalState.h"
#include "Components/PersonalityComponent.h"

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
        UE_LOG(LogTemp, Error, TEXT("UtilityComp must be attached to UtilityAIController!"));
        return;
    }

    LoadActionsFromTable();
}

void UUtilityAIComponent::LoadActionsFromTable()
{
    if (!ActionDataTable) return;

    static const FString ContextString(TEXT("UtilityAI Actions Load"));
    TArray<FUtilityActionConfig*> Rows;
    ActionDataTable->GetAllRows(ContextString, Rows);

    for (FUtilityActionConfig* Row : Rows)
    {
        if (Row && Row->ActionClass)
        {
            // 注意：Action 的 Outer 设为 this (Component)，方便管理生命周期
            UUtilityActionBase* NewAction = NewObject<UUtilityActionBase>(this, Row->ActionClass);
            NewAction->InitFromConfig(*Row);
            AvailableActions.Add(NewAction);
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
}

void UUtilityAIComponent::EvaluateAndDecide()
{
    if (AvailableActions.Num() == 0) return;

    UUtilityActionBase* BestAction = nullptr;
    float BestScore = -1.0f;
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
        UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
        UE_LOG(LogTemp, Warning, TEXT("[UtilityAI|%s] Evaluating Actions (Count: %d) [Triggered]"), *PersonalityID, AvailableActions.Num());
        
        // 显示当前正在执行的 Action
        if (CurrentAction)
        {
            UE_LOG(LogTemp, Log, TEXT("[UtilityAI|%s] Currently Running: %s"), *PersonalityID, *CurrentAction->ActionName);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("[UtilityAI|%s] Currently Running: None"), *PersonalityID);
        }
    }

    // --- 遍历打分 ---
    for (UUtilityActionBase* Action : AvailableActions)
    {
        // 调用 Action 自身的算分逻辑
        // Pass bShouldLog (which is true if bPendingDebugLog is true) to enable detailed calculation logs
        float Score = Action->CalculateScore(State, OwnerController, bShouldLog);

        // 🔍 调试：打印每个 Action 的分数
        if (bShouldLog)
        {/*
            UE_LOG(LogTemp, Log, TEXT("  [%s|%s] Score: %.3f (BaseWeight: %.2f, Considerations: %d)"), 
                   *PersonalityID, *Action->ActionName, Score, Action->BaseWeight, Action->Considerations.Num());*/
        }

        // 惯性奖励 (Momentum)
        if (Action == CurrentAction)
        {
            float OldScore = Score;
            Score += Action->InertiaBonus; // 或者直接写死 +0.1f
            
            if (bShouldLog)
            {
                UE_LOG(LogTemp, Log, TEXT("    ↳ [%s] Inertia Bonus: +%.2f (%.3f -> %.3f)"), 
                       *PersonalityID, Action->InertiaBonus, OldScore, Score);
            }
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestAction = Action;
        }
    }

    if (bShouldLog)
    {
        if (BestAction)
        {
            UE_LOG(LogTemp, Warning, TEXT("[UtilityAI|%s] 🏆 Best Action: %s (Score: %.3f)"), 
                   *PersonalityID, *BestAction->ActionName, BestScore);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[UtilityAI|%s] ⚠️ No valid action found!"), *PersonalityID);
        }
        UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
        // Reset the request flag
        bPendingDebugLog = false;
    }

    // --- 状态切换 ---
    if (BestAction && BestAction != CurrentAction)
    {
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
        UE_LOG(LogTemp, Warning, TEXT("[UtilityAI|%s] ✅ Switch Action: %s (Score: %.2f)"), *PersonalityID, *CurrentAction->ActionName, BestScore);
    }
}