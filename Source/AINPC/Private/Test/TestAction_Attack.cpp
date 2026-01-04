// TestAction_Attack.cpp
// 测试用的攻击动作实现

#include "Test/TestAction_Attack.h"
#include "AIController.h"
#include "GameFramework/Character.h"

UTestAction_Attack::UTestAction_Attack()
{
	ActionName = "Test_Attack";
	ExecutionTime = 0.0f;
	bIsComplete = false;
}

void UTestAction_Attack::Enter_Implementation(AAIController* Controller)
{
	Super::Enter_Implementation(Controller);
	
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		ExecutionTime = World->GetTimeSeconds();
	}
	
	// 重置完成标志，允许重新执行
	bIsComplete = false;
	
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Warning, TEXT("[TEST] Attack Action ENTERED"));
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	
	// 播放攻击动画（测试用，只打印日志）
	if (APawn* Pawn = Controller->GetPawn())
	{
		UE_LOG(LogTemp, Log, TEXT("[Attack] Pawn: %s"), *Pawn->GetName());
		UE_LOG(LogTemp, Log, TEXT("[Attack] Location: %s"), *Pawn->GetActorLocation().ToString());
	}
}

void UTestAction_Attack::Execute_Implementation(AAIController* Controller)
{
	Super::Execute_Implementation(Controller);
	
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		float CurrentTime = World->GetTimeSeconds();
		float ElapsedTime = CurrentTime - ExecutionTime;
		
		// 每秒打印一次
		if (FMath::Fmod(ElapsedTime, 1.0f) < World->GetDeltaSeconds())
		{
			UE_LOG(LogTemp, Log, TEXT("[Attack] Executing... Time: %.1fs"), ElapsedTime);
		}
		
		// 执行一段时间后完成
		if (ElapsedTime >= MaxExecutionTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Attack] Completed after %.1fs"), ElapsedTime);
			bIsComplete = true;
		}
	}
}

void UTestAction_Attack::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);
	
	float Duration = 0.0f;
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		Duration = World->GetTimeSeconds() - ExecutionTime;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Warning, TEXT("[TEST] Attack Action EXITED"));
	UE_LOG(LogTemp, Warning, TEXT("[TEST] Total execution time: %.1fs"), Duration);
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
}
