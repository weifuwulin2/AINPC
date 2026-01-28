#include "Actions/Action_SmartObject.h"
#include "Social/SocialGameplayTags.h"
#include "Controller/UtilityAIController.h"
#include "Components/SensoryComponent.h"
#include "Components/SmartObjectComponent.h"
#include "Subsystems/SmartObjectManager.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "AINPC.h"

// Helper: Project a location to the NavMesh
static FVector ProjectToNavMesh(UWorld* World, const FVector& Location, float QueryExtent = 500.0f)
{
	if (!World) return Location;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys) return Location;

	FNavLocation NavLocation;
	const FVector QueryExtentVec(QueryExtent, QueryExtent, QueryExtent);
	
	if (NavSys->ProjectPointToNavigation(Location, NavLocation, QueryExtentVec))
	{
		return NavLocation.Location;
	}

	// Fallback: Try with larger extent for very underground objects
	const FVector LargeExtent(1000.0f, 1000.0f, 1000.0f);
	if (NavSys->ProjectPointToNavigation(Location, NavLocation, LargeExtent))
	{
		AINPC_LOG(Warning, "Had to use large query extent to find NavMesh for location %s", *Location.ToString());
		return NavLocation.Location;
	}

	AINPC_LOG(Error, "Failed to project location %s to NavMesh!", *Location.ToString());
	return Location;
}

void UAction_SmartObject::Enter_Implementation(AAIController* Controller)
{
	// ✅ Call base class to set ActionStartTime and CommitmentStartTime
	Super::Enter_Implementation(Controller);

	if (!Controller) return;

	TargetSmartObject = nullptr;
	bIsInteracting = false;
	ReservedSlotIndex = -1;

	// ✅ Reset Retry Counters
	LastMoveRetryTime = -100.0f;
	MoveRetryCount = 0;

	// 1. Get SmartObject Manager
	USmartObjectManager* SmartObjectMgr = nullptr;
	if (UWorld* World = Controller->GetWorld())
	{
		SmartObjectMgr = World->GetSubsystem<USmartObjectManager>();
	}

	if (!SmartObjectMgr) 
	{
		AINPC_LOG_ERROR("SmartObjectManager not found!");
		return;
	}

	// 2. Find Best Object matching our Configured Tag
	if (!SmartObjectTag.IsValid())
	{
		AINPC_LOG_ERROR("SmartObjectTag is empty! Check DataTable.");
		return;
	}

	// Use Manager to find object with available slot
	TargetSmartObject = SmartObjectMgr->FindBestSmartObject(Controller->GetPawn(), SmartObjectTag);

	if (TargetSmartObject)
	{
		// 3. ✅ NEW: Try Reserve a SLOT (not the whole object)
		USmartObjectComponent* SmartComp = TargetSmartObject->FindComponentByClass<USmartObjectComponent>();
		if (SmartComp)
		{
			ReservedSlotIndex = SmartComp->TryReserveSlot(Controller->GetPawn());
			if (ReservedSlotIndex >= 0)
			{
				ReservedResource = TargetSmartObject;
				AINPC_LOG(Log, "[%s] Reserved Slot %d on %s", *ActionName, ReservedSlotIndex, *ReservedResource->GetName());
				
				// 4. Move to the reserved slot location (PROJECT TO NAVMESH)
				FVector TargetLoc = SmartComp->GetInteractionLocation(ReservedSlotIndex);
				TargetLoc = ProjectToNavMesh(Controller->GetWorld(), TargetLoc);
				
				Controller->MoveToLocation(TargetLoc, 100.0f, true, true, true);
				AINPC_LOG(Log, "[%s] Moving to Slot %d at: %s", *ActionName, ReservedSlotIndex, *TargetLoc.ToString());
				
				// Initialize MoveRetryTime
				LastMoveRetryTime = Controller->GetWorld()->GetTimeSeconds();
				
				// 5. Setup Recovery Timer
				SetupRecoveryTimer(Controller);
			}
			else
			{
				AINPC_LOG(Warning, "[%s] Failed to reserve slot on %s (All occupied?)", *ActionName, *TargetSmartObject->GetName());
				TargetSmartObject = nullptr;
			}
		}
		else
		{
			// Fallback: Legacy reservation (no SmartObjectComponent)
			if (SmartObjectMgr->TryReserveSmartObject(TargetSmartObject, Controller->GetPawn()))
			{
				ReservedResource = TargetSmartObject;
				Controller->MoveToActor(ReservedResource, 150.0f);
				AINPC_LOG(Log, "[%s] (Legacy) Reserved and moving to: %s", *ActionName, *ReservedResource->GetName());
				LastMoveRetryTime = Controller->GetWorld()->GetTimeSeconds();
				SetupRecoveryTimer(Controller);
			}
			else
			{
				AINPC_LOG(Warning, "[%s] Failed to reserve resource: %s", *ActionName, *TargetSmartObject->GetName());
				TargetSmartObject = nullptr;
			}
		}
	}
	else
	{
		AINPC_LOG(Warning, "[%s] No valid unreserved Smart Object found for tag: %s", *ActionName, *SmartObjectTag.ToString());
	}
}

void UAction_SmartObject::Execute_Implementation(AAIController* Controller)
{
	if (!Controller) return;

	// Determine Target Destination (use slot location if reserved)
	FVector Destination = FVector::ZeroVector;
	
	if (TargetSmartObject)
	{
		USmartObjectComponent* SmartComp = TargetSmartObject->FindComponentByClass<USmartObjectComponent>();
		if (SmartComp && ReservedSlotIndex >= 0)
		{
			Destination = SmartComp->GetInteractionLocation(ReservedSlotIndex);
		}
		else
		{
			Destination = TargetSmartObject->GetActorLocation();
		}
		
		// ✅ Project to NavMesh to avoid underground targets
		Destination = ProjectToNavMesh(Controller->GetWorld(), Destination);
	}
	
	// ✅ 如果没有目标（或Retry），尝试重新查找
	// If no target, try to find one again
	if (!TargetSmartObject)
	{
		USmartObjectManager* SmartObjectMgr = nullptr;
		if (UWorld* World = Controller->GetWorld())
		{
			SmartObjectMgr = World->GetSubsystem<USmartObjectManager>();
		}

		if (SmartObjectMgr && SmartObjectTag.IsValid())
		{
			TargetSmartObject = SmartObjectMgr->FindBestSmartObject(Controller->GetPawn(), SmartObjectTag);
			
			if (TargetSmartObject)
			{
				USmartObjectComponent* SmartComp = TargetSmartObject->FindComponentByClass<USmartObjectComponent>();
				if (SmartComp)
				{
					ReservedSlotIndex = SmartComp->TryReserveSlot(Controller->GetPawn());
					if (ReservedSlotIndex >= 0)
					{
						ReservedResource = TargetSmartObject;
						Destination = SmartComp->GetInteractionLocation(ReservedSlotIndex);
						Controller->MoveToLocation(Destination, 100.0f, true, true, true);
						UE_LOG(LogTemp, Log, TEXT("[%s] (Retry) Reserved Slot %d, moving to: %s"), *ActionName, ReservedSlotIndex, *Destination.ToString());
						LastMoveRetryTime = Controller->GetWorld()->GetTimeSeconds();
						SetupRecoveryTimer(Controller);
					}
					else
					{
						TargetSmartObject = nullptr;
					}
				}
				else
				{
					// Legacy fallback
					if (SmartObjectMgr->TryReserveSmartObject(TargetSmartObject, Controller->GetPawn()))
					{
						ReservedResource = TargetSmartObject;
						Destination = TargetSmartObject->GetActorLocation();
						Controller->MoveToActor(TargetSmartObject, 200.0f);
						UE_LOG(LogTemp, Log, TEXT("[%s] (Retry Legacy) Moving to: %s"), *ActionName, *TargetSmartObject->GetName());
						LastMoveRetryTime = Controller->GetWorld()->GetTimeSeconds();
						SetupRecoveryTimer(Controller);
					}
					else
					{
						TargetSmartObject = nullptr;
					}
				}
			}
		}
		
		// 还是没找到，直接返回
		if (!TargetSmartObject) return;
	}

	// 1. Check if reached
	EPathFollowingStatus::Type Status = Controller->GetMoveStatus();
	float CurrentTime = Controller->GetWorld()->GetTimeSeconds();
	
	// 🔍 调试：显示移动状态（每 2 秒）
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
			
			// ✅ Manual Loop Fallback: If animation finished but action continues, replay it
			if (bIsInteracting && bLoopAnimation && InteractionMontage)
			{
				if (ACharacter* Character = Cast<ACharacter>(Controller->GetPawn()))
				{
					if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
					{
						if (!AnimInstance->Montage_IsPlaying(InteractionMontage))
						{
							// Only replay if duration permit (or infinite)
							float ElapsedTime = Controller->GetWorld()->GetTimeSeconds() - ActionStartTime;
							if (ActionDuration <= 0.0f || ElapsedTime < ActionDuration)
							{
								AnimInstance->Montage_Play(InteractionMontage, 1.0f);
							}
						}
					}
				}
			}

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
			// ✅ Throttle Retries: Only retry every 3.0 seconds
			if (CurrentTime - LastMoveRetryTime > 3.0f)
			{
				MoveRetryCount++;
				
				if (MoveRetryCount <= 3)
				{
					// Try with progressively larger acceptance radius
					float AcceptRadius = 100.0f + (MoveRetryCount * 50.0f);
					EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
						Destination, AcceptRadius, true, true, true);
					
					if (MoveResult == EPathFollowingRequestResult::Failed)
					{
						AINPC_LOG(Warning, "[%s] (Retry #%d) Path to %s is UNREACHABLE! Will try MoveToActor.", 
							*ActionName, MoveRetryCount, *Destination.ToString());
						
						// Fallback: Try MoveToActor instead (uses pathfinding to actor's navmesh position)
						if (TargetSmartObject)
						{
							Controller->MoveToActor(TargetSmartObject, AcceptRadius);
						}
					}
					else
					{
						AINPC_LOG(Warning, "[%s] (Retry #%d) Moving to: %s (Radius: %.0f)", 
							*ActionName, MoveRetryCount, *Destination.ToString(), AcceptRadius);
					}
				}
				else if (MoveRetryCount == 4)
				{
					// Last resort: Just move NEAR the target actor with very large radius
					if (TargetSmartObject)
					{
						Controller->MoveToActor(TargetSmartObject, 300.0f);
						AINPC_LOG(Warning, "[%s] (Retry #%d) Final attempt - moving NEAR actor with 300cm radius", 
							*ActionName, MoveRetryCount);
					}
				}
				else if (MoveRetryCount == 5)
				{
					// Give up on movement but start interaction anyway if close enough
					float Dist2D = FVector::DistXY(Controller->GetPawn()->GetActorLocation(), Destination);
					if (Dist2D < 500.0f) // Within 5 meters
						{
						AINPC_LOG(Warning, "[%s] Close enough (%.0f cm), starting interaction despite path issues", 
							*ActionName, Dist2D);
						bIsInteracting = true;
						ActionStartTime = Controller->GetWorld()->GetTimeSeconds();
						if (TargetSmartObject)
						{
							Controller->SetFocalPoint(TargetSmartObject->GetActorLocation());
						}
						SetupRecoveryTimer(Controller);
						}
					else
					{
						AINPC_LOG_ERROR("[%s] ❌ Failed to reach target after 5 retries. Too far (%.0f cm).", 
							*ActionName, Dist2D);
					}
				
					LastMoveRetryTime = CurrentTime;
				}
			}
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
	
	// ✅ Release Slot Reservation (NEW)
	if (ReservedResource && Controller)
	{
		if (USmartObjectComponent* SmartComp = ReservedResource->FindComponentByClass<USmartObjectComponent>())
		{
			if (ReservedSlotIndex >= 0)
			{
				SmartComp->ReleaseSlot(ReservedSlotIndex, Controller->GetPawn());
				UE_LOG(LogTemp, Log, TEXT("[%s] Released Slot %d on %s"), *ActionName, ReservedSlotIndex, *ReservedResource->GetName());
			}
		}
		else
		{
			// Legacy: Release via Manager
			if (UWorld* World = Controller->GetWorld())
			{
				if (USmartObjectManager* SmartObjectMgr = World->GetSubsystem<USmartObjectManager>())
				{
					SmartObjectMgr->ReleaseReservation(ReservedResource, Controller->GetPawn());
					UE_LOG(LogTemp, Log, TEXT("[%s] (Legacy) Released Reservation on %s"), *ActionName, *ReservedResource->GetName());
				}
			}
		}
	}

	TargetSmartObject = nullptr;
	ReservedResource = nullptr;
	ReservedSlotIndex = -1;
	
	if (Controller)
	{
		Controller->StopMovement();
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
	
	UE_LOG(LogTemp, Log, TEXT("[%s] Exit - Reset state"), *ActionName);
}

// =========================================================
// ✅ ShouldExit REMOVED - Now handled by Transition System
// Previous hardcoded logic (Emergency checks, Duration, Need satisfaction)
// is now data-driven via:
// - Priority (EActionPriority::Needs)
// - CommitmentTime (replaces ActionDuration lock)
// - ExitConditions (replaces Hunger < 0.2, Fatigue < 0.2 checks)
// =========================================================

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
