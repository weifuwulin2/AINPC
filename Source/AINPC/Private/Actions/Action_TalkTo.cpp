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

	// ✅ Mark conversation started - suppress vision events
	if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(Controller))
	{
		UAICon->bInConversation = true;
	}

	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		ExecutionTime = World->GetTimeSeconds();
	}

	// Reset conversation state
	bConversationSatisfied = false;
	ConversationTimer = 0.f;
	NoTargetTimer = 0.f;

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
			NoTargetTimer = 0.f;
			UE_LOG(LogTemp, Log, TEXT("[TalkTo] Found new target: %s"), *CurrentTarget->GetName());
		}
		else
		{
			// No valid target - track how long we've been without one
			NoTargetTimer += World->GetDeltaSeconds();
			if (NoTargetTimer > 5.0f)
			{
				// Stuck with no target for too long - satisfy needs to allow exit
				MarkConversationSatisfied(Controller);
			}
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

		// ✅ Check max conversation duration - satisfy social needs after talking long enough
		float ElapsedTime = World->GetTimeSeconds() - ExecutionTime;
		if (!bConversationSatisfied && ElapsedTime > MaxConversationDuration)
		{
			MarkConversationSatisfied(Controller);
		}

		// ✅ Auto-speech timer: only generate new speech if conversation not yet satisfied
		if (!bConversationSatisfied)
		{
			float DeltaTime = World->GetDeltaSeconds();
			ConversationTimer += DeltaTime;

			if (ConversationTimer >= AutoSpeakInterval)
			{
				ConversationTimer = 0.f;

				if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(Controller))
				{
					if (UAICon->CognitionComp)
					{
						FString Stimulus = FString::Printf(
							TEXT("You are having a friendly chat with %s. Make light small talk about everyday topics like the weather, work, local news, or how their day is going. Keep it casual and brief."),
							*CurrentTarget->GetName()
						);
						UAICon->CognitionComp->ProcessStimulus(Stimulus);
						UE_LOG(LogTemp, Log, TEXT("[Action_TalkTo] Auto-speak triggered after %.1fs"), AutoSpeakInterval);
					}
				}
			}
		}

		// Poll for Speech output every frame (independent of auto-speak trigger)
		if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(Controller))
		{
			if (UAICon->MentalState && !UAICon->MentalState->Speech.IsEmpty())
			{
				if (UAICon->SensoryComp)
				{
					UAICon->SensoryComp->ReceiveSpeech(ControlledPawn, UAICon->MentalState->Speech);
				}

				UE_LOG(LogTemp, Log, TEXT("[Action_TalkTo] Said: %s"), *UAICon->MentalState->Speech);
				UAICon->MentalState->Speech = "";
			}
		}
	}
}

void UAction_TalkTo::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);
	
	// ✅ Mark conversation ended - resume vision events
	if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(Controller))
	{
		UAICon->bInConversation = false;
	}
	
	if (Controller)
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
		Controller->StopMovement();
	}
	
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[ACTION] TalkTo Exited"));
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
}

void UAction_TalkTo::MarkConversationSatisfied(AAIController* Controller)
{
	if (bConversationSatisfied) return;
	bConversationSatisfied = true;

	// Set low Loneliness target via Interpolator.
	// The Interpolator (InterpSpeed=2.0) will smoothly reduce values over ~1-2s,
	// causing TalkTo's score to drop naturally, allowing other actions to take over.
	// NOTE: Talk only affects Loneliness. Boredom is reduced through Work activities.
	if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(Controller))
	{
		if (UAICon->CognitionComp && UAICon->CognitionComp->Interpolator && UAICon->MentalState)
		{
			UAICon->CognitionComp->Interpolator->SetTargetValue(TEXT("Loneliness"), 0.1f);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Action_TalkTo] Conversation satisfied - Loneliness reduced, yielding soon"));
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
