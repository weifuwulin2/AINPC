#include "Actions/Action_TalkTo.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Components/SensoryComponent.h" 
#include "Controller/UtilityAIController.h"
#include "Components/CognitionComponent.h"
#include "UtilityAI/MentalStateInterpolation.h"
#include "UtilityAI/UNPCMentalState.h"

UAction_TalkTo::UAction_TalkTo()
{
	ActionName = "TalkTo";
	ExecutionTime = 0.0f;
	LastChatTime = -999.0f;
	CurrentTarget = nullptr;
}

void UAction_TalkTo::Enter_Implementation(AAIController* Controller)
{
	Super::Enter_Implementation(Controller);

	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		ExecutionTime = World->GetTimeSeconds();
	}
	
	CurrentTarget = FindBestTalkTarget(Controller);

	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[ACTION] TalkTo Started"));
	
	if (CurrentTarget)
	{
		UE_LOG(LogTemp, Display, TEXT("[TalkTo] Target found: %s"), *CurrentTarget->GetName());
		Controller->SetFocus(CurrentTarget);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TalkTo] No friendly target found immediately."));
	}
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
}

void UAction_TalkTo::Execute_Implementation(AAIController* Controller)
{
	Super::Execute_Implementation(Controller);

	if (!Controller) return;
	UWorld* World = Controller->GetWorld();
	if (!World) return;
	APawn* ControlledPawn = Controller->GetPawn();
	if (!ControlledPawn) return;

	// 1. Validate Target
	bool bTargetInvalid = !CurrentTarget || !IsValid(CurrentTarget) || CurrentTarget->IsPendingKillPending() || CurrentTarget->ActorHasTag("Dead");
	
	if (bTargetInvalid)
	{
		CurrentTarget = FindBestTalkTarget(Controller);
		if (CurrentTarget)
		{
			Controller->SetFocus(CurrentTarget);
			UE_LOG(LogTemp, Log, TEXT("[TalkTo] Found new target: %s"), *CurrentTarget->GetName());
		}
		else
		{
			// Nothing to do, stop and wait
			Controller->StopMovement();
			return;
		}
	}

	// 2. Movement Logic
	float Dist = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
	float ChatRange = 300.0f;

	if (Dist > ChatRange)
	{
		Controller->MoveToActor(CurrentTarget, ChatRange - 50.0f);
	}
	else
	{
		Controller->StopMovement();
		Controller->SetFocus(CurrentTarget);

		// 3. Chat Logic (Every 5 seconds)
		float CurrentTime = World->GetTimeSeconds();
		if (CurrentTime - LastChatTime > 5.0f)
		{
			LastChatTime = CurrentTime;

            // Trigger Dialogue via LLM
			if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(Controller))
			{
                // Trigger Cognition
                if (UAICon->CognitionComp)
                {
                    FString Stimulus = FString::Printf(TEXT("You are talking to %s."), *CurrentTarget->GetName());
                    UAICon->CognitionComp->ProcessStimulus(Stimulus);
                    UE_LOG(LogTemp, Log, TEXT("[Action_TalkTo] Triggered Cognition Stimulus: %s"), *Stimulus);
                }

                // Temporary Poll: Check if Speech is ready in Mental State (simple simulation since Cognition is async)
                // In a robust system, we'd bind to a 'OnSpeechReady' delegate. 
                // Currently, we rely on the MentalState update that happens shortly after.
                // Because we poll every frame in Execute, we can check UAICon->MentalState->Speech.
                
                // However, LLM takes time (seconds). 
                // So checking *immediately* after ProcessStimulus won't work for the *current* request.
                // But it might pick up the previous one or we check continuously.
			}
		}
		
		// Poll for Speech output every frame (independent of the 5s trigger)
		if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(Controller))
		{
            if (UAICon->MentalState && !UAICon->MentalState->Speech.IsEmpty())
            {
                // Emit speech
                if (UAICon->SensoryComp)
                {
                    UAICon->SensoryComp->ReceiveSpeech(ControlledPawn, UAICon->MentalState->Speech);
                }
                
                UE_LOG(LogTemp, Log, TEXT("[Action_TalkTo] Said: %s"), *UAICon->MentalState->Speech);
                
                // Clear speech to avoid loop
                UAICon->MentalState->Speech = "";
                
                 // Reduce Social Need (Loneliness) significantly after speaking
                 if (UAICon->MentalState)
                 {
                     UAICon->MentalState->Loneliness = FMath::Max(0.0f, UAICon->MentalState->Loneliness - 0.2f);
                     UAICon->MentalState->Boredom = FMath::Max(0.0f, UAICon->MentalState->Boredom - 0.1f);
                     
                     // Sync Interpolator
                     if (UAICon->CognitionComp && UAICon->CognitionComp->Interpolator)
                     {
                         UAICon->CognitionComp->Interpolator->SetTargetValue(TEXT("Loneliness"), UAICon->MentalState->Loneliness);
                         UAICon->CognitionComp->Interpolator->SetTargetValue(TEXT("Boredom"), UAICon->MentalState->Boredom);
                     }
                 }
            }
		}
	}
}

void UAction_TalkTo::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);
	
	if (Controller)
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
		Controller->StopMovement();
	}
	
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[ACTION] TalkTo Exited"));
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
}

AActor* UAction_TalkTo::FindBestTalkTarget(AAIController* Controller)
{
	if (!Controller || !Controller->GetWorld()) return nullptr;
	APawn* MyPawn = Controller->GetPawn();
	if (!MyPawn) return nullptr;

	AActor* BestCandidate = nullptr;
	float ClosestDistSq = FLT_MAX;

	for (TActorIterator<ACharacter> It(Controller->GetWorld()); It; ++It)
	{
		ACharacter* Candidate = *It;
		
		if (Candidate == MyPawn) continue;
		if (!IsValid(Candidate) || Candidate->IsPendingKillPending()) continue;
        if (Candidate->ActorHasTag("Dead")) continue;
        if (Candidate->ActorHasTag("Enemy")) continue; 
		if (Candidate->GetMesh() && Candidate->GetMesh()->IsSimulatingPhysics()) continue;

		float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			BestCandidate = Candidate;
		}
	}

	return BestCandidate;
}
