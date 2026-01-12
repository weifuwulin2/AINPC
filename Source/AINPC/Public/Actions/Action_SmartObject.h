#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "Action_SmartObject.generated.h"

/**
 * Universal Action for interacting with Smart Objects based on Tags.
 * Handles Eating, Sleeping, Socializing, etc.
 */
UCLASS()
class AINPC_API UAction_SmartObject : public UUtilityActionBase
{
	GENERATED_BODY()
	
public:
	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;

private:
	// The object we are currently targeting
	UPROPERTY(Transient)
	AActor* TargetSmartObject;

	// Is the interaction currently active? (e.g. eating loop)
	bool bIsInteracting;

	// Helper to restore stats based on the tag
	void RestoreStats(AAIController* Controller, float DeltaTime);
};
