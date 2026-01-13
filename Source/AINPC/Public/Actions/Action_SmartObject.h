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

	// 检查是否应该退出（持续时长到期）
	// Check if action should exit (duration expired)
	bool ShouldExit(AAIController* Controller) const;

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

	// 动作持续时长（秒）/ Action duration in seconds
	// 0 = 无限持续，直到 Utility AI 切换到其他动作
	// 0 = Infinite duration, until Utility AI switches to another action
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float ActionDuration = 0.0f;

private:
	// The object we are currently targeting
	UPROPERTY(Transient)
	AActor* TargetSmartObject;

	// Is the interaction currently active? (e.g. eating loop)
	bool bIsInteracting;

	// 动作开始时间 / Action start time
	float ActionStartTime;

	// Helper to restore stats based on the tag
	void RestoreStats(AAIController* Controller, float DeltaTime);
};
