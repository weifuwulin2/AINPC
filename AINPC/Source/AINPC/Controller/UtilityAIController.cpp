// Fill out your copyright notice in the Description page of Project Settings.
#include "Controller/UtilityAIController.h"
#include "Base/UtilityActionBase.h"
#include "UtilityAI/UNPCMentalState.h"


AUtilityAIController::AUtilityAIController(): MentalState(nullptr), CurrentAction(nullptr)
{
    // 开启 Tick，让 AI 能实时反应
    PrimaryActorTick.bCanEverTick = true;
}

void AUtilityAIController::BeginPlay()
{
    Super::BeginPlay();
    
    MentalState = NewObject<UNPCMentalState>(this);
    if (MentalState)
    {
        MentalState->Anger = 0.9f;
        MentalState->Fear = 0.1f;
    }
    
    for (TSubclassOf<UUtilityActionBase> Class : ActionClasses)
    {
        if (Class)
        {
            UUtilityActionBase* NewAction = NewObject<UUtilityActionBase>(this, Class);
            if (NewAction)
            {
                AvailableActions.Add(NewAction);
                UE_LOG(LogTemp, Log, TEXT("Action Initialized: %s"), *NewAction->ActionName);
            }
        }
    }
}

void AUtilityAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 每帧思考 (优化建议：可以用 Timer 每 0.2s 跑一次)
    EvaluateUtilityLogic();
}

void AUtilityAIController::EvaluateUtilityLogic()
{
    if (!MentalState || AvailableActions.Num() == 0) return;

    UUtilityActionBase* BestAction = nullptr;
    float BestScore = -1.0f;

    // --- 1. 遍历打分 ---
    for (UUtilityActionBase* Action : AvailableActions)
    {
        float Score = Action->CalculateScore(MentalState);
        
        // 如果是当前动作，给点惯性分(Inertia)，防止频繁抖动
        if (Action == CurrentAction)
        {
            Score += 5.0f; 
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestAction = Action;
        }
    }

    // --- 2. 执行最优解 ---
    if (BestAction && BestAction != CurrentAction)
    {
        // 退出旧动作
        if (CurrentAction) CurrentAction->Exit(this);

        // 切换新动作
        CurrentAction = BestAction;
        CurrentAction->Execute(this);

        UE_LOG(LogTemp, Warning, TEXT("Switching to Action: %s (Score: %f)"), *CurrentAction->ActionName, BestScore);
    }
    else if (CurrentAction)
    {
        // 保持当前动作
        CurrentAction->Execute(this);
    }
}