#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"

/**
 * UDialogueWidget
 * Base class for the Dialogue UI.
 * Handles displaying speech bubbles or chat logs from NPCs.
 */
UCLASS()
class AINPC_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 
	 * Adds a new dialogue line to the UI.
	 * @param SpeakerName - Name of the specific NPC (e.g., "Merchant")
	 * @param DialogueText - The actual speech content
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Dialogue")
	void ShowDialogue(const FString& SpeakerName, const FString& DialogueText);
};
