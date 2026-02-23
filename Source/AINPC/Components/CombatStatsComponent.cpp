// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/CombatStatsComponent.h"
#include "Components/NPCDefinitionComponent.h"
#include "AINPC.h"

UCombatStatsComponent::UCombatStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	// Auto-resolve StatsRowID from NPCDefinitionComponent when not explicitly configured.
	// This means adding both components to an NPC and setting DefinitionTemplateID is enough -
	// no manual StatsRowID entry needed.
	if (StatsTable && StatsRowID.IsNone())
	{
		if (AActor* Owner = GetOwner())
		{
			if (UNPCDefinitionComponent* NPCDef = Owner->FindComponentByClass<UNPCDefinitionComponent>())
			{
				if (!NPCDef->DefinitionTemplateID.IsNone())
				{
					StatsRowID = NPCDef->DefinitionTemplateID;
					COMBAT_LOG(Log, TEXT("Auto-resolved StatsRowID '%s' from NPCDefinitionComponent on '%s'"),
						*StatsRowID.ToString(), *Owner->GetName());
				}
			}
		}
	}

	if (StatsTable && !StatsRowID.IsNone())
	{
		LoadFromTable();
	}

	ResetHealth();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UCombatStatsComponent::LoadFromTable()
{
	if (!StatsTable)
	{
		COMBAT_LOG(Warning, TEXT("LoadFromTable called on '%s' but StatsTable is null."),
			*GetOwner()->GetName());
		return;
	}

	const FCombatStatsRow* Row = StatsTable->FindRow<FCombatStatsRow>(
		StatsRowID, TEXT("UCombatStatsComponent::LoadFromTable"));

	if (Row)
	{
		BaseStats = *Row;
		COMBAT_LOG(Log, TEXT("'%s' loaded stats row '%s': HP=%.0f ATK=%.1f RES=%.0f%% KB_RES=%.0f%% Speed=%.0f Range=%.0f"),
			*GetOwner()->GetName(),
			*StatsRowID.ToString(),
			BaseStats.MaxHealth,
			BaseStats.BaseAttackDamage,
			BaseStats.DamageResistance  * 100.f,
			BaseStats.KnockbackResistance * 100.f,
			BaseStats.WalkSpeed,
			BaseStats.AttackRange);
	}
	else
	{
		COMBAT_LOG(Warning, TEXT("'%s': row '%s' not found in StatsTable - using inline BaseStats."),
			*GetOwner()->GetName(), *StatsRowID.ToString());
	}
}

void UCombatStatsComponent::ResetHealth()
{
	CurrentHealth = BaseStats.MaxHealth;
	COMBAT_LOG(Verbose, TEXT("'%s' health reset to %.0f / %.0f"),
		*GetOwner()->GetName(), CurrentHealth, BaseStats.MaxHealth);
}

// ---------------------------------------------------------------------------
// Damage / Healing
// ---------------------------------------------------------------------------

float UCombatStatsComponent::ReceiveDamage(float RawDamage, AActor* DamageCauser)
{
	if (!IsAlive()) return 0.0f;

	const float Resistance   = FMath::Clamp(BaseStats.DamageResistance, 0.0f, 0.99f);
	const float ActualDamage = RawDamage * (1.0f - Resistance);

	if (ActualDamage <= 0.0f) return 0.0f;

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Max(CurrentHealth - ActualDamage, 0.0f);
	const float Delta = CurrentHealth - PreviousHealth; // negative

	COMBAT_LOG(Log, TEXT("'%s' received %.1f dmg (raw %.1f, res %.0f%%) -> HP %.0f / %.0f"),
		*GetOwner()->GetName(),
		ActualDamage, RawDamage,
		Resistance * 100.f,
		CurrentHealth, BaseStats.MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, BaseStats.MaxHealth, Delta);

	if (CurrentHealth <= 0.0f)
	{
		COMBAT_LOG(Log, TEXT("'%s' died. Killer: %s"),
			*GetOwner()->GetName(),
			DamageCauser ? *DamageCauser->GetName() : TEXT("None"));
		OnDeath.Broadcast(DamageCauser);
	}

	return ActualDamage;
}

void UCombatStatsComponent::RestoreHealth(float Amount)
{
	if (!IsAlive() || Amount <= 0.0f) return;

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Min(CurrentHealth + Amount, BaseStats.MaxHealth);
	const float Delta = CurrentHealth - PreviousHealth;

	COMBAT_LOG(Log, TEXT("'%s' healed %.1f -> HP %.0f / %.0f"),
		*GetOwner()->GetName(), Delta, CurrentHealth, BaseStats.MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, BaseStats.MaxHealth, Delta);
}

// ---------------------------------------------------------------------------
// Knockback helper
// ---------------------------------------------------------------------------

FVector UCombatStatsComponent::ScaleKnockback(const FVector& RawImpulse) const
{
	const float Resistance = FMath::Clamp(BaseStats.KnockbackResistance, 0.0f, 1.0f);
	return RawImpulse * (1.0f - Resistance);
}

// ---------------------------------------------------------------------------
// Stat accessors
// ---------------------------------------------------------------------------

float UCombatStatsComponent::GetHealthPercent() const
{
	if (BaseStats.MaxHealth <= 0.0f) return 0.0f;
	return FMath::Clamp(CurrentHealth / BaseStats.MaxHealth, 0.0f, 1.0f);
}
