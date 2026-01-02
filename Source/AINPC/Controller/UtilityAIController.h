#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h" 
#include "UtilityAIController.generated.h"

class UAISenseConfig_Sight;
class UUtilityAIComponent;
class UCognitionComponent;
class UNPCMentalState;

UCLASS()
class AINPC_API AUtilityAIController : public AAIController
{
	GENERATED_BODY()

public:
	AUtilityAIController();

	// --- 组件定义 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
	UUtilityAIComponent* UtilityComp; // 决策组件

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
	UCognitionComponent* CognitionComp; // 认知组件

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
	UAISenseConfig_Sight* SightConfig; // 视力配置

	// --- 数据 ---
	// 情绪状态依然放在 Controller 上作为共享数据，或者放在 Cognition 上也可以
	// 这里为了方便 UtilityComp 读取，我们还是存在 Controller 里
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Data")
	UNPCMentalState* MentalState;

protected:
	virtual void BeginPlay() override;

	// 感知回调
	UFUNCTION()
	void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

	// 认知更新回调
	UFUNCTION()
	void OnMindUpdated(const FMentalState& NewState);
};