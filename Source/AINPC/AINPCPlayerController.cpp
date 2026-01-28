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

    float SpeakRangeSq = 1200.0f * 1200.0f; // 12 meters
    
    APawn* BestTarget = nullptr;
    float MinDistSq = FLT_MAX;
    
    FVector PlayerLoc = MyPawn->GetActorLocation();
    // Use Camera rotation (ControlRotation) to determine where player is LOOKING
    FVector PlayerLookDir = GetControlRotation().Vector(); 

    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        APawn* NPC = *It;
        if (NPC == MyPawn) continue;
        if (!IsValid(NPC) || NPC->IsPendingKillPending()) continue;
        if (NPC->ActorHasTag("Dead")) continue;

        // Filter: Must have Controller and SensoryComponent
        AController* NPCCon = NPC->GetController();
        if (!NPCCon) continue;
        if (!NPCCon->FindComponentByClass<USensoryComponent>()) continue;

        FVector NPCLoc = NPC->GetActorLocation();
        float DistSq = FVector::DistSquared(PlayerLoc, NPCLoc);
        
        if (DistSq < SpeakRangeSq)
        {
            // Direction Check: Must be roughly in front
            FVector ToNPC = (NPCLoc - PlayerLoc).GetSafeNormal();
            float Dot = FVector::DotProduct(PlayerLookDir, ToNPC);
            
            // Dot > 0.5 means within 60 degrees of center (120 degree cone)
            if (Dot > 0.5f)
            {
                // Pick closest one in the cone
                if (DistSq < MinDistSq)
                {
                    MinDistSq = DistSq;
                    BestTarget = NPC;
                }
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
                 UE_LOG(LogTemp, Log, TEXT("[Chat] 🗣️ Sent message to: %s (Dist: %.1fm)"), 
                     *BestTarget->GetName(), FMath::Sqrt(MinDistSq)/100.0f);
             }
         }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Chat] ❌ No valid NPC found in front of player (Range 12m)."));
    }
}
