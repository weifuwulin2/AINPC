// Copyright Epic Games, Inc. All Rights Reserved.


#include "AINPCPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "AINPC.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "UI/PlayerDialogueWidget.h"
#include "Components/SensoryComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

void AAINPCPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogAINPC, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AAINPCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void AAINPCPlayerController::ToggleChat()
{
    if (IsLocalPlayerController())
    {
        if (!ChatWidgetInstance && ChatWidgetClass)
        {
            ChatWidgetInstance = CreateWidget<UPlayerDialogueWidget>(this, ChatWidgetClass);
            if (ChatWidgetInstance)
            {
                ChatWidgetInstance->AddToViewport(10); // High Z-Order
                ChatWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
                
                // Bind Delegate
                ChatWidgetInstance->OnMessageSubmitted.AddDynamic(this, &AAINPCPlayerController::HandleChatMessage);
            }
        }

        if (ChatWidgetInstance)
        {
            bool bIsVisible = ChatWidgetInstance->GetVisibility() == ESlateVisibility::Visible;
            if (bIsVisible)
            {
                // Hide and Return to Game
                ChatWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
                FInputModeGameOnly InputMode;
                SetInputMode(InputMode);
                SetShowMouseCursor(false);
            }
            else
            {
                // Show and Focus UI (use GameAndUI so player can still press T to close)
                ChatWidgetInstance->SetVisibility(ESlateVisibility::Visible);
                FInputModeGameAndUI InputMode;
                InputMode.SetWidgetToFocus(ChatWidgetInstance->TakeWidget());
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                InputMode.SetHideCursorDuringCapture(false);
                SetInputMode(InputMode);
                SetShowMouseCursor(true);
            }
        }
    }
}

void AAINPCPlayerController::HandleChatMessage(const FString& Message)
{
    if (Message.IsEmpty()) return;

    // 1. Log Player's Speech
    UE_LOG(LogTemp, Log, TEXT("[Player] Said: %s"), *Message);

    // 2. Hide UI
    ToggleChat(); 

    // 3. Propagate to nearby NPCs
    APawn* MyPawn = GetPawn();
    if (!MyPawn) return;

    float SpeakRangeSq = 1500.0f * 1500.0f; // 15 meters

    APawn* BestTarget = nullptr;
    float BestScore = -FLT_MAX;
    float BestDistSq = FLT_MAX;

    FVector PlayerLoc = MyPawn->GetActorLocation();
    FVector PlayerLookDir = GetControlRotation().Vector();

    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        APawn* NPC = *It;
        if (NPC == MyPawn) continue;
        if (!IsValid(NPC) || NPC->IsPendingKillPending()) continue;
        if (NPC->ActorHasTag("Dead")) continue;

        AController* NPCCon = NPC->GetController();
        if (!NPCCon) continue;
        if (!NPCCon->FindComponentByClass<USensoryComponent>()) continue;

        FVector NPCLoc = NPC->GetActorLocation();
        float DistSq = FVector::DistSquared(PlayerLoc, NPCLoc);

        if (DistSq < SpeakRangeSq)
        {
            // Score: prefer NPCs the player is looking at, but don't require it
            FVector ToNPC = (NPCLoc - PlayerLoc).GetSafeNormal();
            float Dot = FVector::DotProduct(PlayerLookDir, ToNPC);

            // Score = direction bonus (0~1) - distance penalty (0~1)
            float NormDist = FMath::Sqrt(DistSq) / 1500.0f;
            float Score = Dot * 0.6f + (1.0f - NormDist) * 0.4f;

            if (Score > BestScore)
            {
                BestScore = Score;
                BestDistSq = DistSq;
                BestTarget = NPC;
            }
        }
    }

    if (BestTarget)
    {
         if (AController* NPCController = BestTarget->GetController())
         {
             if (USensoryComponent* Sensory = NPCController->FindComponentByClass<USensoryComponent>())
             {
                 Sensory->ReceiveSpeech(MyPawn, Message);
                 UE_LOG(LogTemp, Warning, TEXT("[Chat] 🗣️ Sent message to: %s (Dist: %.1fm, Score: %.2f)"),
                     *BestTarget->GetName(), FMath::Sqrt(BestDistSq)/100.0f, BestScore);
             }
         }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Chat] ❌ No valid NPC found near player (Range 15m)."));
    }
}
