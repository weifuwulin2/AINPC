#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Social/SocialTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "SensoryComponent.generated.h"

// Output Nerve: Connects to MemoryComponent
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSemanticEventSensed, const FSemanticEvent&, Event);

// Legacy: Legacy output for Chat/Bubbles
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSensoryStimulusProduced, const FString&, StimulusDescription);

class UAIPerceptionComponent;
class USmartObjectComponent;
class ACombatEnemy;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API USensoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USensoryComponent();

	// Output Nerve: Connects to MemoryComponent
	UPROPERTY(BlueprintAssignable, Category = "AI Sensory")
	FOnSemanticEventSensed OnSemanticEventSensed;

	// Legacy: Generic Description Output
	UPROPERTY(BlueprintAssignable, Category = "AI Sensory")
	FOnSensoryStimulusProduced OnStimulusProduced;

	// --- Input Nerves (Receptors) ---

	/**
	 * Called when this NPC interacts with a Smart Object.
	 * Transforms the static Semantics into a runtime Event.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI Sensory")
	void HandleSmartObjectInteraction(AActor* Instigator, AActor* SmartObjectActor);

	// Legacy: Connect to standard AIPerception
	UFUNCTION(BlueprintCallable, Category = "AI Sensory")
	void InitializeSensorySystem(UAIPerceptionComponent* InPerceptionComp);

	// Legacy: Receive Speech for Chat
	UFUNCTION(BlueprintCallable, Category = "AI Communication")
	void ReceiveSpeech(AActor* Speaker, FString Message);

	// New: Handle Death Event
	UFUNCTION(BlueprintCallable, Category = "AI Sensory")
	void HandleDeath(AActor* DeadActor, AActor* Killer);
	
	// Wrapper callback for CombatEnemy delegate
	UFUNCTION()
	void HandleCombatEnemyDeath(ACombatEnemy* DeadEnemy, AActor* Killer);

	// --- Processing Layer (The Spinal Cord) ---

	/**
	 * Filters event based on physiological needs.
	 * Returns true if the event is important enough to be sent to the brain.
	 */
	bool ProcessEventFilter(FSemanticEvent& Event);

	/**
	 * Finds the best (closest) Smart Object matching the given tag.
	 * 寻找最佳（最近）的匹配 Tag 的智能对象。
	 */
	UFUNCTION(BlueprintCallable, Category = "AI Sensory")
	AActor* FindBestSmartObject(FGameplayTag ActivityTag);

	/**
	 * Helper: Get the faction of an actor (Safe to call on any actor).
	 * Defaults to Neutral.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI Sensory")
	static EFactionType GetFaction(AActor* Actor);

protected:
	virtual void BeginPlay() override;

	// Legacy Callbacks
	UFUNCTION()
	void HandleTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void HandleDamageTaken(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

private:
	// Helper to parse templates like "{Instigator} sits on {Target}"
	FString ParseDescriptionTemplate(const FString& Template, AActor* Instigator, AActor* Target);

	// Legacy Helper
	FString FormatDescription(FString Verb, AActor* Target, FString ExtraInfo = "");
	
	// ✅ NEW: Enhanced description with faction/hostility context for LLM
	// 新增：包含阵营/敌对信息的增强描述，供 LLM 使用
	FString FormatDescriptionWithContext(FString Verb, AActor* Target, FName SelfFaction, FName TargetFaction, bool bIsHostile);


private:
	// ✅ 注意力追踪：防止重复感知同一目标
	// Attention Tracking: Prevent repeated perception of same target
	
	// 记录最近感知到的目标及其时间戳
	// Track recently perceived targets and their timestamps
	UPROPERTY()
	TMap<AActor*, float> RecentlyPerceivedActors;
	
	// 同一目标的感知冷却时间（秒）
	// Cooldown time for perceiving the same target (seconds)
	UPROPERTY(EditDefaultsOnly, Category = "AI Sensory | Attention")
	float PerceptionCooldown = 5.0f;
	
	
	// ✅ 视觉累积系统：追踪非敌对目标的累积观察次数
	// Visual Accumulation System: Track cumulative sightings of non-hostile targets
	
	// 记录每个目标被看到的累积次数
	// Track cumulative sighting count for each target
	UPROPERTY()
	TMap<AActor*, int32> VisualAccumulationCount;
	
	// 触发认知事件所需的累积次数阈值（仅用于低优先级目标）
	// Threshold of sightings needed to trigger a cognitive event (for low-priority targets only)
	UPROPERTY(EditDefaultsOnly, Category = "AI Sensory | Attention")
	int32 AccumulationThreshold = 3;
	
	// ✅ 即时响应系统：高优先级目标的即时感知
	// Immediate Response System: Instant perception for high-priority targets
	
	// 是否对高优先级目标（玩家、敌人）启用即时响应
	// Enable immediate response for high-priority targets (Player, Enemy)
	UPROPERTY(EditDefaultsOnly, Category = "AI Sensory | Immediate Response")
	bool bEnableImmediateResponse = true;
	
	// 高优先级目标的感知重要性（Magnitude）
	// Perception magnitude for high-priority targets
	UPROPERTY(EditDefaultsOnly, Category = "AI Sensory | Immediate Response", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HighPriorityMagnitude = 0.5f;
	
	// 检查目标是否应该被感知（考虑冷却）
	// Check if target should be perceived (considering cooldown)
	bool ShouldPerceiveTarget(AActor* Target);
	
	// 标记目标已被感知
	// Mark target as perceived
	void MarkTargetPerceived(AActor* Target);
	
	// 清理过期的感知记录
	// Clean up expired perception records
	void CleanupPerceptionTracking();
	
	// 检查两个 Actor 是否属于敌对阵营
	// Check if two actors belong to hostile factions
	bool AreActorsHostile(AActor* ActorA, AActor* ActorB) const;
	
	// 获取 Actor 的阵营
	// Get actor's faction
	FName GetActorFaction(AActor* Actor) const;
	
	// 增加目标的视觉累积计数，返回当前计数
	// Increment visual accumulation for target, returns current count
	int32 IncrementVisualAccumulation(AActor* Target);
	
	// 重置目标的累积计数（在触发认知事件后）
	// Reset accumulation count for target (after triggering cognitive event)
	void ResetVisualAccumulation(AActor* Target);
	
	// ✅ 获取用于记忆的 Actor 名称 (Smart Name)
	// Get actor name for memory (e.g. PersonalityID or "Player")
	FString GetSmartActorName(AActor* Actor);
};