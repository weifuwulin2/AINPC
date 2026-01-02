// TestAction_Attack.cpp
// 测试用的攻击动作实现

#include "Test/TestAction_Attack.h"
#include "AIController.h"
#include "GameFramework/Character.h"

UTestAction_Attack::UTestAction_Attack()
{
	ActionName = "Test_Attack";
	ExecutionTime = 0.0f;
}

void UTestAction_Attack::OnEnter_Implementation(AAIController* Controller)
{
	Super::OnEnter_Implementation(Controller);
	
	ExecutionTime = 0.0f;
	
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

void UTestAction_Attack::OnExecute_Implementation(AAIController* Controller, float DeltaTime)
{
	Super::OnExecute_Implementation(Controller, DeltaTime);
	
	ExecutionTime += DeltaTime;
	
	// 每秒打印一次
	if (FMath::Fmod(ExecutionTime, 1.0f) < DeltaTime)
	{
		UE_LOG(LogTemp, Log, TEXT("[Attack] Executing... Time: %.1fs"), ExecutionTime);
	}
	
	// 执行一段时间后完成
	if (ExecutionTime >= MaxExecutionTime)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Attack] Completed after %.1fs"), ExecutionTime);
		bIsComplete = true;
	}
}

void UTestAction_Attack::OnExit_Implementation(AAIController* Controller)
{
	Super::OnExit_Implementation(Controller);
	
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Warning, TEXT("[TEST] Attack Action EXITED"));
	UE_LOG(LogTemp, Warning, TEXT("[TEST] Total execution time: %.1fs"), ExecutionTime);
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
}
