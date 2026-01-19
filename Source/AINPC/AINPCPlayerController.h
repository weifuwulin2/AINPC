// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AINPCPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AAINPCPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** Chat Widget Class */
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UPlayerDialogueWidget> ChatWidgetClass;

	/** Chat Widget Instance */
	UPROPERTY()
	TObjectPtr<class UPlayerDialogueWidget> ChatWidgetInstance;
    
    /** Toggle Chat UI */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleChat();

    /** Handle Message Submission */
    UFUNCTION()
    void HandleChatMessage(const FString& Message);

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

};
