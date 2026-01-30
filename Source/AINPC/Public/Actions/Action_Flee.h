// Action_Flee.h
// Flee to a random safe location away from danger

#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "Action_Flee.generated.h"

/**
 * Flee Action - Escapes to a random safe location
 * 逃跑动作 - 逃跑到随机安全位置
 */
UCLASS(Blueprintable)
class AINPC_API UAction_Flee : public UUtilityActionBase
{
	GENERATED_BODY()

public:
	UAction_Flee();

	virtual void Enter_Implementation(AAIController* Controller) override;
	virtual void Execute_Implementation(AAIController* Controller) override;
	virtual void Exit_Implementation(AAIController* Controller) override;

protected:
	// =========================================================
	// Configuration Parameters
	// =========================================================
	
	/** Flee speed multiplier (applied to max walk speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	float FleeSpeedMultiplier = 1.5f;
	
	/** Minimum distance to flee from threat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	float MinFleeDistance = 1500.0f; // 15 meters
	
	/** Maximum distance to flee from threat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	float MaxFleeDistance = 3000.0f; // 30 meters
	
	/** How often to search for new flee location (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	float LocationUpdateInterval = 2.0f;
	
	/** NavMesh search radius for random location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	float NavSearchRadius = 5000.0f; // 50 meters
	
	/** Tags to identify threat actors (e.g., "Enemy", "Danger") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	TArray<FName> ThreatTags = { "Enemy" };
	
	/** Should flee from player if no tagged threats found? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	bool bFleeFromPlayerIfNoThreats = true;

private:
	// =========================================================
	// Runtime State
	// =========================================================
	
	/** Current identified threat actor */
	UPROPERTY()
	AActor* CurrentThreat = nullptr;
	
	/** Current flee destination */
	FVector FleeDestination = FVector::ZeroVector;
	
	/** Last time we updated flee location */
	float LastLocationUpdateTime = 0.0f;
	
	/** Original movement speed (to restore on exit) */
	float OriginalMovementSpeed = 0.0f;
	
	/** Whether we successfully found a flee location */
	bool bHasValidFleeLocation = false;
	
	// =========================================================
	// Helper Functions
	// =========================================================
	
	/**
	 * Find the closest threat actor
	 * @return True if threat found
	 */
	bool FindClosestThreat(AAIController* Controller);
	
	/**
	 * Calculate a random safe flee location
	 * @return True if valid location found
	 */
	bool CalculateRandomFleeLocation(AAIController* Controller);
	
	/**
	 * Get a random point away from threat
	 */
	FVector GetRandomDirectionAwayFromThreat(const FVector& PawnLocation, const FVector& ThreatLocation);
};
