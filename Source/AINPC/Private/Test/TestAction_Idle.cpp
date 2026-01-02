// TestAction_Idle.cpp
// 测试用的待机动作实现

#include "Test/TestAction_Idle.h"
#include "AIController.h"

UTestAction_Idle::UTestAction_Idle()
{
	ActionName = "Test_Idle";
	ExecutionTime = 0.0f;
	IdleCounter = 0;
}

void UTestAction_Idle::OnEnter_Implementation(AAIController* Controller)
{
	Super::OnEnter_Implementation(Controller);
	
	ExecutionTime = 0.0f;
	IdleCounter++;
	
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] Idle Action ENTERED (Count: %d)"), IdleCounter);
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
}

void UTestAction_Idle::OnExecute_Implementation(AAIController* Controller, float DeltaTime)
{
	Super::OnExecute_Implementation(Controller, DeltaTime);
	
	ExecutionTime += DeltaTime;
	
	// 每2秒打印一次
	if (FMath::Fmod(ExecutionTime, 2.0f) < DeltaTime)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Idle] Waiting... Time: %.1fs"), ExecutionTime);
	}
	
	// Idle 永不完成，除非被其他动作打断
}

void UTestAction_Idle::OnExit_Implementation(AAIController* Controller)
{
	Super::OnExit_Implementation(Controller);
	
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] Idle Action EXITED"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] Idle duration: %.1fs"), ExecutionTime);
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
}
