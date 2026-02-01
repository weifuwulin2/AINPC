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

	// ========================================
	// Action Change Events (Observation System)
	// ========================================

	/** Fired when this NPC's action changes (allows other NPCs to observe) */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActionChanged, AActor*, NPC, UUtilityActionBase*, OldAction, UUtilityActionBase*, NewAction);

	UPROPERTY(BlueprintAssignable, Category = "AI|Observation")
	FOnActionChanged OnActionChanged;

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
	
    // ✅ NEW: Current Profession ID (Pushed by NPCDefinitionComponent)
    // 职业 ID：由 NPCDefinitionComponent 推送
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Config")
    FName CurrentProfessionID;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    // ✅ NEW: Explicitly set Profession (Fixes Initialization Order)
    // 显式设置职业（修复初始化顺序问题）
    UFUNCTION(BlueprintCallable, Category = "AI Config")
    void SetProfession(FName NewProfessionID);

private:
	// 内部函数：加载表格
	void LoadActionsFromTable();
	// 内部函数：选择最佳动作
	void EvaluateAndDecide();
    
    // =========================================================
    // Transition System: The Constitution
    // =========================================================
    // Centralized transition logic (replaces scattered if/else)
    bool CanTransition(UUtilityActionBase* Current, UUtilityActionBase* Candidate, float CandidateScore, float CurrentScore);

public:
	// 请求下一帧打印调试日志 (通常在感知到刺激后调用)
	UFUNCTION(BlueprintCallable, Category = "AI Debug")
	void RequestDebugLog() { bPendingDebugLog = true; }

private:
	// 是否请求了调试日志
	bool bPendingDebugLog = false;
	
	// 上次状态日志时间（每个实例独立）
	// Last status log time (per-instance)
	float LastStatusLogTime = 0.0f;
};