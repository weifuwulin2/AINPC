// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/DamageDetectionComponent.h"

#include "AINPC.h"
#include "Variant_Combat/Interfaces/CombatDamageable.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

UDamageDetectionComponent::UDamageDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Stay dormant until StartDetection() is called - no unnecessary ticking
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UDamageDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		CachedMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
		if (!CachedMesh)
		{
			COMBAT_LOG(Warning, TEXT("No SkeletalMeshComponent on '%s' - traces will not function."),
				*Owner->GetName());
		}
	}
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void UDamageDetectionComponent::StartDetection(float OverrideDamage)
{
	if (!CachedMesh)
	{
		COMBAT_LOG(Warning, TEXT("StartDetection: no mesh on '%s', aborting."),
			*GetOwner()->GetName());
		return;
	}

	if (DamageSockets.IsEmpty())
	{
		COMBAT_LOG(Warning, TEXT("StartDetection on '%s': DamageSockets is empty - add at least one socket."),
			*GetOwner()->GetName());
		return;
	}

	ActiveDamage = (OverrideDamage > 0.0f) ? OverrideDamage : BaseDamage;
	COMBAT_LOG(Verbose, TEXT("Detection started on '%s' | damage=%.1f | sockets=%d"),
		*GetOwner()->GetName(), ActiveDamage, DamageSockets.Num());
	HitActorsThisSwing.Empty();
	bFirstTick = true; // first tick only captures positions; no sweep yet
	bIsActive  = true;

	CacheSocketPositions();
	SetComponentTickEnabled(true);
}

void UDamageDetectionComponent::StopDetection()
{
	if (!bIsActive) return;

	bIsActive = false;
	SetComponentTickEnabled(false);
	HitActorsThisSwing.Empty();
	LastSocketPositions.Empty();

	// Cancel any pending auto-stop timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoStopTimer);
	}
}

void UDamageDetectionComponent::StartDetectionForDuration(float Duration, float OverrideDamage)
{
	StartDetection(OverrideDamage);

	if (bIsActive && Duration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			AutoStopTimer,
			this, &UDamageDetectionComponent::StopDetection,
			Duration,
			false);
	}
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void UDamageDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsActive || !CachedMesh) return;

	if (bFirstTick)
	{
		// Positions were snapshotted in StartDetection(); skip tracing for the
		// first real tick so the sweep has valid "from" and "to" endpoints.
		bFirstTick = false;
	}
	else
	{
		PerformTraces();
	}

	CacheSocketPositions();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void UDamageDetectionComponent::CacheSocketPositions()
{
	for (const FDamageSocket& Socket : DamageSockets)
	{
		if (Socket.SocketName.IsNone()) continue;

		if (CachedMesh->DoesSocketExist(Socket.SocketName))
		{
			LastSocketPositions.Add(Socket.SocketName, CachedMesh->GetSocketLocation(Socket.SocketName));
		}
		else
		{
			COMBAT_LOG(Warning, TEXT("Socket '%s' not found on '%s'"),
				*Socket.SocketName.ToString(), *GetOwner()->GetName());
		}
	}
}

void UDamageDetectionComponent::PerformTraces()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AActor* Owner = GetOwner();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	QueryParams.bTraceComplex = false;

	for (const FDamageSocket& Socket : DamageSockets)
	{
		if (Socket.SocketName.IsNone()) continue;

		const FVector* LastPosPtr = LastSocketPositions.Find(Socket.SocketName);
		if (!LastPosPtr || !CachedMesh->DoesSocketExist(Socket.SocketName)) continue;

		const FVector CurrentPos = CachedMesh->GetSocketLocation(Socket.SocketName);
		const FVector LastPos    = *LastPosPtr;

		// No movement this frame - skip to avoid degenerate zero-length sweep
		if (FVector::DistSquared(CurrentPos, LastPos) < KINDA_SMALL_NUMBER) continue;

		TArray<FHitResult> Hits;
		const FCollisionShape Sphere = FCollisionShape::MakeSphere(Socket.TraceRadius);

		const bool bAnyHit = World->SweepMultiByChannel(
			Hits,
			LastPos, CurrentPos,
			FQuat::Identity,
			TraceChannel,
			Sphere,
			QueryParams);

#if ENABLE_DRAW_DEBUG
		if (bDrawDebugTraces)
		{
			const FColor LineColor = bAnyHit ? FColor::Red : FColor::Green;
			DrawDebugLine(World, LastPos, CurrentPos, LineColor, false, 0.1f, 0, 1.5f);
			DrawDebugSphere(World, CurrentPos, Socket.TraceRadius, 8, LineColor, false, 0.1f);
		}
#endif

		for (const FHitResult& Hit : Hits)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActor == Owner) continue;

			// One damage event per actor per swing
			TWeakObjectPtr<AActor> WeakHit(HitActor);
			if (HitActorsThisSwing.Contains(WeakHit)) continue;

			HitActorsThisSwing.Add(WeakHit);
			ApplyHitDamage(HitActor, Hit);
		}
	}
}

void UDamageDetectionComponent::ApplyHitDamage(AActor* Target, const FHitResult& Hit)
{
	AActor* Owner = GetOwner();

	// Knock the target away from the attacker, with an upward bias
	const FVector ImpactDir = (Hit.ImpactPoint - Owner->GetActorLocation()).GetSafeNormal2D();
	const FVector Impulse   = (ImpactDir * KnockbackImpulse) + (FVector::UpVector * LaunchImpulse);

	if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(Target))
	{
		Damageable->ApplyDamage(ActiveDamage, Owner, Hit.ImpactPoint, Impulse);
		COMBAT_LOG(Log, TEXT("'%s' hit '%s' for %.1f (ICombatDamageable)"),
			*Owner->GetName(), *Target->GetName(), ActiveDamage);
	}
	else
	{
		// Fallback: Unreal native point damage (works on any AActor)
		AController* Instigator = Owner ? Owner->GetInstigatorController() : nullptr;
		UGameplayStatics::ApplyPointDamage(
			Target, ActiveDamage,
			ImpactDir, Hit,
			Instigator, Owner,
			nullptr);
		COMBAT_LOG(Log, TEXT("'%s' hit '%s' for %.1f (native ApplyPointDamage fallback)"),
			*Owner->GetName(), *Target->GetName(), ActiveDamage);
	}

	OnDamageDealt.Broadcast(Target, ActiveDamage, Hit);
}
