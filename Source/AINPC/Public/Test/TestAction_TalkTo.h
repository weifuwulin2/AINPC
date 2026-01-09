#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "TestAction_TalkTo.generated.h"

/**
 * UTAction_TalkTo
 * Finds a nearby non-enemy unit and approaches them to chat.
 */
UCLASS()
class AINPC_API UTestAction_TalkTo : public UUtilityActionBase
{
	GENERATED_BODY()

public:
	UTestAction_TalkTo();

	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;

private:
	// 当前聊天的目标
	UPROPERTY()
	AActor* CurrentTarget;

	float ExecutionTime;
	float LastChatTime;
	
	// 寻找最近的友军/中立单位
	AActor* FindBestTalkTarget(AAIController* Controller);
};
