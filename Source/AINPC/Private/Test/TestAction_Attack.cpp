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
	
	// ✅ 立即搜索并设置攻击目标
	// Immediately search and set attack target
	if (Controller && Controller->GetWorld())
	{
		APawn* ControlledPawn = Controller->GetPawn();
		if (ControlledPawn)
		{
			// 搜索最近的敌人
			AActor* TargetEnemy = nullptr;
			float MinDistance = FLT_MAX;
			
			TArray<AActor*> FoundActors;
			UGameplayStatics::GetAllActorsWithTag(Controller->GetWorld(), FName("Enemy"), FoundActors);
			
			for (AActor* Actor : FoundActors)
			{
				if (Actor == ControlledPawn) continue;
				if (!IsValid(Actor)) continue;
				
				float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), Actor->GetActorLocation());
				if (Distance < MinDistance)
				{
					MinDistance = Distance;
					TargetEnemy = Actor;
				}
			}
			
			// 如果没有找到 "Enemy" 标签的 Actor，尝试攻击玩家
			if (!TargetEnemy)
			{
				TargetEnemy = UGameplayStatics::GetPlayerPawn(Controller->GetWorld(), 0);
				if (TargetEnemy == ControlledPawn)
				{
					TargetEnemy = nullptr;
				}
			}
			
			// ✅ 设置 FocusActor，让 Utility AI 的 HasAttackTarget 检查通过
			if (TargetEnemy)
			{
				Controller->SetFocus(TargetEnemy, EAIFocusPriority::Gameplay);
				UE_LOG(LogTemp, Log, TEXT("[Attack] Target locked: %s"), *TargetEnemy->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Attack] No target found on Enter!"));
				bIsComplete = true; // 没有目标，直接标记完成
			}
		}
	}
	
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

	// ✅ 死亡检查：如果 Pawn 已死亡，立即停止攻击
	// Death Check: If Pawn is dead, stop attacking immediately
	if (ACharacter* Character = Cast<ACharacter>(ControlledPawn))
	{
		if (Character->GetMesh() && Character->GetMesh()->GetAnimInstance())
		{
			// 检查是否在播放死亡动画或已经死亡
			// Check if playing death animation or already dead
			if (Character->GetMesh()->GetAnimInstance()->IsAnyMontagePlaying())
			{
				// 可能在播放死亡动画，停止行为
				bIsComplete = true;
				UE_LOG(LogTemp, Warning, TEXT("[Attack] Pawn is playing montage (possibly death), stopping attack"));
				return;
			}
		}
	}

	// ✅ 通用死亡检查：检查 Pawn 是否有效且未被销毁
	// Generic death check: Check if Pawn is valid and not being destroyed
	if (!IsValid(ControlledPawn) || ControlledPawn->IsPendingKillPending())
	{
		bIsComplete = true;
		UE_LOG(LogTemp, Warning, TEXT("[Attack] Pawn is invalid or pending kill, stopping attack"));
		return;
	}
	
	// 寻找最近的敌人（优先攻击带有 "Enemy" 标签的 Actor）
	// Find nearest enemy (prioritize actors with "Enemy" tag)
	AActor* TargetEnemy = nullptr;
	float MinDistance = FLT_MAX;
	
	// 搜索所有带有 "Enemy" 标签的 Actor
	// Search for all actors with "Enemy" tag
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("Enemy"), FoundActors);
	
	for (AActor* Actor : FoundActors)
	{
		if (Actor == ControlledPawn) continue;
		if (!IsValid(Actor)) continue;
		
		float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), Actor->GetActorLocation());
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			TargetEnemy = Actor;
		}
	}
	
	// 如果没有找到 "Enemy" 标签的 Actor，尝试攻击玩家
	// If no "Enemy" tagged actor found, try to attack player
	if (!TargetEnemy)
	{
		TargetEnemy = UGameplayStatics::GetPlayerPawn(World, 0);
		if (TargetEnemy == ControlledPawn)
		{
			TargetEnemy = nullptr;
		}
	}
	
	if (!TargetEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Attack] No enemy found! Stopping attack."));
		bIsComplete = true; // ✅ 没有目标，标记完成
		Controller->ClearFocus(EAIFocusPriority::Gameplay); // ✅ 清除 Focus
		return;
	}

	// ✅ 设置 FocusActor，让 Utility AI 知道当前攻击目标
	// Set FocusActor so Utility AI knows the current attack target
	Controller->SetFocus(TargetEnemy, EAIFocusPriority::Gameplay);
	
	float CurrentTime = World->GetTimeSeconds();
	float ElapsedTime = CurrentTime - ExecutionTime;
	
	// 计算到敌人的距离
	FVector ToEnemy = TargetEnemy->GetActorLocation() - ControlledPawn->GetActorLocation();
	float DistanceToEnemy = ToEnemy.Size();
	
	// 移动向敌人
	if (DistanceToEnemy > AttackRange)
	{
		// 还没到攻击范围，继续移动
		Controller->MoveToActor(TargetEnemy, AttackRange * 0.8f);
		
		if (FMath::Fmod(ElapsedTime, 1.0f) < World->GetDeltaSeconds())
		{
			UE_LOG(LogTemp, Log, TEXT("[Attack] Chasing %s... Distance: %.1f"), *TargetEnemy->GetName(), DistanceToEnemy);
		}
	}
	else
	{
		// 在攻击范围内，停止移动并攻击
		Controller->StopMovement();
		
		// 面向敌人
		FRotator LookAtRotation = ToEnemy.Rotation();
		LookAtRotation.Pitch = 0.0f;
		LookAtRotation.Roll = 0.0f;
		ControlledPawn->SetActorRotation(LookAtRotation);
		
		// 检查是否可以攻击
		if (CurrentTime - LastAttackTime >= AttackCooldown)
		{
			// 造成伤害
			UGameplayStatics::ApplyDamage(
				TargetEnemy,
				Damage,
				Controller,
				ControlledPawn,
				UDamageType::StaticClass()
			);
			
			LastAttackTime = CurrentTime;
			
			UE_LOG(LogTemp, Warning, TEXT("[Attack] Dealt %.1f damage to %s!"), Damage, *TargetEnemy->GetName());
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
		
		// ✅ 清除攻击目标 Focus
		// Clear attack target Focus
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
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
