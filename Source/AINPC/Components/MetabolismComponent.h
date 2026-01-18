#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MetabolismComponent.generated.h"

class AUtilityAIController;
class UNPCMentalState;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API UMetabolismComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMetabolismComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// === 配置 (Configuration) ===

	// 饥饿增长速率 (每秒增加多少，0.0~1.0)
	// Hunger increase rate per second
	// Example: 0.01 = 1.0 full hunger in 100 seconds (~1.7 mins) - Good for testing
	// Production: 0.001 = 1.0 full hunger in 1000 seconds (~16 mins)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism|Physiological")
	float HungerRate = 0.01f; 

	// 疲劳增长速率 (每秒增加多少)
	// Energy/Fatigue increase rate per second
	// Example: 0.008 = 1.0 full fatigue in 125 seconds (~2 mins) - Good for testing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism|Physiological")
	float EnergyRate = 0.008f;

	// 无聊增长速率 (每秒增加多少)
	// Boredom rate per second
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism|SelfActualization")
	float BoredomRate = 0.02f; // Get bored faster than hungry

	// 孤独增长速率 (每秒增加多少)
	// Loneliness rate per second
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism|Social")
	float LonelinessRate = 0.005f; // Get lonely slowly

	// 情绪冷却速率 (每秒减少多少)
	// Emotional decay rate (Cool down) per second
	// Example: 0.05 = Takes 20 seconds to go from Furious(1.0) to Calm(0.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism|Emotional")
	float EmotionalDecayRate = 0.05f;

	// 威胁感消失速率 (通常比情绪快或慢，取决于设定)
	// Threat decay rate
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism|Safety")
	float ThreatDecayRate = 0.1f;

	// 获取 MentalState 的辅助函数
	UNPCMentalState* GetMentalState() const;
private:
	// 缓存 Controller 引用
	UPROPERTY()
	AUtilityAIController* CachedController;

	// 调试日志计时器（每个 NPC 独立）
	// Debug log timer (per-NPC)
	float LastDebugLogTime = 0.0f;
	
};
