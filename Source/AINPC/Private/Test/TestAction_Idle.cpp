// TestAction_Idle.cpp
// 测试用的待机动作实现

#include "Test/TestAction_Idle.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

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
		// 使用导航系统找到可到达的位置
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
		bool bFoundLocation = false;

		if (NavSys)
		{
			FNavLocation NavLocation;
			// 使用 GetRandomReachablePointInRadius 代替手动计算 projection，确保点在 NavMesh 上且可达
			if (NavSys->GetRandomReachablePointInRadius(ControlledPawn->GetActorLocation(), WanderRadius, NavLocation))
			{
				CurrentTargetLocation = NavLocation.Location;
				bFoundLocation = true;
				
				EPathFollowingRequestResult::Type Result = Controller->MoveToLocation(CurrentTargetLocation, 50.0f);
				if (Result == EPathFollowingRequestResult::Type::Failed)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Idle] MoveToLocation Failed! Retrying soon..."));
					// Retry quickly
					NextDirectionChangeTime = CurrentTime + 0.5f;
					CurrentTargetLocation = FVector::ZeroVector; 
				}
				else
				{
					UE_LOG(LogTemp, Verbose, TEXT("[Idle] Moving to random location: %s"), *CurrentTargetLocation.ToString());
					// Set next change time normally
					NextDirectionChangeTime = CurrentTime + DirectionChangeInterval;
				}
			}
		}
		
		if (!bFoundLocation)
		{
			// 如果没有 NavSys 或者找不到点 (fallback)
			Controller->StopMovement();
			UE_LOG(LogTemp, Warning, TEXT("[Idle] No valid reachable point found within radius %.1f, staying in place. Retrying in 1s."), WanderRadius);
			
			// Try a manual fallback if NavSys is missing (though rare for AI)
			if (!NavSys)
			{
				CurrentTargetLocation = ControlledPawn->GetActorLocation() + FMath::RandPointInBox(FBox(FVector(-WanderRadius), FVector(WanderRadius)));
				CurrentTargetLocation.Z = ControlledPawn->GetActorLocation().Z;
				Controller->MoveToLocation(CurrentTargetLocation, 50.0f);
			}

			// Retry faster if we failed to find a point
			NextDirectionChangeTime = CurrentTime + 1.0f;
		}
	}
	
	// 检查是否到达目标位置
	if (CurrentTargetLocation != FVector::ZeroVector)
	{
		float DistanceToTarget = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentTargetLocation);
		
		if (DistanceToTarget < 100.0f)
		{
			// 到达目标，停止移动
			Controller->StopMovement();
			CurrentTargetLocation = FVector::ZeroVector; // 清除当前目标
			
			// 随机停顿一下 (0.5 - 2.0s)，而不是等满3秒
			float PauseTime = FMath::RandRange(0.5f, 2.0f);
			NextDirectionChangeTime = CurrentTime + PauseTime;
			
			UE_LOG(LogTemp, Verbose, TEXT("[Idle] Reached target, pausing for %.1fs..."), PauseTime);
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
