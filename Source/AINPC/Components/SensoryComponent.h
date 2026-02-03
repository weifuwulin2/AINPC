#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Social/SocialTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "SensoryComponent.generated.h"

// Output Nerve: Connects to MemoryComponent
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSemanticEventSensed, const FSemanticEvent&, Event);



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

	// ========================================
	// Action Observation System
	// ========================================

	/**
	 * Subscribe to an NPC's action changes (when they enter vision).
	 * Automatically managed by perception system.
	 */
	void SubscribeToActionChanges(AActor* ObservedActor);

	/**
	 * Unsubscribe from an NPC's action changes (when they leave vision).
	 */
	void UnsubscribeFromActionChanges(AActor* ObservedActor);

	/**
	 * Callback when an observed NPC changes their action.
	 * Filters and submits to AttentionBudgetComponent.
	 */
	/**
	 * Callback when an observed NPC changes their action.
	 * Filters and submits to AttentionBudgetComponent.
	 */
	UFUNCTION()
	void HandleObservedActionChange(AActor* NPC, UUtilityActionBase* OldAction, UUtilityActionBase* NewAction);

	/**
	 * Manually suppress/unsuppress action observation for this specific NPC.
	 * Useful for specific plot moments or cutscenes where this NPC should be blind to action changes.
	 * 
	 * @param bSuppressed If true, action changes will be ignored.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI Sensory | Config")
	void SetActionObservationSuppressed(bool bSuppressed);

	/**
	 * Check if action observation is currently suppressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI Sensory | Config")
	bool IsActionObservationSuppressed() const { return bSuppressActionObservation; }
protected:
	/** If true, HandleObservedActionChange will not process events. Default is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Sensory | Config")
	bool bSuppressActionObservation = false;

public:

	/**
	 * Handle immediate high-priority observations from AttentionBudget.
	 * Converts to semantic event and broadcasts immediately.
	 */
	UFUNCTION()
	void HandleImmediateObservation(const FSemanticEvent& Event);

	/**
	 * Handle batched low-priority observations from AttentionBudget.
	 * Merges similar events and broadcasts as summary.
	 */
	UFUNCTION()
	void HandleBatchedObservations(const TArray<FSemanticEvent>& Events);

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
	 * Check if two actors belong to hostile factions
	 * 检查两个 Actor 是否属于敌对阵营
	 * @deprecated Use FactionHelpers::AreActorsHostile() instead
	 */
	UFUNCTION(BlueprintCallable, Category = "AI Sensory",
	          meta=(DeprecatedFunction, DeprecationMessage="Use FactionHelpers::AreActorsHostile() instead"))
	bool AreActorsHostile(AActor* ActorA, AActor* ActorB) const;

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

	// ✅ Universal helper: Describe any actor with faction relationship relative to self
	// 通用辅助：描述任何 Actor 及其与自身的阵营关系
	// Returns e.g. "Guard (Faction: Human, MY ALLY)" or "Slave (Faction: Slave, MY ENEMY)"
	FString DescribeActorWithRelationship(AActor* Actor) const;

	// ✅ Check if an actor is myself (Owner or Owner's Pawn)
	// 检查一个 Actor 是否是自己
	bool IsMyself(AActor* Actor) const;


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
	float PerceptionCooldown = 2.0f;
	
	
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

	// 获取 Actor 的阵营
	// Get actor's faction
	FName GetActorFaction(AActor* Actor) const;
	
	// 增加目标的视觉累积计数，返回当前计数
	// Increment visual accumulation for target, returns current count
	int32 IncrementVisualAccumulation(AActor* Target);
	
	// 重置目标的累积计数（在触发认知事件后）
	// Reset accumulation count for target (after triggering cognitive event)
	void ResetVisualAccumulation(AActor* Target);

	// ========================================
	// Action Observation Tracking
	// ========================================

	/** NPCs we're currently subscribed to (tracking their action changes) */
	UPROPERTY()
	TSet<AActor*> ObservedNPCs;
};