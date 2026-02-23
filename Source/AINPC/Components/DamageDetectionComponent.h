// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DamageDetectionComponent.generated.h"

/** A single socket entry used as a damage source for tracing */
USTRUCT(BlueprintType)
struct FDamageSocket
{
	GENERATED_BODY()

	/** Socket/bone name on the skeletal mesh to trace from */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName = NAME_None;

	/** Sphere radius for the sweep at this socket (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1, Units = "cm"))
	float TraceRadius = 8.0f;
};

/** Fired each time a hit is confirmed and damage is applied */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageDealt,
	AActor*, Target,
	float, Damage,
	const FHitResult&, Hit);

/**
 * UDamageDetectionComponent
 *
 * Attach to any Character/Actor that has a SkeletalMeshComponent.
 * Call StartDetection() at the start of an attack window and StopDetection() at the end.
 * While active, every tick sweeps each registered socket from its last-frame world position
 * to its current world position, resolving hits against ICombatDamageable targets.
 *
 * Hit deduplication ensures each actor is damaged at most once per activation window.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AINPC_API UDamageDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UDamageDetectionComponent();

	// -----------------------------------------------------------------------
	// Configuration
	// -----------------------------------------------------------------------

	/** Sockets (or bones) to trace on the owner's skeletal mesh each tick */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Detection")
	TArray<FDamageSocket> DamageSockets;

	/** Default damage dealt per hit when StartDetection is called without an override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Detection", meta = (ClampMin = 0))
	float BaseDamage = 10.0f;

	/** Horizontal knockback impulse applied to hit targets (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Detection", meta = (ClampMin = 0, Units = "cm/s"))
	float KnockbackImpulse = 150.0f;

	/** Upward launch impulse applied to hit targets (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Detection", meta = (ClampMin = 0, Units = "cm/s"))
	float LaunchImpulse = 350.0f;

	/** Collision channel to query against (default: ECC_Pawn) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Detection")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	/** Show debug lines and spheres for each trace in the viewport */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Detection")
	bool bDrawDebugTraces = false;

	// -----------------------------------------------------------------------
	// Events
	// -----------------------------------------------------------------------

	/** Fires every time a new actor is hit and damage is applied this swing */
	UPROPERTY(BlueprintAssignable, Category = "Damage Detection")
	FOnDamageDealt OnDamageDealt;

	// -----------------------------------------------------------------------
	// API
	// -----------------------------------------------------------------------

	/**
	 * Begin the damage detection window.
	 * @param OverrideDamage  If > 0, overrides BaseDamage for this swing only.
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Detection")
	void StartDetection(float OverrideDamage = -1.0f);

	/** End the damage detection window and clear the hit-actor set. */
	UFUNCTION(BlueprintCallable, Category = "Damage Detection")
	void StopDetection();

	/**
	 * Convenience: start detection and automatically stop after the given duration.
	 * @param Duration        Seconds before auto-stop.
	 * @param OverrideDamage  If > 0, overrides BaseDamage for this swing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Detection")
	void StartDetectionForDuration(float Duration, float OverrideDamage = -1.0f);

	/** Returns true while a detection window is active */
	UFUNCTION(BlueprintPure, Category = "Damage Detection")
	bool IsDetecting() const { return bIsActive; }

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	bool bIsActive   = false;
	bool bFirstTick  = false; // true on the tick right after StartDetection - skip traces, only capture positions
	float ActiveDamage = 0.0f;

	UPROPERTY()
	USkeletalMeshComponent* CachedMesh = nullptr;

	FTimerHandle AutoStopTimer;

	/** Last-frame world positions for each registered socket */
	TMap<FName, FVector> LastSocketPositions;

	/** Actors already damaged during the current activation window */
	TSet<TWeakObjectPtr<AActor>> HitActorsThisSwing;

	void CacheSocketPositions();
	void PerformTraces();
	void ApplyHitDamage(AActor* Target, const FHitResult& Hit);
};
