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

	// --- Processing Layer (The Spinal Cord) ---

	/**
	 * Filters event based on physiological needs.
	 * Returns true if the event is important enough to be sent to the brain.
	 */
	bool ProcessEventFilter(FSemanticEvent& Event);

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
	
	// 检查目标是否应该被感知（考虑冷却）
	// Check if target should be perceived (considering cooldown)
	bool ShouldPerceiveTarget(AActor* Target);
	
	// 标记目标已被感知
	// Mark target as perceived
	void MarkTargetPerceived(AActor* Target);
	
	// 清理过期的感知记录
	// Clean up expired perception records
	void CleanupPerceptionTracking();
};