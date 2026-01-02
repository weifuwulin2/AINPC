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

    // --- 遍历打分 ---
    for (UUtilityActionBase* Action : AvailableActions)
    {
        // 调用 Action 自身的算分逻辑
        float Score = Action->CalculateScore(State, OwnerController);

        // 惯性奖励 (Momentum)
        if (Action == CurrentAction)
        {
            Score += Action->InertiaBonus; // 或者直接写死 +0.1f
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestAction = Action;
        }
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

        // 可选：打印日志
        UE_LOG(LogTemp, Warning, TEXT("[UtilityAI] Switch Action: %s (Score: %.2f)"), *CurrentAction->ActionName, BestScore);
    }
}