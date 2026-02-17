
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Social/FactionTypes.h"
#include "Social/SocialTypes.h"
#include "FactionReputationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SevenParams(
	FOnRelationshipChanged,
	AActor*, SourceActor,
	AActor*, TargetActor,
	FName, SourceID,
	FName, TargetID,
	float, OldAttitude,
	float, NewAttitude,
	bool, bCrossedBondThreshold);

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
	FName FactionID;

	/** 
	 * Personal overrides for specific actors.
	 * Key: Actor (usually Player or specific NPC)
	 * Value: 0-100 Attitude.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Faction")
	TMap<AActor*, float> PersonalReputations;

	/**
	 * Semantic social bonds keyed by stable actor identity (NameID when available).
	 * Complements PersonalReputations numeric overrides.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Faction|Social")
	TMap<FName, FSocialBond> SocialBonds;

	/** Broadcast when relationship attitude changes (useful for reflection hooks/UI/debug). */
	UPROPERTY(BlueprintAssignable, Category = "Faction|Social")
	FOnRelationshipChanged OnRelationshipChanged;

	// --- API ---

	/**
	 * Get Attitude towards a target (0-100).
	 * Logic:
	 * 1. Check PersonalReputations (Override).
	 * 2. Check SocialBonds (semantic override mapped to attitude).
	 * 3. If none, check Global Faction Relations (Subsystem).
	 * 4. Return result.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	float GetAttitudeTowards(AActor* Target) const;

	/**
	 * Get global reputation towards another faction (0-100).
	 * Queries the FactionSubsystem for cross-faction reputation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	float GetReputationWith(FName TargetFactionID) const;

	/** Returns true if Attitude is below Hostility Threshold (default 25) */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	bool IsHostile(AActor* Target) const;

	/** Modify personal reputation towards an actor */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	void ModifyReputation(AActor* Target, float Delta);

	/** Get stable relationship key for actor (prefers NPCDefinition NameID). */
	UFUNCTION(BlueprintCallable, Category = "Faction|Social")
	FName GetStableSocialID(AActor* Target) const;

	/** Get semantic relationship summary toward target for prompt/context injection. */
	UFUNCTION(BlueprintCallable, Category = "Faction|Social")
	FString GetRelationshipSummaryTowards(AActor* Target) const;

	/**
	 * Helper to find FactionID of any actor (tries Component -> Tag -> Default)
	 * @deprecated Use FactionHelpers::GetFactionID() instead
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction",
	          meta=(DeprecatedFunction, DeprecationMessage="Use FactionHelpers::GetFactionID() instead"))
	static FName GetFactionID(AActor* Actor);

	/** 
	 * Centralized policy to check if combat is allowed regardless of context (e.g. Scene Safety).
	 * Resolves Tags like Event.Danger, Directive.Combat, etc.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	bool EvaluateCombatPolicy(const AActor* Source, const AActor* Target) const;
};
