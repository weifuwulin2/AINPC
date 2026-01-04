// TestAction_Flee.cpp
// 测试用的逃跑动作实现

#include "Test/TestAction_Flee.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

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
	
	// 设置逃跑速度
	if (APawn* Pawn = Controller->GetPawn())
	{
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
			{
				MovementComp->MaxWalkSpeed = FleeSpeed;
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[Flee] Pawn: %s"), *Pawn->GetName());
		UE_LOG(LogTemp, Log, TEXT("[Flee] Escaping from danger!"));
	}
}

void UTestAction_Flee::Execute_Implementation(AAIController* Controller)
{
	Super::Execute_Implementation(Controller);
	
	if (!Controller)
	{
		return;
	}
	
	UWorld* World = Controller->GetWorld();
	if (!World)
	{
		return;
	}
	
	APawn* ControlledPawn = Controller->GetPawn();
	if (!ControlledPawn)
	{
		return;
	}
	
	// 获取玩家角色
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Flee] No player found!"));
		return;
	}
	
	float CurrentTime = World->GetTimeSeconds();
	float ElapsedTime = CurrentTime - ExecutionTime;
	
	// 计算到玩家的距离
	FVector PawnLocation = ControlledPawn->GetActorLocation();
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	FVector ToPlayer = PlayerLocation - PawnLocation;
	float DistanceToPlayer = ToPlayer.Size();
	
	// 如果还没到达安全距离，继续逃跑
	if (DistanceToPlayer < SafeDistance)
	{
		// 计算逃跑方向（玩家的反方向）
		FVector FleeDirection = -ToPlayer.GetSafeNormal();
		
		// 计算目标位置
		FVector FleeTarget = PawnLocation + FleeDirection * SafeDistance;
		
		// 使用导航系统找到可到达的位置
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
		if (NavSys)
		{
			FNavLocation NavLocation;
			if (NavSys->ProjectPointToNavigation(FleeTarget, NavLocation, FVector(500.0f, 500.0f, 500.0f)))
			{
				Controller->MoveToLocation(NavLocation.Location, 50.0f);
			}
			else
			{
				// 如果找不到导航点，直接移动到目标位置
				Controller->MoveToLocation(FleeTarget, 50.0f);
			}
		}
		else
		{
			Controller->MoveToLocation(FleeTarget, 50.0f);
		}
		
		if (FMath::Fmod(ElapsedTime, 0.5f) < World->GetDeltaSeconds())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Flee] Running away... Distance: %.1f / %.1f"), DistanceToPlayer, SafeDistance);
		}
	}
	else
	{
		// 已经到达安全距离
		Controller->StopMovement();
		
		if (FMath::Fmod(ElapsedTime, 1.0f) < World->GetDeltaSeconds())
		{
			UE_LOG(LogTemp, Log, TEXT("[Flee] Safe distance reached! Distance: %.1f"), DistanceToPlayer);
		}
	}
	
	// 执行一段时间后完成
	if (ElapsedTime >= MaxExecutionTime)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Flee] Escaped successfully after %.1fs"), ElapsedTime);
		bIsComplete = true;
	}
}

void UTestAction_Flee::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);
	
	// 停止移动
	if (Controller)
	{
		Controller->StopMovement();
	}
	
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
