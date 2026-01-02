#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "UtilityAI/UNPCMentalState.h"
#include "UtilityActionBase.generated.h"

class UUtilityActionBase;
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
	Hunger,
	SelfHealth,      // 自身血量百分比
	TargetHealth,    // 锁定目标的血量百分比
	DistanceToTarget,// 与目标的距离
	AmmoCount,       // 弹药量
	HasCover,        // 附近是否有掩体 (0 or 1)
	IsTargetPlayer   // 目标是否是玩家 (而不是其他怪)
};

USTRUCT(BlueprintType)
struct FUtilityConsideration
{
	GENERATED_BODY()

	// 输入源：我们要考量什么？(比如 E_SelfHealth)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUtilityInputType InputType;

	// 反应曲线：数值如何映射为分数？
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCurveFloat* ResponseCurve;
};

USTRUCT(BlueprintType)
struct FUtilityActionConfig : public FTableRowBase
{
	GENERATED_BODY()
	
	// A. 逻辑类：这个行为具体是干嘛的？(比如 BP_Attack)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UUtilityActionBase> ActionClass;

	// B & C. 变成了一个列表！这里可以放任意多个因子 (Anger, HP, Ammo...)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FUtilityConsideration> Considerations; // <--- 重点改动在这里

	// D. 基础权重：优先级修正 (比如逃跑比发呆重要 2 倍)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Weight = 1.0f;
};


// 前置声明
class AAIController;
class UNPCMentalState;

// --- 保持你的 Enum 和 Struct 不变 (这里省略，直接用你上面的) ---

UCLASS(Abstract, Blueprintable, BlueprintType)
class AINPC_API UUtilityActionBase : public UObject
{
    GENERATED_BODY()

public:
    // ==========================================
    // 1. 数据存储迭代：现在我们要存一个列表了
    // ==========================================
    
    // 不再是单个 LinkedInput/Curve，而是考量因子的数组
    UPROPERTY()
    TArray<FUtilityConsideration> Considerations;

    // 基础权重 (Base Weight)，作为最终乘数
    float BaseWeight = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Action Config")
    FString ActionName;

    // ==========================================
    // 2. 初始化函数迭代：注入数组
    // ==========================================
    void InitFromConfig(const FUtilityActionConfig& Config)
    {
       // 直接把配置里的数组拷贝过来
       Considerations = Config.Considerations;
       BaseWeight = Config.Weight;
       
       // 为了调试方便，如果没有名字，就把类名设为名字
       if (ActionName.IsEmpty())
       {
           ActionName = GetName();
       }
    }

    // ==========================================
    // 3. 核心打分逻辑迭代：循环 + 乘法
    // ==========================================
    
    // 注意：这里参数我加了 Controller，因为算“距离”和“自身血量”需要用到 Pawn
    virtual float CalculateScore(UNPCMentalState* MentalState, AAIController* Controller)
    {
       if (Considerations.Num() == 0) return 0.0f; // 没有配置因子，就没有分数

       float FinalScore = BaseWeight; // 初始分 = 基础权重

       // --- 遍历所有的考量因子 ---
       for (const FUtilityConsideration& Factor : Considerations)
       {
           // A. 获取原始输入值 (0~1 或 真实数值)
           float RawValue = GetConsiderationValue(Factor.InputType, MentalState, Controller);

           // B. 查曲线 (Normalize) -> 得到 0~1 的效用分
           // 如果没配曲线，默认 RawValue 就是分数 (线性)
           float FactorScore = (Factor.ResponseCurve) 
                               ? Factor.ResponseCurve->GetFloatValue(RawValue) 
                               : FMath::Clamp(RawValue, 0.0f, 1.0f);

           // C. 核心算法：乘法归一 (Mutiplicative Scoring)
           // 只要有一个因子是 0 (比如没子弹)，总分立刻变成 0
           FinalScore *= FactorScore;

           // 优化：如果分数为0，后面不用算了，直接返回
           if (FinalScore <= KINDA_SMALL_NUMBER) 
           {
               return 0.0f;
           }
       }

       return FinalScore;
    }

protected:
    // ==========================================
    // 4. 新增辅助函数：统一处理数值获取
    // ==========================================
    // 把复杂的 switch 逻辑抽离出来，让 CalculateScore 保持干净
    float GetConsiderationValue(EUtilityInputType InputType, UNPCMentalState* State, AAIController* Controller)
    {
        // 安全检查
        APawn* BotPawn = Controller ? Controller->GetPawn() : nullptr;
        if (!BotPawn) return 0.0f;

        switch (InputType)
        {
            // --- 情绪类 (来自 MentalState) ---
            case EUtilityInputType::Anger:         return State ? State->Anger : 0.f;
            case EUtilityInputType::Fear:          return State ? State->Fear : 0.f;
            case EUtilityInputType::Confidence:    return State ? State->Confidence : 0.f;
            case EUtilityInputType::SocialBattery: return State ? State->SocialBattery : 0.f;
            case EUtilityInputType::Hunger:        return State ? State->Hunger : 0.f;

            // --- 物理状态类 (需要 Pawn) ---
            case EUtilityInputType::SelfHealth:
                // 这里假设你有个通用的接口获取血量，或者 Cast 到你的 Character
                // return MyBot->GetHealthPercent(); 
                return 1.0f; // 占位

            case EUtilityInputType::AmmoCount:
                // return MyWeapon->GetAmmoPercent();
                return 1.0f; // 占位

            // --- 战术环境类 (需要 Controller / Target) ---
            case EUtilityInputType::DistanceToTarget:
            {
                AActor* Target = Controller->GetFocusActor(); // 或者你自己写的 GetTarget()
                if (Target)
                {
                    float Dist = FVector::Dist(BotPawn->GetActorLocation(), Target->GetActorLocation());
                    // 归一化：假设 2000 码是最大考虑距离
                    return FMath::Clamp(Dist / 2000.0f, 0.0f, 1.0f);
                }
                return 1.0f; // 没目标通常意味着非常远
            }
            
            case EUtilityInputType::HasCover:
                // 这里需要调用你的 EQS 系统或者简单的 LineTrace
                return 0.0f; // 假设没掩体

            default: 
                return 0.0f;
        }
    }

public:
    // --- 保持原本的执行逻辑不变 ---
    UFUNCTION(BlueprintNativeEvent)
    void Enter(AAIController* Controller);
    virtual void Enter_Implementation(AAIController* Controller) {}
    
    UFUNCTION(BlueprintNativeEvent)
    void Execute(AAIController* Controller);
    virtual void Execute_Implementation(AAIController* Controller) {}
    
    UFUNCTION(BlueprintNativeEvent)
    void Exit(AAIController* Controller);
    virtual void Exit_Implementation(AAIController* Controller) {}
};