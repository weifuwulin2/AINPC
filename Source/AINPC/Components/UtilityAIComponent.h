#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Base/UtilityActionBase.h"
#include "UtilityAIComponent.generated.h"

class AUtilityAIController;
class UNPCMentalState;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API UUtilityAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UUtilityAIComponent();

	// --- 配置 ---
	// 在蓝图中指定 DataTable
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Config")
	UDataTable* ActionDataTable;

	// --- 运行时状态 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Debug")
	UUtilityActionBase* CurrentAction;

	UPROPERTY()
	TArray<UUtilityActionBase*> AvailableActions;

	// 缓存 Controller 引用
	UPROPERTY()
	AUtilityAIController* OwnerController;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// 内部函数：加载表格
	void LoadActionsFromTable();
	// 内部函数：选择最佳动作
	void EvaluateAndDecide();
};