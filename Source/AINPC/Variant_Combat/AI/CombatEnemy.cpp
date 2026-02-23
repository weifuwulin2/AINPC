// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatEnemy.h"

#include "AINPC.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CombatAIController.h"
#include "Components/WidgetComponent.h"
#include "Engine/DamageEvents.h"
#include "CombatLifeBar.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/NPCDefinitionComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/GoalComponent.h"
#include "Components/MonsterComponent.h"
#include "Controller/UtilityAIController.h"
#include "Components/SensoryComponent.h"
#include "Components/FactionReputationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/NarrativeHistorySubsystem.h"
#include "Components/DamageDetectionComponent.h"
#include "Components/CombatStatsComponent.h"

ACombatEnemy::ACombatEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// bind the attack montage ended delegate
	OnAttackMontageEnded.BindUObject(this, &ACombatEnemy::AttackMontageEnded);

	// set the AI Controller class by default (Use UtilityAIController for full Utility AI support)
	AIControllerClass = AUtilityAIController::StaticClass();

	// use an AI Controller regardless of whether we're placed or spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// ignore the controller's yaw rotation
	bUseControllerRotationYaw = false;

	// create the life bar
	LifeBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("LifeBar"));
	LifeBar->SetupAttachment(RootComponent);

	// continuous multi-socket damage detection (configure DamageSockets in BP/editor)
	DamageDetection = CreateDefaultSubobject<UDamageDetectionComponent>(TEXT("DamageDetection"));

	// centralised combat stats (HP, attack damage, defense, speed, range)
	CombatStats = CreateDefaultSubobject<UCombatStatsComponent>(TEXT("CombatStats"));

	// create NPC Definition (Profile)
	NPCDefinition = CreateDefaultSubobject<UNPCDefinitionComponent>(TEXT("NPCDefinition"));

	// Faction Reputation Component
	FactionReputationComponent = CreateDefaultSubobject<UFactionReputationComponent>(TEXT("FactionReputationComponent"));

	// set the collision capsule size
	GetCapsuleComponent()->SetCapsuleSize(35.0f, 90.0f);

	// set the character movement properties
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	// reset HP to maximum
	CurrentHP = MaxHP;
}

void ACombatEnemy::DoAIComboAttack()
{
	// ignore if we're already playing an attack animation
	if (bIsAttacking)
	{
		return;
	}

	// raise the attacking flag
	bIsAttacking = true;

	// choose how many times we're going to attack
	TargetComboCount = FMath::RandRange(1, ComboSectionNames.Num() - 1);

	// reset the attack counter
	CurrentComboAttack = 0;

	// play the attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ComboAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

		// subscribe to montage completed and interrupted events
		if (MontageLength > 0.0f)
		{
			// set the end delegate for the montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ComboAttackMontage);

			// begin continuous damage detection for the full attack window
			DamageDetection->StartDetection(MeleeDamage);
		}
	}
}

void ACombatEnemy::DoAIChargedAttack()
{
	// ignore if we're already playing an attack animation
	if (bIsAttacking)
	{
		return;
	}

	// raise the attacking flag
	bIsAttacking = true;

	// choose how many loops are we going to charge for
	TargetChargeLoops = FMath::RandRange(MinChargeLoops, MaxChargeLoops);

	// reset the charge loop counter
	CurrentChargeLoop = 0;

	// play the attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ChargedAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

		// subscribe to montage completed and interrupted events
		if (MontageLength > 0.0f)
		{
			// set the end delegate for the montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ChargedAttackMontage);

			// begin continuous damage detection for the full attack window
			DamageDetection->StartDetection(MeleeDamage);
		}
	}
}

void ACombatEnemy::AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// reset the attacking flag
	bIsAttacking = false;

	// close the damage detection window (also handles interrupted montages)
	DamageDetection->StopDetection();

	// call the attack completed delegate so the StateTree can continue execution
	OnAttackCompleted.ExecuteIfBound();
}

void ACombatEnemy::DoAttackTrace(FName DamageSourceBone)
{
	// If the continuous detection window is already running (started by DoAIComboAttack /
	// DoAIChargedAttack), the component handles hits every tick - nothing to do here.
	if (DamageDetection->IsDetecting())
	{
		return;
	}

	// Legacy fallback: AnimNotify fired outside the normal AI attack flow (e.g. player-driven
	// or Blueprint attack).  Fall back to the original single-frame forward sweep.
	TArray<FHitResult> OutHits;

	const FVector TraceStart = GetMesh()->GetSocketLocation(DamageSourceBone);
	const FVector TraceEnd   = TraceStart + (GetActorForwardVector() * MeleeTraceDistance);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(MeleeTraceRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
		for (const FHitResult& CurrentHit : OutHits)
		{
			if (CurrentHit.GetActor()->ActorHasTag(FName("Player")))
			{
				if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(CurrentHit.GetActor()))
				{
					const FVector Impulse = (CurrentHit.ImpactNormal * -MeleeKnockbackImpulse) + (FVector::UpVector * MeleeLaunchImpulse);
					Damageable->ApplyDamage(MeleeDamage, this, CurrentHit.ImpactPoint, Impulse);
				}
			}
		}
	}
}

void ACombatEnemy::CheckCombo()
{
	// increase the combo counter
	++CurrentComboAttack;

	// do we still have attacks to play in this string?
	if (CurrentComboAttack < TargetComboCount)
	{
		// jump to the next attack section
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_JumpToSection(ComboSectionNames[CurrentComboAttack], ComboAttackMontage);
		}
	}
}

void ACombatEnemy::CheckChargedAttack()
{
	// increase the charge loop counter
	++CurrentChargeLoop;

	// jump to either the loop or attack section of the montage depending on whether we hit the loop target
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(CurrentChargeLoop >= TargetChargeLoops ? ChargeAttackSection : ChargeLoopSection, ChargedAttackMontage);
	}
}

void ACombatEnemy::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (!CombatStats->IsAlive()) return;

	// track last attacker for death reporting
	if (DamageCauser)
	{
		LastDamageCauser = DamageCauser;
	}

	// delegate HP math and resistance to the component
	const float ActualDamage = CombatStats->ReceiveDamage(Damage, DamageCauser);

	if (ActualDamage > 0.0f)
	{
		// apply knockback scaled by the component's KnockbackResistance
		const FVector ScaledImpulse = CombatStats->ScaleKnockback(DamageImpulse);
		GetCharacterMovement()->AddImpulse(ScaledImpulse, true);

		if (GetMesh()->IsSimulatingPhysics())
		{
			GetMesh()->AddImpulseAtLocation(ScaledImpulse * GetMesh()->GetMass(), DamageLocation);
		}

		// interrupt any playing attack montages
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.1f, ComboAttackMontage);
			AnimInstance->Montage_Stop(0.1f, ChargedAttackMontage);
		}

		ReceivedDamage(ActualDamage, DamageLocation, DamageImpulse.GetSafeNormal());
	}
}

void ACombatEnemy::HandleDeath()
{
	COMBAT_LOG(Log, TEXT("%s died. Killer: %s"),
		*GetName(),
		LastDamageCauser ? *LastDamageCauser->GetName() : TEXT("None"));

	// Mark as dead for AI perception and narrative systems
	Tags.Add(FName("Dead"));
	Tags.Add(FName("Status.Dead"));
	
	// ✅ REPORT TO NARRATIVE DIRECTOR
	if (UWorld* World = GetWorld())
	{
		if (auto* Director = World->GetSubsystem<UNarrativeHistorySubsystem>())
		{
			Director->RecordNPCDeath(this, LastDamageCauser);
		}
	}

	// hide the life bar
	LifeBar->SetHiddenInGame(true);

	// disable the collision capsule to avoid being hit again while dead
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// disable character movement
	GetCharacterMovement()->DisableMovement();

	// enable full ragdoll physics
	GetMesh()->SetSimulatePhysics(true);

	// call the died delegate to notify any subscribers
	OnEnemyDied.Broadcast(this, LastDamageCauser);

	// ✅ Notify nearby AI Sensory systems
	// 通知附近的 AI 感知系统
	TArray<AActor*> NearbyControllers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUtilityAIController::StaticClass(), NearbyControllers);

	AController* MyController = GetController();
	
	for (AActor* ControllerActor : NearbyControllers)
	{
		if (AUtilityAIController* AICon = Cast<AUtilityAIController>(ControllerActor))
		{
			// ⚰️ Skip self - dead NPCs don't perceive their own death!
			// 跳过自己 - 死者不应该感知自己的死亡！
			if (AICon == MyController)
			{
				continue;
			}
			
			// Check distance (Sensory Range check, approx 30m)
			if (AICon->GetPawn() && FVector::DistSquared(GetActorLocation(), AICon->GetPawn()->GetActorLocation()) < 3000.0f * 3000.0f)
			{
				if (AICon->SensoryComp)
				{
					AICon->SensoryComp->HandleDeath(this, LastDamageCauser);
				}
				else if (USensoryComponent* FoundSensory = AICon->FindComponentByClass<USensoryComponent>())
				{
					FoundSensory->HandleDeath(this, LastDamageCauser);
				}
			}
		}
	}

	// set up the death timer
	GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &ACombatEnemy::RemoveFromLevel, DeathRemovalTime);
}

void ACombatEnemy::ApplyHealing(float Healing, AActor* Healer)
{
	CombatStats->RestoreHealth(Healing);
}

void ACombatEnemy::OnCombatHealthChanged(float NewHealth, float MaxHealth, float Delta)
{
	// Keep legacy mirror in sync
	CurrentHP = NewHealth;

	if (LifeBarWidget)
	{
		LifeBarWidget->SetLifePercentage(CombatStats->GetHealthPercent());
	}

	// Apply partial ragdoll physics on hit (only while alive)
	if (NewHealth > 0.0f)
	{
		GetMesh()->SetPhysicsBlendWeight(0.5f);
		GetMesh()->SetBodySimulatePhysics(PelvisBoneName, false);
	}
}

void ACombatEnemy::OnCombatDeath(AActor* Killer)
{
	// Sync mirror to 0 before HandleDeath reads/broadcasts anything
	CurrentHP = 0.0f;
	HandleDeath();
}

void ACombatEnemy::RemoveFromLevel()
{
	// destroy this actor
	Destroy();
}

float ACombatEnemy::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!CombatStats->IsAlive()) return 0.0f;

	if (DamageCauser)
	{
		LastDamageCauser = DamageCauser;
	}

	// Delegate HP management to CombatStatsComponent.
	// OnHealthChanged / OnDeath delegates handle UI and death flow.
	return CombatStats->ReceiveDamage(Damage, DamageCauser);
}

void ACombatEnemy::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// is the character still alive?
	if (CurrentHP >= 0.0f)
	{
		// disable ragdoll physics
		GetMesh()->SetPhysicsBlendWeight(0.0f);
	}

	// call the landed Delegate for StateTree
	OnEnemyLanded.ExecuteIfBound();
}

void ACombatEnemy::BeginPlay()
{
	// we call Super first so components (including CombatStatsComponent) initialize
	// and LoadFromTable + ResetHealth run before we read stats below
	Super::BeginPlay();

	// Configure AI Personality via NPCDefinition
	if (NPCDefinition)
	{
		COMBAT_LOG(Log, TEXT("%s: Applying NPC Definition..."), *GetName());

		// CRITICAL: Skip ApplyDefinition if MonsterComponent exists.
		// Monster settings are already applied in UtilityAIController::OnPossess;
		// calling ApplyDefinition here would overwrite those settings.
		if (UMonsterComponent* MonsterComp = FindComponentByClass<UMonsterComponent>())
		{
			COMBAT_LOG(Log, TEXT("%s: MonsterComponent detected - skipping NPCDefinition (configured in OnPossess)."), *GetName());
		}
		else if (AController* MyController = GetController())
		{
			if (AAIController* AICon = Cast<AAIController>(MyController))
			{
				// Schedule delayed initialization (100ms) to ensure Controller components are ready
				FTimerHandle ProfileTimerHandle;
				GetWorld()->GetTimerManager().SetTimer(
					ProfileTimerHandle,
					[this, AICon]()
					{
						if (NPCDefinition)
						{
							NPCDefinition->ApplyDefinition(AICon);
						}
					},
					0.1f,
					false
				);
			}
		}
	}

	// get the life bar widget from the widget comp
	LifeBarWidget = Cast<UCombatLifeBar>(LifeBar->GetUserWidgetObject());
	check(LifeBarWidget);

	// --- Wire up CombatStatsComponent ---

	// Keep the deprecated CurrentHP mirror in sync so existing Blueprints still work
	CombatStats->OnHealthChanged.AddDynamic(this, &ACombatEnemy::OnCombatHealthChanged);

	// Let the component's death event drive HandleDeath so HP logic stays in one place
	CombatStats->OnDeath.AddDynamic(this, &ACombatEnemy::OnCombatDeath);

	// Feed attack damage into DamageDetectionComponent so there's a single source of truth
	DamageDetection->BaseDamage = CombatStats->GetAttackDamage();

	// Apply movement speed from stats (CombatStatsComponent::BeginPlay runs first via
	// component registration order, so BaseStats is already populated from the table)
	GetCharacterMovement()->MaxWalkSpeed = CombatStats->GetWalkSpeed();

	// Sync legacy CurrentHP for Blueprints that read it directly
	CurrentHP = CombatStats->GetCurrentHealth();

	// Fill the life bar (component already reset health in its own BeginPlay)
	LifeBarWidget->SetLifePercentage(1.0f);
}

void ACombatEnemy::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the death timer
	GetWorld()->GetTimerManager().ClearTimer(DeathTimer);
}
