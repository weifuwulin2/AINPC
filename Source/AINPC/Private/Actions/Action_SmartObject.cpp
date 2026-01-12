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
			bIsInteracting = true;
			
			// 2. Perform Restore (Simulate Interaction)
			// In a real game, potential animation trigger here
			
			float DeltaTime = Controller->GetWorld()->GetDeltaSeconds();
			RestoreStats(Controller, DeltaTime);
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
	TargetSmartObject = nullptr;
	if (Controller)
	{
		Controller->StopMovement();
	}
}

void UAction_SmartObject::RestoreStats(AAIController* Controller, float DeltaTime)
{
	AUtilityAIController* UtilController = Cast<AUtilityAIController>(Controller);
	if (!UtilController) return;

	// ✅ ARCHITECTURE FIX: Access MentalState directly from the Controller (Source of Truth)
	// Instead of depending on MetabolismComponent to find the state.
	UNPCMentalState* State = UtilController->MentalState;
	if (!State) return;

	// Rate of restoration (e.g. 0.2 per second -> 5 seconds to full)
	const float RestoreRate = 0.2f; 

	if (SmartObjectTag == AINPCTags::Activity_Eat)
	{
		// Eat -> Reduce Hunger
		State->Hunger = FMath::Clamp(State->Hunger - (RestoreRate * DeltaTime), 0.0f, 1.0f);
		UE_LOG(LogTemp, Verbose, TEXT("[%s] Eating... Hunger: %.2f"), *ActionName, State->Hunger);
	}
	else if (SmartObjectTag == AINPCTags::Activity_Rest)
	{
		// Sleep -> Reduce Fatigue (Energy)
		State->Energy = FMath::Clamp(State->Energy - (RestoreRate * DeltaTime), 0.0f, 1.0f);
		UE_LOG(LogTemp, Verbose, TEXT("[%s] Sleeping... Energy: %.2f"), *ActionName, State->Energy);
	}
    // Add more cases here as needed (e.g. Socializing -> Reduce Loneliness)
}
