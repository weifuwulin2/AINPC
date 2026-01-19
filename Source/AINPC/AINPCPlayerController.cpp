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
                // Show and Focus UI
                ChatWidgetInstance->SetVisibility(ESlateVisibility::Visible);
                FInputModeUIOnly InputMode;
                InputMode.SetWidgetToFocus(ChatWidgetInstance->TakeWidget());
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
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

    float SpeakRangeSq = 1000.0f * 1000.0f; // 10 meters

    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        APawn* NPC = *It;
        if (NPC == MyPawn) continue;

        float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), NPC->GetActorLocation());
        if (DistSq < SpeakRangeSq)
        {
            // Send to SensoryComponent if available
            // Note: FindComponentByClass handles Controllers too if we check carefully, 
            // but usually SensoryComp is on the Controller for AI.
            
            AController* NPCController = NPC->GetController();
            if (NPCController)
            {
                if (USensoryComponent* Sensory = NPCController->FindComponentByClass<USensoryComponent>())
                {
                    Sensory->ReceiveSpeech(MyPawn, Message);
                }
            }
        }
    }
}
