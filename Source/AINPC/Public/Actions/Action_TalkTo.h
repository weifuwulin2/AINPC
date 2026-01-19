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
	
	// Findings nearest friendly/neutral unit
	AActor* FindBestTalkTarget(AAIController* Controller);
};
