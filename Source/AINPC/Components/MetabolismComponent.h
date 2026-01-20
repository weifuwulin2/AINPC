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
	// Current: 0.002 = 0.6 hunger in 300 seconds (~5 mins) - Balanced for gameplay
	// Example: 0.01 = 1.0 full hunger in 100 seconds (~1.7 mins) - Good for testing
	// Production: 0.001 = 1.0 full hunger in 1000 seconds (~16 mins)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism|Physiological")
	float HungerRate = 0.002f;  // Slower: 5 mins to reach 0.6

	// 疲劳增长速率 (每秒增加多少)
	// Energy/Fatigue increase rate per second
	// Current: 0.0016 = 0.6 fatigue in 375 seconds (~6.25 mins) - Balanced for gameplay
	// Example: 0.008 = 1.0 full fatigue in 125 seconds (~2 mins) - Good for testing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism|Physiological")
	float EnergyRate = 0.0016f;  // Slower: 6.25 mins to reach 0.6

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
	// Current: 0.02 = Takes ~25s to go from 0.7 to 0.2 (safe again)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism|Safety")
	float ThreatDecayRate = 0.02f;  // Slower: Remember threats longer

	// 获取 MentalState 的辅助函数
	UNPCMentalState* GetMentalState() const;
private:
	// 缓存 Controller 引用
	UPROPERTY()
	AUtilityAIController* CachedController;
	
	// ✅ 威胁状态追踪：如果之前很高，当降低到安全值时触发记忆清理
	// Threat Tracking: If it was high, trigger memory cleanup when it drops to safe levels
	bool bWasThreatened = false;

	// 调试日志计时器（每个 NPC 独立）
	// Debug log timer (per-NPC)
	float LastDebugLogTime = 0.0f;
	
};
