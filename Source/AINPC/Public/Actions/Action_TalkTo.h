#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "Action_TalkTo.generated.h"

/**
 * Action_TalkTo (Utility Action)
 * Finds a nearby non-enemy unit and approaches them to chat.
 * Triggers LLM Cognition to generate dynamic dialogue.
 */
UCLASS()
class AINPC_API UAction_TalkTo : public UUtilityActionBase
{
	GENERATED_BODY()

public:
	UAction_TalkTo();

	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;

private:
	// Current conversation target
	UPROPERTY()
	AActor* CurrentTarget;

	float ExecutionTime;
	float LastChatTime;

	// ✅ Auto-speech timer - speaks every AutoSpeakInterval seconds if player doesn't reply
	float ConversationTimer = 0.f;
	float AutoSpeakInterval = 8.f;

	// ✅ Max conversation duration - after this, social needs are satisfied and action yields
	float MaxConversationDuration = 45.0f;
	bool bConversationSatisfied = false;

	// Track time with no valid target to avoid getting stuck
	float NoTargetTimer = 0.f;

public:
	// Reset timer when player speaks
	void ResetConversationTimer() { ConversationTimer = 0.f; }
	
	// Satisfy social needs via Interpolator to allow natural action exit
	void MarkConversationSatisfied(AAIController* Controller);

private:
	// Findings nearest friendly/neutral unit
	AActor* FindBestTalkTarget(AAIController* Controller);
};
