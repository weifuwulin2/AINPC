#include "Actions/Action_SmartObject.h"
#include "Social/SocialGameplayTags.h" // ✅ Use Canonical Tags
#include "Controller/UtilityAIController.h"
#include "Components/SensoryComponent.h"
#include "Components/SmartObjectComponent.h"
#include "Subsystems/SmartObjectManager.h"
#include "Social/SocialGameplayTags.h"
#include "Navigation/PathFollowingComponent.h"

void UAction_SmartObject::Enter_Implementation(AAIController* Controller)
{
	if (!Controller) return;

	TargetSmartObject = nullptr;
	bIsInteracting = false;
	ActionStartTime = 0.0f;

	// 1. Get SmartObject Manager
	USmartObjectManager* SmartObjectMgr = nullptr;
	if (UWorld* World = Controller->GetWorld())
	{
		SmartObjectMgr = World->GetSubsystem<USmartObjectManager>();
	}

	if (!SmartObjectMgr) 
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Failed: SmartObjectManager not found!"), *ActionName);
		return;
	}

	// 2. Find Best Object matching our Configured Tag
	if (!SmartObjectTag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Failed: SmartObjectTag is empty! Check DataTable."), *ActionName);
		return;
	}

	// Use Manager to find Unreserved object
	TargetSmartObject = SmartObjectMgr->FindBestSmartObject(Controller->GetPawn(), SmartObjectTag);

	if (TargetSmartObject)
	{
		// 3. Try Reserve
		if (SmartObjectMgr->TryReserveSmartObject(TargetSmartObject, Controller->GetPawn()))
		{
			ReservedResource = TargetSmartObject;
			UE_LOG(LogTemp, Log, TEXT("[%s] Reserved Resource: %s"), *ActionName, *ReservedResource->GetName());
			
			// 4. Move to it
			USmartObjectComponent* SmartComp = ReservedResource->FindComponentByClass<USmartObjectComponent>();
			bool bUsedOffset = false;
			
			if (SmartComp && !SmartComp->InteractionOffset.IsNearlyZero())
			{
				FVector TargetLoc = SmartComp->GetInteractionLocation();
				Controller->MoveToLocation(TargetLoc, 100.0f, true, true, true); // Radius 100cm, StopOnOverlap, Pathfinding, ProjectToNav
				UE_LOG(LogTemp, Log, TEXT("[%s] Moving to Interaction Point: %s (Offset: %s)"), 
					   *ActionName, *TargetLoc.ToString(), *SmartComp->InteractionOffset.ToString());
				bUsedOffset = true;
			}
			else
			{
				Controller->MoveToActor(ReservedResource, 150.0f); // 1.5m radius
				UE_LOG(LogTemp, Log, TEXT("[%s] Moving to Smart Object: %s"), *ActionName, *ReservedResource->GetName());
			}
			
			if (EPathFollowingRequestResult::RequestSuccessful == Controller->GetMoveStatus())
			{
				// Log handled above
			}
			
			// ✅ 5. Setup Recovery Timer (1 second interval)
			SetupRecoveryTimer(Controller);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to reserve resource: %s (Was taken?)"), *ActionName, *TargetSmartObject->GetName());
			TargetSmartObject = nullptr; // Reset to fail gracefully
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] No valid unreserved Smart Object found for tag: %s"), *ActionName, *SmartObjectTag.ToString());
	}
}

void UAction_SmartObject::Execute_Implementation(AAIController* Controller)
{
	if (!Controller) return;

	// Determine Target Destination (Actor or Offset)
	FVector Destination = FVector::ZeroVector;
	bool bUseOffset = false;
	
	if (TargetSmartObject)
	{
		Destination = TargetSmartObject->GetActorLocation();
		if (USmartObjectComponent* SmartComp = TargetSmartObject->FindComponentByClass<USmartObjectComponent>())
		{
			if (!SmartComp->InteractionOffset.IsNearlyZero())
			{
				Destination = SmartComp->GetInteractionLocation();
				bUseOffset = true;
			}
		}
	}
	
	// ✅ 如果没有目标（或Retry），尝试重新查找
	// If no target, try to find one again
	if (!TargetSmartObject)
	{
		USensoryComponent* SensoryComp = Controller->FindComponentByClass<USensoryComponent>();
		if (SensoryComp && SmartObjectTag.IsValid())
		{
			TargetSmartObject = SensoryComp->FindBestSmartObject(SmartObjectTag);
			if (TargetSmartObject)
			{
				// Recalculate destination for new target
				Destination = TargetSmartObject->GetActorLocation();
				bUseOffset = false;
				if (USmartObjectComponent* SmartComp = TargetSmartObject->FindComponentByClass<USmartObjectComponent>())
				{
					if (!SmartComp->InteractionOffset.IsNearlyZero())
					{
						Destination = SmartComp->GetInteractionLocation();
						bUseOffset = true;
					}
				}

				if (bUseOffset)
				{
					Controller->MoveToLocation(Destination, 100.0f, true, true, true);
					UE_LOG(LogTemp, Log, TEXT("[%s] (Retry) Moving to Interaction Point: %s"), *ActionName, *Destination.ToString());
				}
				else
				{
					Controller->MoveToActor(TargetSmartObject, 200.0f); 
					UE_LOG(LogTemp, Log, TEXT("[%s] (Retry) Moving to Smart Object: %s"), *ActionName, *TargetSmartObject->GetName());
				}
			}
		}
		
		// 还是没找到，直接返回
		if (!TargetSmartObject) return;
	}

	// 1. Check if reached
	EPathFollowingStatus::Type Status = Controller->GetMoveStatus();
	
	// 🔍 调试：显示移动状态（每 2 秒）
	float CurrentTime = Controller->GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastDebugTime > 2.0f)
	{
		// ✅ 使用 2D 距离 (忽略高度 Z 轴差异)
		float DistSq2D = FVector::DistSquaredXY(Controller->GetPawn()->GetActorLocation(), Destination);
		float Dist2D = FMath::Sqrt(DistSq2D);
		UE_LOG(LogTemp, Log, TEXT("[%s] MoveStatus: %d, Dist2D: %.0f cm (Ignored Z), bIsInteracting: %s"), 
		       *ActionName, (int)Status, Dist2D, bIsInteracting ? TEXT("Yes") : TEXT("No"));
		LastDebugTime = CurrentTime;
	}
	
	// If Idle or Waiting, we might be there
	if (Status == EPathFollowingStatus::Idle)
	{
		// Double check distance to be sure (使用 200cm = 2m)
		// Use Destination which accounts for Offset
		// ✅ CHANGE: Use DistSquaredXY to ignore height differences (e.g. object underground)
		float DistSq2D = FVector::DistSquaredXY(Controller->GetPawn()->GetActorLocation(), Destination);
		if (DistSq2D < 200.0f * 200.0f) // Within 2m horizontally
		{
			if (!bIsInteracting)
			{
				// ✅ 刚到达，开始交互 / Just arrived, start interaction
				bIsInteracting = true;
				ActionStartTime = Controller->GetWorld()->GetTimeSeconds();

				// ✅ 设定朝向：交互时必须朝向 Smart Object (例如挖矿时面朝矿石)
				// Set facing: Must face the Smart Object during interaction
				if (TargetSmartObject)
				{
					Controller->SetFocalPoint(TargetSmartObject->GetActorLocation());
				}

				// ✅ 确保 Timer 已设置（可能 Enter 失败了）
				// Ensure timer is set up (Enter might have failed)
				if (!RecoveryTimerHandle.IsValid())
				{
					SetupRecoveryTimer(Controller);
				}

				// ✅ 播放动画 / Play animation
				if (InteractionMontage && Controller->GetPawn())
				{
					if (ACharacter* Character = Cast<ACharacter>(Controller->GetPawn()))
					{
						if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
						{
							// 播放 Montage
							AnimInstance->Montage_Play(InteractionMontage, 1.0f);
							
							// 如果需要循环
							if (bLoopAnimation)
							{
								AnimInstance->Montage_SetNextSection(FName("Default"), FName("Default"), InteractionMontage);
							}

							UE_LOG(LogTemp, Log, TEXT("[%s] Playing animation: %s (Loop: %s)"), 
								   *ActionName, *InteractionMontage->GetName(), bLoopAnimation ? TEXT("Yes") : TEXT("No"));
						}
					}
				}
			}
			
			// 2. Timer handles recovery now (no need for per-frame calls)
			// Recovery is handled by the 1-second timer set up in Enter

			// 3. 检查是否超过持续时长 / Check if duration expired
			if (ActionDuration > 0.0f)
			{
				float ElapsedTime = Controller->GetWorld()->GetTimeSeconds() - ActionStartTime;
				if (ElapsedTime >= ActionDuration)
				{
					UE_LOG(LogTemp, Log, TEXT("[%s] Duration expired (%.1fs), action should end soon"), 
						   *ActionName, ElapsedTime);
					// Note: Utility AI 会在下一帧重新评估并可能切换动作
					// The Utility AI will re-evaluate next frame and may switch actions
				}
			}
		}
		else
		{
			// Maybe got stuck? Retry move?
			if (bUseOffset)
				Controller->MoveToLocation(Destination, 50.0f, true, true, true);
			else
				Controller->MoveToActor(TargetSmartObject, 50.0f);
		}
	}
}

void UAction_SmartObject::Exit_Implementation(AAIController* Controller)
{
	bIsInteracting = false;
	
	// ✅ Clear Recovery Timer
	if (Controller && Controller->GetWorld())
	{
		Controller->GetWorld()->GetTimerManager().ClearTimer(RecoveryTimerHandle);
	}
	
	// ✅ 停止动画 / Stop animation
	if (InteractionMontage && Controller && Controller->GetPawn())
	{
		if (ACharacter* Character = Cast<ACharacter>(Controller->GetPawn()))
		{
			if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
			{
				if (AnimInstance->Montage_IsPlaying(InteractionMontage))
				{
					AnimInstance->Montage_Stop(0.2f, InteractionMontage);
					UE_LOG(LogTemp, Log, TEXT("[%s] Stopped animation: %s"), 
						   *ActionName, *InteractionMontage->GetName());
				}
			}
		}
	}

	// ✅ 重置状态 / Reset state
	bIsInteracting = false;
	
	// Release Reservation
	if (ReservedResource && Controller)
	{
		if (UWorld* World = Controller->GetWorld())
		{
			if (USmartObjectManager* SmartObjectMgr = World->GetSubsystem<USmartObjectManager>())
			{
				SmartObjectMgr->ReleaseReservation(ReservedResource, Controller->GetPawn());
				UE_LOG(LogTemp, Log, TEXT("[%s] Released Reservation on %s"), *ActionName, *ReservedResource->GetName());
			}
		}
	}

	TargetSmartObject = nullptr;
	ReservedResource = nullptr;
	
	if (Controller)
	{
		Controller->StopMovement();
		Controller->ClearFocus(EAIFocusPriority::Gameplay); // Clear the focus we set on arrival
	}
	
	UE_LOG(LogTemp, Log, TEXT("[%s] Exit - Reset state"), *ActionName);
}

bool UAction_SmartObject::ShouldExit(AAIController* Controller) const
{
	// ✅ 0. 紧急情况检查（Critical Needs - Always Allow Exit）
	// Emergency check - allow exit even during ActionDuration
	AUtilityAIController* UtilController = Cast<AUtilityAIController>(Controller);
	if (UtilController && UtilController->MentalState)
	{
		UNPCMentalState* State = UtilController->MentalState;
		
		// Critical hunger, fatigue, or threat - allow immediate exit
		if (State->Hunger > 0.8f || State->Fatigue > 0.8f || State->Perceived_Threat > 0.5f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] ⚠️ EMERGENCY EXIT: Hunger=%.2f, Fatigue=%.2f, Threat=%.2f"), 
			       *ActionName, State->Hunger, State->Fatigue, State->Perceived_Threat);
			return true;  // Emergency exit allowed
		}
	}

	// ✅ 1. 持续时长检查（仅在交互时阻止切换）
	// Duration check (only block switching when actually interacting)
	if (ActionDuration > 0.0f)
	{
		// 如果还没开始交互（正在走路），允许切换到更好的动作
		// If not yet interacting (still moving), allow switching to better action
		if (!bIsInteracting)
		{
			return true;  // Allow switching during movement phase
		}
		
		// 正在交互中，检查是否超过持续时长
		// Currently interacting, check if duration expired
		if (Controller && Controller->GetWorld())
		{
			float ElapsedTime = Controller->GetWorld()->GetTimeSeconds() - ActionStartTime;
			// 只有时长到期才退出
			return ElapsedTime >= ActionDuration;
		}
		return false;
	}
	
	// ✅ 2. 需求满足检查（仅当没有设置 Duration 时）
	// Need satisfaction check (only when Duration is not set)
	// Reuse UtilController from emergency check above
	if (UtilController && UtilController->MentalState)
	{
		UNPCMentalState* State = UtilController->MentalState;
		
		// Eat -> Check Hunger
		if (SmartObjectTag == AINPCTags::Interaction_Eat)
		{
			if (State->Hunger < 0.2f) // Well-fed threshold
			{
				return true;
			}
		}
		// Sleep -> Check Fatigue
		else if (SmartObjectTag == AINPCTags::Interaction_Rest)
		{
			if (State->Fatigue < 0.2f) // Well-rested threshold
			{
				return true;
			}
		}
		// 针对工作类动作，增加惯性 (Add inertia for work actions)
		else if (SmartObjectTag.MatchesTag(AINPCTags::Interaction_Work))
		{
			// 工作持续时间较长，容易被打断，增加惯性保护
			if (ActionDuration > 0.0f && Controller && Controller->GetWorld())
			{
				float ElapsedTime = Controller->GetWorld()->GetTimeSeconds() - ActionStartTime;
				// 如果还没工作够时长，给予惯性加成 (e.g., allow exit only after 50% of duration)
				if (ElapsedTime < ActionDuration * 0.5f) // Example: Don't exit if less than half duration
				{
					return false; // Prevent exit due to inertia
				}
			}
		}
	}

	return false;
}

void UAction_SmartObject::SetupRecoveryTimer(AAIController* Controller)
{
	if (!Controller || !Controller->GetWorld()) return;

	// ✅ Read RestoreValue from SmartObject
	float RestoreRate = 0.2f; // Default fallback
	if (TargetSmartObject)
	{
		if (USmartObjectComponent* SmartComp = TargetSmartObject->FindComponentByClass<USmartObjectComponent>())
		{
			RestoreRate = SmartComp->RestoreValue;
			UE_LOG(LogTemp, Warning, TEXT("[%s] ✅ Read RestoreValue from SmartObject '%s': %.2f/s"), 
			       *ActionName, *TargetSmartObject->GetName(), RestoreRate);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[%s] ❌ SmartObject '%s' has NO SmartObjectComponent! Using default %.2f/s"), 
			       *ActionName, *TargetSmartObject->GetName(), RestoreRate);
		}
	}

	// ✅ Setup Timer (1 second interval, repeat)
	Controller->GetWorld()->GetTimerManager().SetTimer(
		RecoveryTimerHandle,
		[this, Controller]()
		{
			RestoreStats(Controller);
		},
		1.0f,  // 每1秒执行一次
		true   // 重复执行
	);

	UE_LOG(LogTemp, Log, TEXT("[%s] ✅ Recovery Timer Started (Rate: %.2f/s)"), *ActionName, RestoreRate);
}

void UAction_SmartObject::RestoreStats(AAIController* Controller)
{
	AUtilityAIController* UtilController = Cast<AUtilityAIController>(Controller);
	if (!UtilController) return;

	UNPCMentalState* State = UtilController->MentalState;
	if (!State) return;

	// ✅ Read RestoreValue from SmartObject
	float RestoreRate = 0.2f; // Default fallback
	if (TargetSmartObject)
	{
		if (USmartObjectComponent* SmartComp = TargetSmartObject->FindComponentByClass<USmartObjectComponent>())
		{
			RestoreRate = SmartComp->RestoreValue;
		}
	}

	// ✅ Directly apply RestoreRate (no DeltaTime multiplication needed - timer runs every 1 second)
	if (SmartObjectTag == AINPCTags::Interaction_Eat)
	{
		float OldHunger = State->Hunger;
		State->Hunger = FMath::Clamp(State->Hunger - RestoreRate, 0.0f, 1.0f);
		float Delta = OldHunger - State->Hunger;
		UE_LOG(LogTemp, Warning, TEXT("[%s] Eating... Hunger: %.2f (-%0.2f this tick, Rate: %.2f/s)"), 
		       *ActionName, State->Hunger, Delta, RestoreRate);
	}
	else if (SmartObjectTag == AINPCTags::Interaction_Rest)
	{
		float OldFatigue = State->Fatigue;
		State->Fatigue = FMath::Clamp(State->Fatigue - RestoreRate, 0.0f, 1.0f);
		float Delta = OldFatigue - State->Fatigue;
		UE_LOG(LogTemp, Warning, TEXT("[%s] Sleeping... Fatigue: %.2f (-%0.2f this tick, Rate: %.2f/s)"), 
		       *ActionName, State->Fatigue, Delta, RestoreRate);
	}
	// ✅ Work actions reduce Boredom (work provides purpose and fulfillment)
	else if (SmartObjectTag.MatchesTag(FGameplayTag::RequestGameplayTag("Interaction.Work")))
	{
		float OldBoredom = State->Boredom;
		State->Boredom = FMath::Clamp(State->Boredom - RestoreRate, 0.0f, 1.0f);
		float Delta = OldBoredom - State->Boredom;
		UE_LOG(LogTemp, Warning, TEXT("[%s] Working... Boredom: %.2f (-%0.2f this tick, Rate: %.2f/s)"), 
		       *ActionName, State->Boredom, Delta, RestoreRate);
	}
    // Add more cases here as needed (e.g. Socializing -> Reduce Loneliness)
}
