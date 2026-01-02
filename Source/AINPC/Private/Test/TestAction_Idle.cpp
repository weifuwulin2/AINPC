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

void UTestAction_Idle::Enter_Implementation(AAIController* Controller)
{
	Super::Enter_Implementation(Controller);
	
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		ExecutionTime = World->GetTimeSeconds();
	}
	IdleCounter++;
	
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] Idle Action ENTERED (Count: %d)"), IdleCounter);
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
}

void UTestAction_Idle::Execute_Implementation(AAIController* Controller)
{
	Super::Execute_Implementation(Controller);
	
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		float CurrentTime = World->GetTimeSeconds();
		float ElapsedTime = CurrentTime - ExecutionTime;
		
		// 每2秒打印一次
		if (FMath::Fmod(ElapsedTime, 2.0f) < World->GetDeltaSeconds())
		{
			UE_LOG(LogTemp, Verbose, TEXT("[Idle] Waiting... Time: %.1fs"), ElapsedTime);
		}
	}
	
	// Idle 永不完成，除非被其他动作打断
}

void UTestAction_Idle::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);
	
	float Duration = 0.0f;
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		Duration = World->GetTimeSeconds() - ExecutionTime;
	}
	
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] Idle Action EXITED"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] Idle duration: %.1fs"), Duration);
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
}
