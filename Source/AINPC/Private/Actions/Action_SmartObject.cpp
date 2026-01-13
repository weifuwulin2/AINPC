#include "Actions/Action_SmartObject.h"
#include "Controller/UtilityAIController.h"
#include "Components/SensoryComponent.h"
#include "Components/SmartObjectComponent.h"
#include "Social/SocialGameplayTags.h"
#include "Navigation/PathFollowingComponent.h"

void UAction_SmartObject::Enter_Implementation(AAIController* Controller)
{
	if (!Controller) return;

	TargetSmartObject = nullptr;
	bIsInteracting = false;
	ActionStartTime = 0.0f;

	// 1. Get Sensory Component
	USensoryComponent* SensoryComp = Controller->FindComponentByClass<USensoryComponent>();
	if (!SensoryComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed: No SensoryComponent"), *ActionName);
		return;
	}

	// 2. Find Best Object matching our Configured Tag
	if (!SmartObjectTag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Failed: SmartObjectTag is empty! Check DataTable."), *ActionName);
		return;
	}

	TargetSmartObject = SensoryComp->FindBestSmartObject(SmartObjectTag);

	if (TargetSmartObject)
	{
		// 3. Move to it
		Controller->MoveToActor(TargetSmartObject, 50.0f); // 50cm acceptance radius
		UE_LOG(LogTemp, Log, TEXT("[%s] Moving to Smart Object: %s"), *ActionName, *TargetSmartObject->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] No valid Smart Object found for tag: %s"), *ActionName, *SmartObjectTag.ToString());
		// Ideally, we should fail or exit here, but UtilityAI usually just runs Execute next.
	}
}

void UAction_SmartObject::Execute_Implementation(AAIController* Controller)
{
	if (!Controller || !TargetSmartObject) return;

	// 1. Check if reached
	EPathFollowingStatus::Type Status = Controller->GetMoveStatus();
	
	// If Idle or Waiting, we might be there
	if (Status == EPathFollowingStatus::Idle)
	{
		// Double check distance to be sure
		float DistSq = FVector::DistSquared(Controller->GetPawn()->GetActorLocation(), TargetSmartObject->GetActorLocation());
		if (DistSq < 150.0f * 150.0f) // Within 1.5m
		{
			if (!bIsInteracting)
			{
				// ✅ 刚到达，开始交互 / Just arrived, start interaction
				bIsInteracting = true;
				ActionStartTime = Controller->GetWorld()->GetTimeSeconds();

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
			
			// 2. Perform Restore (Simulate Interaction)
			// In a real game, potential animation trigger here
			
			float DeltaTime = Controller->GetWorld()->GetDeltaSeconds();
			RestoreStats(Controller, DeltaTime);

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
			Controller->MoveToActor(TargetSmartObject, 50.0f);
		}
	}
}

void UAction_SmartObject::Exit_Implementation(AAIController* Controller)
{
	bIsInteracting = false;
	
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

	TargetSmartObject = nullptr;
	if (Controller)
	{
		Controller->StopMovement();
	}
}

bool UAction_SmartObject::ShouldExit(AAIController* Controller) const
{
	// 如果没有设置持续时长，永不主动退出
	// If no duration is set, never exit on its own
	if (ActionDuration <= 0.0f)
	{
		return false;
	}

	// 如果还没开始交互，不退出
	// If not yet interacting, don't exit
	if (!bIsInteracting)
	{
		return false;
	}

	// 检查是否超过持续时长
	// Check if duration has expired
	if (Controller && Controller->GetWorld())
	{
		float ElapsedTime = Controller->GetWorld()->GetTimeSeconds() - ActionStartTime;
		return ElapsedTime >= ActionDuration;
	}

	return false;
}

void UAction_SmartObject::RestoreStats(AAIController* Controller, float DeltaTime)
{
	AUtilityAIController* UtilController = Cast<AUtilityAIController>(Controller);
	if (!UtilController) return;

	// ✅ ARCHITECTURE FIX: Access MentalState directly from the Controller (Source of Truth)
	// Instead of depending on MetabolismComponent to find the state.
	UNPCMentalState* State = UtilController->MentalState;
	if (!State) return;

	// ✅ NEW: Read RestoreValue from the SmartObjectComponent
	float RestoreRate = 0.2f; // Default fallback
	if (TargetSmartObject)
	{
		if (USmartObjectComponent* SmartComp = TargetSmartObject->FindComponentByClass<USmartObjectComponent>())
		{
			RestoreRate = SmartComp->RestoreValue;
		}
	}

	if (SmartObjectTag == AINPCTags::Activity_Eat)
	{
		// Eat -> Decrease Hunger (Hunger = 饥饿度，吃饭减少饥饿度)
		// Eat -> Decrease Hunger (Hunger = hunger level, eating decreases hunger)
		State->Hunger = FMath::Clamp(State->Hunger - (RestoreRate * DeltaTime), 0.0f, 1.0f);
		UE_LOG(LogTemp, Log, TEXT("[%s] Eating... Hunger: %.2f (Rate: %.2f/s)"), *ActionName, State->Hunger, RestoreRate);
	}
	else if (SmartObjectTag == AINPCTags::Activity_Rest)
	{
		// Sleep -> Decrease Fatigue (Fatigue = 疲劳度，睡觉减少疲劳度)
		// Sleep -> Decrease Fatigue (Fatigue = fatigue level, sleeping decreases fatigue)
		State->Fatigue = FMath::Clamp(State->Fatigue - (RestoreRate * DeltaTime), 0.0f, 1.0f);
		UE_LOG(LogTemp, Log, TEXT("[%s] Sleeping... Fatigue: %.2f (Rate: %.2f/s)"), *ActionName, State->Fatigue, RestoreRate);
	}
    // Add more cases here as needed (e.g. Socializing -> Reduce Loneliness)
}

