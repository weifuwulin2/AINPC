
#pragma once

#include "CoreMinimal.h"
#include "Variant_Combat/AI/CombatEnemy.h" // Inherit from CombatEnemy
#include "Social/SocialTypes.h"
#include "NarrativeCompanion.generated.h"

class UCognitionComponent;

/**
 * The Player's Companion (Paimon).
 * 1. Follows the Player.
 * 2. AUTOMATICALLY Observes world events via SensoryComponent.
 * 3. Records significant events to NarrativeDirector.
 * 4. Can FIGHT via CombatEnemy inheritance.
 */
UCLASS()
class AINPC_API ANarrativeCompanion : public ACombatEnemy
{
	GENERATED_BODY()

public:
	ANarrativeCompanion();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:	
	
	// --- Cognition Access ---
	UFUNCTION(BlueprintPure, Category = "Narrative Companion")
	UCognitionComponent* GetCognitionComp() const;

	// --- Automatic Observation Handling ---

	/** 
	 * Binds to the SensoryComponent on the AIController. 
	 * Must be called after Controller is possessed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Companion")
	void BindSensorySystem();

	/** Callback when SensoryComponent detects something */
	UFUNCTION()
	void OnSensoryEvent(const FSemanticEvent& Event);

	// ===========================================
	// MANUAL OBSERVATION API (Backwards compatibility / Scripted Events)
	// ===========================================

	UFUNCTION(BlueprintCallable, Category = "Narrative Companion|Observation")
	void ObserveEvent(FString Description, TArray<FName> EventTags);

	UFUNCTION(BlueprintCallable, Category = "Narrative Companion|Observation")
	void ObserveNPCDeath(AActor* Victim, AActor* Killer);

	UFUNCTION(BlueprintCallable, Category = "Narrative Companion|Observation")
	void ObservePlayerAction(FString ActionDescription, TArray<FName> ActionTags);

	UFUNCTION(BlueprintCallable, Category = "Narrative Companion|Observation")
	void ObserveRelationshipChange(FName SourceActor, FName TargetActor, float OldValue, float NewValue);

	UFUNCTION(BlueprintCallable, Category = "Narrative Companion|Observation")
	void ObserveFactionReputationChange(FName FactionID, float Delta, FString Reason);

	UFUNCTION(BlueprintCallable, Category = "Narrative Companion|Observation")
	void ObserveFactionPopulationChange(FName FactionID, int32 Delta);

	// --- Follow Logic ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion Behavior")
	float FollowDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion Behavior")
	bool bIsFollowing = true;

private:
	AActor* GetLeader() const;
	
	// Track if we have bound to sensory system
	bool bSensoryBound = false;
};
