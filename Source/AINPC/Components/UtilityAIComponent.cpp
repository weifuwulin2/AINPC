#include "Components/UtilityAIComponent.h"
#include "Controller/UtilityAIController.h"
#include "UtilityAI/UNPCMentalState.h"

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

    // 🔍 调试：打印当前 MentalState
    static float LastLogTime = 0.0f;
    float CurrentTime = GetWorld()->GetTimeSeconds();
    bool bShouldLog = (CurrentTime - LastLogTime) > 2.0f; // 每2秒打印一次
    
    if (bShouldLog)
    {
        UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
        UE_LOG(LogTemp, Warning, TEXT("[UtilityAI] Evaluating Actions (Count: %d)"), AvailableActions.Num());
        if (State)
        {
            UE_LOG(LogTemp, Log, TEXT("[UtilityAI] MentalState: Anger=%.2f, Fear=%.2f, Confidence=%.2f"), 
                   State->Anger, State->Fear, State->Confidence);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[UtilityAI] ⚠️ MentalState is NULL!"));
        }
    }

    // --- 遍历打分 ---
    for (UUtilityActionBase* Action : AvailableActions)
    {
        // 调用 Action 自身的算分逻辑
        float Score = Action->CalculateScore(State, OwnerController);

        // 🔍 调试：打印每个 Action 的分数
        if (bShouldLog)
        {
            UE_LOG(LogTemp, Log, TEXT("  [%s] Score: %.3f (BaseWeight: %.2f, Considerations: %d)"), 
                   *Action->ActionName, Score, Action->BaseWeight, Action->Considerations.Num());
        }

        // 惯性奖励 (Momentum)
        if (Action == CurrentAction)
        {
            float OldScore = Score;
            Score += Action->InertiaBonus; // 或者直接写死 +0.1f
            
            if (bShouldLog)
            {
                UE_LOG(LogTemp, Log, TEXT("    ↳ Inertia Bonus: +%.2f (%.3f -> %.3f)"), 
                       Action->InertiaBonus, OldScore, Score);
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
            UE_LOG(LogTemp, Warning, TEXT("[UtilityAI] 🏆 Best Action: %s (Score: %.3f)"), 
                   *BestAction->ActionName, BestScore);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[UtilityAI] ⚠️ No valid action found!"));
        }
        UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
        LastLogTime = CurrentTime;
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
        UE_LOG(LogTemp, Warning, TEXT("[UtilityAI] ✅ Switch Action: %s (Score: %.2f)"), *CurrentAction->ActionName, BestScore);
    }
}