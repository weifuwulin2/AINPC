// TestAction_Flee.cpp
// 测试用的逃跑动作实现

#include "Test/TestAction_Flee.h"
#include "AIController.h"
#include "GameFramework/Character.h"

UTestAction_Flee::UTestAction_Flee()
{
	ActionName = "Test_Flee";
	ExecutionTime = 0.0f;
}

void UTestAction_Flee::OnEnter_Implementation(AAIController* Controller)
{
	Super::OnEnter_Implementation(Controller);
	
	ExecutionTime = 0.0f;
	
	UE_LOG(LogTemp, Error, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Error, TEXT("[TEST] Flee Action ENTERED (EMERGENCY!)"));
	UE_LOG(LogTemp, Error, TEXT("═══════════════════════════════════════"));
	
	if (APawn* Pawn = Controller->GetPawn())
	{
		UE_LOG(LogTemp, Log, TEXT("[Flee] Pawn: %s"), *Pawn->GetName());
		UE_LOG(LogTemp, Log, TEXT("[Flee] Escaping from danger!"));
	}
}

void UTestAction_Flee::OnExecute_Implementation(AAIController* Controller, float DeltaTime)
{
	Super::OnExecute_Implementation(Controller, DeltaTime);
	
	ExecutionTime += DeltaTime;
	
	if (FMath::Fmod(ExecutionTime, 0.5f) < DeltaTime)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Flee] Running away... Time: %.1fs"), ExecutionTime);
	}
	
	if (ExecutionTime >= MaxExecutionTime)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Flee] Escaped successfully after %.1fs"), ExecutionTime);
		bIsComplete = true;
	}
}

void UTestAction_Flee::OnExit_Implementation(AAIController* Controller)
{
	Super::OnExit_Implementation(Controller);
	
	UE_LOG(LogTemp, Error, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Error, TEXT("[TEST] Flee Action EXITED"));
	UE_LOG(LogTemp, Error, TEXT("[TEST] Total escape time: %.1fs"), ExecutionTime);
	UE_LOG(LogTemp, Error, TEXT("═══════════════════════════════════════"));
}
