// TestAction_Idle.cpp
// 测试用的待机动作实现

#include "Test/TestAction_Idle.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

UTestAction_Idle::UTestAction_Idle()
{
	ActionName = "Test_Idle";
	ExecutionTime = 0.0f;
	IdleCounter = 0;
	NextDirectionChangeTime = 0.0f;
	CurrentTargetLocation = FVector::ZeroVector;
}

void UTestAction_Idle::Enter_Implementation(AAIController* Controller)
{
	Super::Enter_Implementation(Controller);
	
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		ExecutionTime = World->GetTimeSeconds();
		// 立即设置第一个随机目标
		NextDirectionChangeTime = ExecutionTime;
	}
	IdleCounter++;
	
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] Idle Action ENTERED (Count: %d)"), IdleCounter);
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	
	// 设置移动速度
	if (APawn* Pawn = Controller->GetPawn())
	{
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
			{
				MovementComp->MaxWalkSpeed = WanderSpeed;
			}
		}
	}
}

void UTestAction_Idle::Execute_Implementation(AAIController* Controller)
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
	
	float CurrentTime = World->GetTimeSeconds();
	float ElapsedTime = CurrentTime - ExecutionTime;
	
	// 检查是否需要改变方向
	if (CurrentTime >= NextDirectionChangeTime)
	{
		// 生成随机目标位置
		FVector PawnLocation = ControlledPawn->GetActorLocation();
		
		// 在当前位置周围生成随机点
		FVector RandomDirection = FVector(
			FMath::RandRange(-1.0f, 1.0f),
			FMath::RandRange(-1.0f, 1.0f),
			0.0f
		).GetSafeNormal();
		
		float RandomDistance = FMath::RandRange(WanderRadius * 0.3f, WanderRadius);
		FVector TargetLocation = PawnLocation + RandomDirection * RandomDistance;
		
		// 使用导航系统找到可到达的位置
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
		if (NavSys)
		{
			FNavLocation NavLocation;
			if (NavSys->ProjectPointToNavigation(TargetLocation, NavLocation, FVector(500.0f, 500.0f, 500.0f)))
			{
				CurrentTargetLocation = NavLocation.Location;
				Controller->MoveToLocation(CurrentTargetLocation, 50.0f);
				
				UE_LOG(LogTemp, Log, TEXT("[Idle] Moving to random location: %s"), *CurrentTargetLocation.ToString());
			}
			else
			{
				// 如果找不到导航点，就停在原地
				Controller->StopMovement();
				UE_LOG(LogTemp, Log, TEXT("[Idle] No valid navigation point found, staying in place"));
			}
		}
		else
		{
			// 没有导航系统，直接移动
			CurrentTargetLocation = TargetLocation;
			Controller->MoveToLocation(CurrentTargetLocation, 50.0f);
		}
		
		// 设置下次改变方向的时间
		NextDirectionChangeTime = CurrentTime + DirectionChangeInterval;
	}
	
	// 检查是否到达目标位置
	if (CurrentTargetLocation != FVector::ZeroVector)
	{
		float DistanceToTarget = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentTargetLocation);
		
		if (DistanceToTarget < 100.0f)
		{
			// 到达目标，停止移动
			Controller->StopMovement();
			
			if (FMath::Fmod(ElapsedTime, 2.0f) < World->GetDeltaSeconds())
			{
				UE_LOG(LogTemp, Verbose, TEXT("[Idle] Reached target, waiting..."));
			}
		}
	}
	
	// 每2秒打印一次状态
	if (FMath::Fmod(ElapsedTime, 2.0f) < World->GetDeltaSeconds())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Idle] Wandering... Time: %.1fs"), ElapsedTime);
	}
	
	// Idle 永不完成，除非被其他动作打断
}

void UTestAction_Idle::Exit_Implementation(AAIController* Controller)
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
	
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] Idle Action EXITED"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] Idle duration: %.1fs"), Duration);
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
}
