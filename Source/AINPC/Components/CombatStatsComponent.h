// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "CombatStatsComponent.generated.h"

// ============================================================================
// Data Table Row
// ============================================================================

/**
 * FCombatStatsRow
 *
 * One row in a DataTable that defines a full set of combat stats for an NPC archetype.
 * Example rows: "Orc_Warrior", "Goblin_Scout", "BossKing".
 *
 * Assign the DataTable and a RowID to UCombatStatsComponent to auto-load stats on BeginPlay.
 * If no table/row is set, the component uses its inline BaseStats values instead.
 */
USTRUCT(BlueprintType)
struct FCombatStatsRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Maximum hit points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = 1))
	float MaxHealth = 100.0f;

	/** Base melee/ranged damage passed to DamageDetectionComponent each swing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offense", meta = (ClampMin = 0))
	float BaseAttackDamage = 10.0f;

	/**
	 * Flat damage resistance [0, 1].
	 * Incoming damage is multiplied by (1 - DamageResistance).
	 * 0 = no resistance, 0.5 = half damage, 1 = immune (clamped to 0.99).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense",
		meta = (ClampMin = 0, ClampMax = 0.99f))
	float DamageResistance = 0.0f;

	/**
	 * Knockback impulse reduction [0, 1].
	 * The incoming impulse vector is scaled by (1 - KnockbackResistance).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense",
		meta = (ClampMin = 0, ClampMax = 1))
	float KnockbackResistance = 0.0f;

	/** CharacterMovement MaxWalkSpeed applied on BeginPlay (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement",
		meta = (ClampMin = 0, Units = "cm/s"))
	float WalkSpeed = 300.0f;

	/** Preferred melee engagement range; used as an AI hint (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
		meta = (ClampMin = 0, Units = "cm"))
	float AttackRange = 150.0f;
};

// ============================================================================
// Delegates
// ============================================================================

/** Fires whenever health changes. Delta is positive for healing, negative for damage. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged,
	float, CurrentHealth,
	float, MaxHealth,
	float, Delta);

/** Fires once when health reaches zero. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, AActor*, Killer);

// ============================================================================
// Component
// ============================================================================

/**
 * UCombatStatsComponent
 *
 * Centralizes all combat stats for an NPC: HP, attack damage, defense, movement speed, etc.
 *
 * Usage:
 *  1. Add to any ACharacter / AActor.
 *  2. Set StatsTable + StatsRowID in editor for DataTable-driven setup,
 *     OR configure BaseStats inline.
 *  3. Call ReceiveDamage() from TakeDamage / ICombatDamageable::ApplyDamage.
 *  4. Bind OnHealthChanged and OnDeath delegates to react to HP events.
 *  5. Use GetAttackDamage() to feed DamageDetectionComponent::BaseDamage.
 *  6. Use GetWalkSpeed() to initialize CharacterMovement::MaxWalkSpeed.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AINPC_API UCombatStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCombatStatsComponent();

	// -----------------------------------------------------------------------
	// DataTable configuration
	// -----------------------------------------------------------------------

	/**
	 * Optional DataTable of FCombatStatsRow entries.
	 * When set alongside StatsRowID, BeginPlay will call LoadFromTable()
	 * to populate BaseStats automatically.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats|Table",
		meta = (RowType = "CombatStatsRow"))
	UDataTable* StatsTable;

	/**
	 * Row name inside StatsTable to load (e.g. "Orc_Warrior").
	 * Leave None to use inline BaseStats values.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats|Table")
	FName StatsRowID;

	// -----------------------------------------------------------------------
	// Inline stats (used directly when no table row is loaded)
	// -----------------------------------------------------------------------

	/**
	 * Editable stat block. Auto-populated from StatsTable on BeginPlay if configured;
	 * otherwise use these values directly (great for one-off NPCs).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats|Inline")
	FCombatStatsRow BaseStats;

	// -----------------------------------------------------------------------
	// Runtime state
	// -----------------------------------------------------------------------

	/** Current health - read-only in editor, managed at runtime */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat Stats")
	float CurrentHealth = 0.0f;

	// -----------------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------------

	/** Fires whenever HP changes (damage or healing) */
	UPROPERTY(BlueprintAssignable, Category = "Combat Stats|Events")
	FOnHealthChanged OnHealthChanged;

	/** Fires once when HP reaches zero */
	UPROPERTY(BlueprintAssignable, Category = "Combat Stats|Events")
	FOnDeath OnDeath;

	// -----------------------------------------------------------------------
	// API - Lifecycle
	// -----------------------------------------------------------------------

	/** Load stats from StatsTable row. Called automatically in BeginPlay if table/ID are set. */
	UFUNCTION(BlueprintCallable, Category = "Combat Stats")
	void LoadFromTable();

	/** Set CurrentHealth to MaxHealth. */
	UFUNCTION(BlueprintCallable, Category = "Combat Stats")
	void ResetHealth();

	// -----------------------------------------------------------------------
	// API - Damage / Healing
	// -----------------------------------------------------------------------

	/**
	 * Apply incoming damage, accounting for DamageResistance.
	 * @param RawDamage    Raw damage amount before resistance.
	 * @param DamageCauser Actor dealing the damage (used for OnDeath event).
	 * @return             Actual damage dealt after resistance (0 if already dead).
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Stats")
	float ReceiveDamage(float RawDamage, AActor* DamageCauser);

	/**
	 * Restore health, clamped to MaxHealth.
	 * @param Amount Positive healing amount.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Stats")
	void RestoreHealth(float Amount);

	// -----------------------------------------------------------------------
	// API - Knockback helper
	// -----------------------------------------------------------------------

	/**
	 * Scale an incoming knockback impulse by (1 - KnockbackResistance).
	 * Pass the raw impulse and apply the returned vector to the character.
	 */
	UFUNCTION(BlueprintPure, Category = "Combat Stats")
	FVector ScaleKnockback(const FVector& RawImpulse) const;

	// -----------------------------------------------------------------------
	// API - Stat accessors
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Combat Stats")
	bool IsAlive() const { return CurrentHealth > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Combat Stats")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Combat Stats")
	float GetMaxHealth() const { return BaseStats.MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Combat Stats")
	float GetCurrentHealth() const { return CurrentHealth; }

	/** Base damage to feed into DamageDetectionComponent::BaseDamage */
	UFUNCTION(BlueprintPure, Category = "Combat Stats")
	float GetAttackDamage() const { return BaseStats.BaseAttackDamage; }

	/** Walk speed to apply to CharacterMovement::MaxWalkSpeed */
	UFUNCTION(BlueprintPure, Category = "Combat Stats")
	float GetWalkSpeed() const { return BaseStats.WalkSpeed; }

	/** Preferred attack range for AI decision-making (cm) */
	UFUNCTION(BlueprintPure, Category = "Combat Stats")
	float GetAttackRange() const { return BaseStats.AttackRange; }

protected:

	virtual void BeginPlay() override;
};
