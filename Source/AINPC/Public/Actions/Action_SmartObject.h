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

	// ✅ ShouldExit removed - now handled by Transition System (Priority + CommitmentTime + ExitConditions)

	// 动画配置 / Animation Configuration
	// 可在 DataTable 中配置，或者在派生 Blueprint 中设置
	// Can be configured in DataTable or set in derived Blueprint
	
	// 交互动画 Montage (例如：吃饭、睡觉动画)
	// Interaction animation montage (e.g., eating, sleeping animation)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* InteractionMontage = nullptr;

	// 是否循环播放动画 / Whether to loop the animation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bLoopAnimation = true;

	// NOTE: ActionDuration is inherited from UUtilityActionBase — do NOT redeclare here

private:
	// The object we are currently targeting
	UPROPERTY(Transient)
	AActor* TargetSmartObject;
	
	// The resource we successfully reserved (may be same as Target)
	UPROPERTY(Transient)
	AActor* ReservedResource;

	// Is the interaction currently active? (e.g. eating loop)
	bool bIsInteracting = false;

	// NOTE: ActionStartTime is inherited from UUtilityActionBase — do NOT redeclare here

	// 调试日志时间戳 / Debug log timestamp
	// 调试日志时间戳 / Debug log timestamp
	float LastDebugTime = 0.0f;

	// ✅ Move To Retry Throttling
	float LastMoveRetryTime = 0.0f;
	int32 MoveRetryCount = 0;

	// ✅ NEW: Slot-based reservation
	int32 ReservedSlotIndex = -1;

	// ✅ 恢复定时器 / Recovery Timer
	FTimerHandle RecoveryTimerHandle;

	// Helper to setup recovery timer (1 second interval)
	void SetupRecoveryTimer(AAIController* Controller);
	
	// Helper to restore stats based on the tag (called by timer)
	void RestoreStats(AAIController* Controller);
};
