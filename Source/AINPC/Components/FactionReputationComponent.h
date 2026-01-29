
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Social/FactionTypes.h"
#include "FactionReputationComponent.generated.h"

/**
 * Manages "Personal Micro" relationships.
 * Holds the NPC's specific Faction ID and personal feelings towards individuals.
 * Queries FactionSubsystem for global baseline if no personal override exists.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API UFactionReputationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFactionReputationComponent();

	virtual void BeginPlay() override;

	// --- State ---

	/** The Faction this NPC belongs to (e.g., "Humans", "Zombies") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FName CurrentFactionID;

	/** 
	 * Personal overrides for specific actors.
	 * Key: Actor (usually Player or specific NPC)
	 * Value: 0-100 Attitude.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Faction")
	TMap<AActor*, float> PersonalReputations;

	// --- API ---

	/**
	 * Get Attitude towards a target (0-100).
	 * Logic:
	 * 1. Check PersonalReputations (Override).
	 * 2. If none, check Global Faction Relations (Subsystem).
	 * 3. Return result.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	float GetAttitudeTowards(AActor* Target) const;

	/** Returns true if Attitude is below Hostility Threshold (default 25) */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	bool IsHostile(AActor* Target) const;

	/** Modify personal reputation towards an actor */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	void ModifyReputation(AActor* Target, float Delta);

	/** Helper to find FactionID of any actor (tries Component -> Tag -> Default) */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	static FName GetFactionID(AActor* Actor);

	/** 
	 * Centralized policy to check if combat is allowed regardless of context (e.g. Scene Safety).
	 * Resolves Tags like Event.Danger, Directive.Combat, etc.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	bool EvaluateCombatPolicy(const AActor* Source, const AActor* Target) const;
};
