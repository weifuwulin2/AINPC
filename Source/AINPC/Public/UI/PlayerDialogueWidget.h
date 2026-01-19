#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerDialogueWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerSubmittedMessage, const FString&, Message);

/**
 * UPlayerDialogueWidget
 * UI for the Player to type messages to NPCs.
 */
UCLASS()
class AINPC_API UPlayerDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Event fired when player hits Enter or clicks Send
	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnPlayerSubmittedMessage OnMessageSubmitted;

	/**
	 * Called by Blueprint when Enter is pressed or Button clicked.
	 * @param Message - The text from the Input Box.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SubmitMessage(const FString& Message);
};
