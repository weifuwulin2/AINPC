// TestAction_Attack.cpp
// 测试用的攻击动作实现

#include "Test/TestAction_Attack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

UTestAction_Attack::UTestAction_Attack()
{
	ActionName = "Test_Attack";
	ExecutionTime = 0.0f;
	bIsComplete = false;
	LastAttackTime = 0.0f;
}

void UTestAction_Attack::Enter_Implementation(AAIController* Controller)
{
	Super::Enter_Implementation(Controller);
	
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		ExecutionTime = World->GetTimeSeconds();
		LastAttackTime = 0.0f;
	}
	
	// 重置完成标志，允许重新执行
	bIsComplete = false;
	
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Warning, TEXT("[TEST] Attack Action ENTERED"));
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	
	// 设置移动速度
	if (APawn* Pawn = Controller->GetPawn())
	{
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
			{
				MovementComp->MaxWalkSpeed = MoveSpeed;
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[Attack] Pawn: %s"), *Pawn->GetName());
		UE_LOG(LogTemp, Log, TEXT("[Attack] Location: %s"), *Pawn->GetActorLocation().ToString());
	}
}

void UTestAction_Attack::Execute_Implementation(AAIController* Controller)
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
		UE_LOG(LogTemp, Warning, TEXT("[Attack] No player found!"));
		return;
	}
	
	float CurrentTime = World->GetTimeSeconds();
	float ElapsedTime = CurrentTime - ExecutionTime;
	
	// 计算到玩家的距离
	FVector ToPlayer = PlayerPawn->GetActorLocation() - ControlledPawn->GetActorLocation();
	float DistanceToPlayer = ToPlayer.Size();
	
	// 移动向玩家
	if (DistanceToPlayer > AttackRange)
	{
		// 还没到攻击范围，继续移动
		Controller->MoveToActor(PlayerPawn, AttackRange * 0.8f);
		
		if (FMath::Fmod(ElapsedTime, 1.0f) < World->GetDeltaSeconds())
		{
			UE_LOG(LogTemp, Log, TEXT("[Attack] Chasing player... Distance: %.1f"), DistanceToPlayer);
		}
	}
	else
	{
		// 在攻击范围内，停止移动并攻击
		Controller->StopMovement();
		
		// 面向玩家
		FRotator LookAtRotation = ToPlayer.Rotation();
		LookAtRotation.Pitch = 0.0f;
		LookAtRotation.Roll = 0.0f;
		ControlledPawn->SetActorRotation(LookAtRotation);
		
		// 检查是否可以攻击
		if (CurrentTime - LastAttackTime >= AttackCooldown)
		{
			// 造成伤害
			UGameplayStatics::ApplyDamage(
				PlayerPawn,
				Damage,
				Controller,
				ControlledPawn,
				UDamageType::StaticClass()
			);
			
			LastAttackTime = CurrentTime;
			
			UE_LOG(LogTemp, Warning, TEXT("[Attack] Dealt %.1f damage to player!"), Damage);
		}
	}
	
	// 执行一段时间后完成
	if (ElapsedTime >= MaxExecutionTime)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Attack] Completed after %.1fs"), ElapsedTime);
		bIsComplete = true;
	}
}

void UTestAction_Attack::Exit_Implementation(AAIController* Controller)
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
	
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Warning, TEXT("[TEST] Attack Action EXITED"));
	UE_LOG(LogTemp, Warning, TEXT("[TEST] Total execution time: %.1fs"), Duration);
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
}
