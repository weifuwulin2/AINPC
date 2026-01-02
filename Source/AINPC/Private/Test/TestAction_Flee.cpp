// TestAction_Flee.cpp
// 测试用的逃跑动作实现

#include "Test/TestAction_Flee.h"
#include "AIController.h"
#include "GameFramework/Character.h"

UTestAction_Flee::UTestAction_Flee()
{
	ActionName = "Test_Flee";
	ExecutionTime = 0.0f;
	bIsComplete = false;
}

void UTestAction_Flee::Enter_Implementation(AAIController* Controller)
{
	Super::Enter_Implementation(Controller);
	
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		ExecutionTime = World->GetTimeSeconds();
	}
	bIsComplete = false;
	
	UE_LOG(LogTemp, Error, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Error, TEXT("[TEST] Flee Action ENTERED (EMERGENCY!)"));
	UE_LOG(LogTemp, Error, TEXT("═══════════════════════════════════════"));
	
	if (APawn* Pawn = Controller->GetPawn())
	{
		UE_LOG(LogTemp, Log, TEXT("[Flee] Pawn: %s"), *Pawn->GetName());
		UE_LOG(LogTemp, Log, TEXT("[Flee] Escaping from danger!"));
	}
}

void UTestAction_Flee::Execute_Implementation(AAIController* Controller)
{
	Super::Execute_Implementation(Controller);
	
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		float CurrentTime = World->GetTimeSeconds();
		float ElapsedTime = CurrentTime - ExecutionTime;
		
		if (FMath::Fmod(ElapsedTime, 0.5f) < World->GetDeltaSeconds())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Flee] Running away... Time: %.1fs"), ElapsedTime);
		}
		
		if (ElapsedTime >= MaxExecutionTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Flee] Escaped successfully after %.1fs"), ElapsedTime);
			bIsComplete = true;
		}
	}
}

void UTestAction_Flee::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);
	
	float Duration = 0.0f;
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		Duration = World->GetTimeSeconds() - ExecutionTime;
	}
	
	UE_LOG(LogTemp, Error, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Error, TEXT("[TEST] Flee Action EXITED"));
	UE_LOG(LogTemp, Error, TEXT("[TEST] Total escape time: %.1fs"), Duration);
	UE_LOG(LogTemp, Error, TEXT("═══════════════════════════════════════"));
}
