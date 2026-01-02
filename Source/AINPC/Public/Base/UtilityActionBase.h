#pragma once

#include "CoreMinimal.h"
#include "UtilityAI/UNPCMentalState.h"
#include "UtilityActionBase.generated.h"

class UNPCMentalState;
class AAIController;
/**
 * 
 */
UENUM(BlueprintType)
enum class EUtilityInputType : uint8
{
	Anger,
	Fear,
	Confidence,
	SocialBattery,
	Hunger
};

USTRUCT(BlueprintType)
struct FUtilityActionConfig : public FTableRowBase
{
	GENERATED_BODY()

public:
	// A. 逻辑类：这个行为具体是干嘛的？(比如 BP_Attack)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UUtilityActionBase> ActionClass;

	// B. 输入源：这个行为受哪个情绪驱动？
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUtilityInputType InputType;

	// C. 反应曲线：情绪值如何转化为意愿值？
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCurveFloat* ResponseCurve;

	// D. 基础权重：优先级修正 (比如逃跑比发呆重要 2 倍)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Weight = 1.0f;
};


UCLASS(Abstract, Blueprintable, BlueprintType)
class AINPC_API UUtilityActionBase : public UObject
{
	GENERATED_BODY()

public:
	// 存储配置数据
	EUtilityInputType LinkedInput;
	UPROPERTY() UCurveFloat* LinkedCurve;
	float LinkedWeight;

	// --- 初始化函数 ---
	// Controller 加载表格时会调用这个，把配置注入进来
	void InitFromConfig(const FUtilityActionConfig& Config)
	{
		LinkedInput = Config.InputType;
		LinkedCurve = Config.ResponseCurve;
		LinkedWeight = Config.Weight;
	}

	// --- 核心打分逻辑 ---
	// 现在不需要子类去重写“怎么算分”，基类统一处理！
	virtual float CalculateScore(UNPCMentalState* State)
	{
		// 1. 获取输入值
		float InputValue = 0.0f;
		if (State)
		{
			switch (LinkedInput)
			{
				case EUtilityInputType::Anger: InputValue = State->Anger; break;
				case EUtilityInputType::Fear:  InputValue = State->Fear; break;
				case EUtilityInputType::Confidence: InputValue = State-> Confidence; break;
				case EUtilityInputType::Hunger: InputValue = State-> Hunger; break;
				case EUtilityInputType::SocialBattery: InputValue = State->SocialBattery; break;
			}
		}

		// 2. 查曲线 (如果没有曲线，就用线性默认值)
		float CurveValue = (LinkedCurve) ? LinkedCurve->GetFloatValue(InputValue) : InputValue;

		// 3. 乘权重
		return CurveValue * LinkedWeight;
	}
	
	// 动作名称，方便调试
	UPROPERTY(EditDefaultsOnly, Category = "Action Config")
	FString ActionName;

	// === 核心虚函数 (Virtual Functions) ===

	
	UFUNCTION(BlueprintNativeEvent)
	void Enter(AAIController* Controller);
	virtual void Enter_Implementation(AAIController* Controller) {}
	
	// 2. 执行函数：真正干活的地方
	UFUNCTION(BlueprintNativeEvent)
	void Execute(AAIController* Controller);
	virtual void Execute_Implementation(AAIController* Controller) {}
	
	// 3. 退出函数：切换动作时清理现场（比如停止动画）
	UFUNCTION(BlueprintNativeEvent)
	void Exit(AAIController* Controller);
	virtual void Exit_Implementation(AAIController* Controller) {}
};
