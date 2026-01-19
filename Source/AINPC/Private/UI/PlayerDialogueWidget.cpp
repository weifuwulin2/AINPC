#include "UI/PlayerDialogueWidget.h"

void UPlayerDialogueWidget::SubmitMessage(const FString& Message)
{
	if (!Message.IsEmpty())
	{
		OnMessageSubmitted.Broadcast(Message);
	}
}
