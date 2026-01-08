// TestAction_Flee.cpp
// 测试用的逃跑动作实现

#include "Test/TestAction_Flee.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"

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
		LastMoveTime = World->GetTimeSeconds();
	}
	bIsComplete = false;
	CurrentThreat = nullptr;
	
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
	
	if (!Controller) return;
	UWorld* World = Controller->GetWorld();
	if (!World) return;
	APawn* ControlledPawn = Controller->GetPawn();
	if (!ControlledPawn) return;
	
	// --- 1. 威胁搜索逻辑 ---
	if (!CurrentThreat || !IsValid(CurrentThreat))
	{
		float MinDistance = FLT_MAX;
		AActor* BestThreat = nullptr;

		// 策略：只把带有 "Enemy" 标签的角色或者玩家当作威胁
		// Strategy: Only treat actors with "Enemy" tag or Player as threat
		
		// Pass 1: 搜索 "Enemy"
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			ACharacter* Candidate = *It;
			if (Candidate == ControlledPawn) continue;
			if (Candidate->IsPendingKillPending()) continue;
			
			// 必须有 Enemy 标签
			if (!Candidate->ActorHasTag("Enemy")) continue;

			float Dist = FVector::Dist(ControlledPawn->GetActorLocation(), Candidate->GetActorLocation());
			if (Dist < MinDistance)
			{
				MinDistance = Dist;
				BestThreat = Candidate;
			}
		}

		if (BestThreat)
		{
			CurrentThreat = BestThreat;
			UE_LOG(LogTemp, Warning, TEXT("[Flee] Found threat (Tag:Enemy): %s (Dist: %.1f)"), *CurrentThreat->GetName(), MinDistance);
		}
		else
		{
			// Pass 2: 如果没有 Enemy，默认远离玩家 (Player 0)
			CurrentThreat = UGameplayStatics::GetPlayerPawn(World, 0);
			if (CurrentThreat == ControlledPawn) CurrentThreat = nullptr;
		}
	}

	if (!CurrentThreat)
	{
		if (fmod(World->GetTimeSeconds(), 2.0f) < World->GetDeltaSeconds())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Flee] No 'Enemy' tag found and Player is safe/self. Relaxing..."));
		}
		return;
	}
	
	float CurrentTime = World->GetTimeSeconds();
	float ElapsedTime = CurrentTime - ExecutionTime;
	
	// --- 2. 逃跑逻辑 ---
	FVector PawnLocation = ControlledPawn->GetActorLocation();
	FVector ThreatLocation = CurrentThreat->GetActorLocation();
	FVector ToThreat = ThreatLocation - PawnLocation;
	float DistanceToThreat = ToThreat.Size();
	
	// 无论距离多远，只要 Action 还在运行，就继续保持远离状态
	
	// 频率限制：每 0.25s 更新一次移动目标，防止鬼畜
	if (CurrentTime - LastMoveTime > 0.25f)
	{
		LastMoveTime = CurrentTime;

		// 计算逃跑方向（威胁源的反方向）
		FVector FleeDirection = -ToThreat.GetSafeNormal();
		
		// 目标：往远处跑
		float RunDist = (DistanceToThreat > SafeDistance) ? 500.0f : SafeDistance;
		FVector FleeTarget = PawnLocation + FleeDirection * RunDist;
		
		// 寻路
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
		if (NavSys)
		{
			FNavLocation NavLocation;
			if (NavSys->ProjectPointToNavigation(FleeTarget, NavLocation, FVector(RunDist, RunDist, 500.0f)))
			{
				Controller->MoveToLocation(NavLocation.Location, 50.0f);
			}
			else
			{
				Controller->MoveToLocation(FleeTarget, 50.0f);
			}
		}
		else
		{
			Controller->MoveToLocation(FleeTarget, 50.0f);
		}
	}
	
	if (FMath::Fmod(ElapsedTime, 1.0f) < World->GetDeltaSeconds())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Flee] Running from %s (Dist: %.0f)..."), 
			*CurrentThreat->GetName(), DistanceToThreat);
	}

	// 永不完成，直到 Utility AI 切换
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
