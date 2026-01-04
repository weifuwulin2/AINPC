// TestCheatManager.cpp
// 测试用作弊管理器实现

#include "Test/TestCheatManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Controller/UtilityAIController.h"
#include "Variant_Combat/AI/CombatEnemy.h"
#include "Engine/DamageEvents.h"

void UTestCheatManager::DamageAllNPCs(float Damage)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] No valid world!"));
		return;
	}

	int32 DamagedCount = 0;
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ACombatEnemy::StaticClass(), FoundActors);

	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Damaging all NPCs with %.1f damage"), Damage);
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Found %d NPCs"), FoundActors.Num());

	for (AActor* Actor : FoundActors)
	{
		if (ACombatEnemy* Enemy = Cast<ACombatEnemy>(Actor))
		{
			// 使用 TakeDamage 来触发伤害事件
			FDamageEvent DamageEvent;
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			
			float ActualDamage = Enemy->TakeDamage(Damage, DamageEvent, PC, PC ? PC->GetPawn() : nullptr);
			
			if (ActualDamage > 0.0f)
			{
				DamagedCount++;
				UE_LOG(LogTemp, Log, TEXT("[CheatManager] ✅ Damaged %s (%.1f HP)"), 
					   *Enemy->GetName(), ActualDamage);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Successfully damaged %d NPCs"), DamagedCount);
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
}

void UTestCheatManager::KillAllNPCs()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] No valid world!"));
		return;
	}

	int32 KilledCount = 0;
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ACombatEnemy::StaticClass(), FoundActors);

	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Killing all NPCs"));

	for (AActor* Actor : FoundActors)
	{
		if (ACombatEnemy* Enemy = Cast<ACombatEnemy>(Actor))
		{
			// 造成致命伤害
			FDamageEvent DamageEvent;
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			Enemy->TakeDamage(9999.0f, DamageEvent, PC, PC ? PC->GetPawn() : nullptr);
			KilledCount++;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Killed %d NPCs"), KilledCount);
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
}

void UTestCheatManager::HealAllNPCs()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] No valid world!"));
		return;
	}

	int32 HealedCount = 0;
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ACombatEnemy::StaticClass(), FoundActors);

	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Healing all NPCs"));

	for (AActor* Actor : FoundActors)
	{
		if (ACombatEnemy* Enemy = Cast<ACombatEnemy>(Actor))
		{
			// 使用 ApplyHealing 接口
			Enemy->ApplyHealing(9999.0f, nullptr);
			HealedCount++;
			UE_LOG(LogTemp, Log, TEXT("[CheatManager] ✅ Healed %s"), *Enemy->GetName());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Healed %d NPCs"), HealedCount);
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
}

void UTestCheatManager::ListAllNPCs()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] No valid world!"));
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ACombatEnemy::StaticClass(), FoundActors);

	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Listing all NPCs (%d found)"), FoundActors.Num());
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));

	int32 Index = 1;
	for (AActor* Actor : FoundActors)
	{
		if (ACombatEnemy* Enemy = Cast<ACombatEnemy>(Actor))
		{
			FVector Location = Enemy->GetActorLocation();
			
			// 尝试获取 AI Controller 来显示更多信息
			AUtilityAIController* AIController = Cast<AUtilityAIController>(Enemy->GetController());
			
			UE_LOG(LogTemp, Warning, TEXT("[%d] %s"), Index, *Enemy->GetName());
			UE_LOG(LogTemp, Log, TEXT("    Location: X=%.0f Y=%.0f Z=%.0f"), 
				   Location.X, Location.Y, Location.Z);
			
			if (AIController)
			{
				UE_LOG(LogTemp, Log, TEXT("    Controller: %s"), *AIController->GetName());
			}
			
			Index++;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
}

void UTestCheatManager::DamageNPC(const FString& NPCName, float Damage)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] No valid world!"));
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ACombatEnemy::StaticClass(), FoundActors);

	bool bFound = false;
	for (AActor* Actor : FoundActors)
	{
		if (ACombatEnemy* Enemy = Cast<ACombatEnemy>(Actor))
		{
			// 部分匹配名称
			if (Enemy->GetName().Contains(NPCName))
			{
				FDamageEvent DamageEvent;
				APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
				float ActualDamage = Enemy->TakeDamage(Damage, DamageEvent, PC, PC ? PC->GetPawn() : nullptr);
				
				UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Damaged %s with %.1f damage"), 
					   *Enemy->GetName(), ActualDamage);
				bFound = true;
			}
		}
	}

	if (!bFound)
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] No NPC found matching '%s'"), *NPCName);
	}
}

void UTestCheatManager::TestLLMConnection()
{
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Testing LLM Connection..."));
	UE_LOG(LogTemp, Warning, TEXT("═══════════════════════════════════════"));

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] No valid world!"));
		return;
	}

	// 查找第一个 UtilityAIController
	TArray<AActor*> FoundControllers;
	UGameplayStatics::GetAllActorsOfClass(World, AUtilityAIController::StaticClass(), FoundControllers);

	if (FoundControllers.Num() > 0)
	{
		if (AUtilityAIController* AIController = Cast<AUtilityAIController>(FoundControllers[0]))
		{
			UE_LOG(LogTemp, Log, TEXT("[CheatManager] Found AI Controller: %s"), *AIController->GetName());
			UE_LOG(LogTemp, Log, TEXT("[CheatManager] Triggering test damage to test LLM..."));
			
			// 触发测试伤害
			//AIController->TestTakeDamage(25.0f);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] No UtilityAIController found!"));
	}
}

void UTestCheatManager::TriggerEmotion(const FString& NPCName, const FString& EmotionType)
{
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] TriggerEmotion: %s -> %s"), *NPCName, *EmotionType);
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] This feature is not yet implemented"));
	
	// TODO: 实现情绪触发功能
	// 可以通过直接修改 MentalState 来实现
}
