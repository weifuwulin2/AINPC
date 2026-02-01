// Action_Flee.cpp
// Implementation of flee to random safe location

#include "Actions/Action_Flee.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"
#include "AINPC.h"

UAction_Flee::UAction_Flee()
{
	ActionName = "Flee";
}

void UAction_Flee::Enter_Implementation(AAIController* Controller)
{
	Super::Enter_Implementation(Controller);
	
	if (!Controller) return;
	
	// Reset state
	CurrentThreat = nullptr;
	bHasValidFleeLocation = false;
	FleeDestination = FVector::ZeroVector;
	LastLocationUpdateTime = 0.0f;
	
	// Save original movement speed
	if (APawn* Pawn = Controller->GetPawn())
	{
		// Tag as fleeing (so TargetSelectionSubsystem can prioritize this target)
		Pawn->Tags.AddUnique(FName("Status.Fleeing"));

		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
			{
				OriginalMovementSpeed = MovementComp->MaxWalkSpeed;

				// Increase speed for fleeing
				MovementComp->MaxWalkSpeed = OriginalMovementSpeed * FleeSpeedMultiplier;

				AINPC_LOG(Warning, "[Flee] Increased speed to %.0f (multiplier: %.1fx)",
					MovementComp->MaxWalkSpeed, FleeSpeedMultiplier);
			}
		}
	}
	
	AINPC_LOG(Warning, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
	AINPC_LOG(Warning, "🏃 FLEE ACTION STARTED - RUNNING FOR SAFETY!");
	AINPC_LOG(Warning, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}

void UAction_Flee::Execute_Implementation(AAIController* Controller)
{
	Super::Execute_Implementation(Controller);
	
	if (!Controller) return;
	
	UWorld* World = Controller->GetWorld();
	if (!World) return;
	
	APawn* ControlledPawn = Controller->GetPawn();
	if (!ControlledPawn) return;
	
	float CurrentTime = World->GetTimeSeconds();
	
	// =========================================================
	// 1. Find or Update Threat
	// =========================================================
	if (!CurrentThreat || !IsValid(CurrentThreat))
	{
		if (!FindClosestThreat(Controller))
		{
			AINPC_LOG(Warning, "[Flee] No threat found, action should end soon");
			return;
		}
	}
	
	// =========================================================
	// 2. Calculate/Update Flee Location
	// =========================================================
	bool bNeedsNewLocation = !bHasValidFleeLocation || 
	                         (CurrentTime - LastLocationUpdateTime > LocationUpdateInterval);
	
	if (bNeedsNewLocation)
	{
		if (CalculateRandomFleeLocation(Controller))
		{
			LastLocationUpdateTime = CurrentTime;
			
			// Move to new flee destination
			Controller->MoveToLocation(FleeDestination, 100.0f, true, true, true);
			
			float DistanceToThreat = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentThreat->GetActorLocation());
			AINPC_LOG(Warning, "[Flee] New flee destination: %s (Distance from threat: %.0f cm)", 
				*FleeDestination.ToString(), DistanceToThreat);
		}
		else
		{
			AINPC_LOG_ERROR("[Flee] Failed to find flee location!");
		}
	}
	
	// =========================================================
	// 3. Check if reached destination
	// =========================================================
	if (bHasValidFleeLocation)
	{
		float DistToDest = FVector::Dist(ControlledPawn->GetActorLocation(), FleeDestination);
		if (DistToDest < 200.0f) // Within 2 meters
		{
			// Reached destination, find a new one
			AINPC_LOG(Log, "[Flee] Reached flee location, finding new position...");
			bHasValidFleeLocation = false;
		}
	}
	
	// =========================================================
	// 4. Debug Logging (every 2 seconds)
	// =========================================================
	static float LastLogTime = 0.0f;
	if (CurrentTime - LastLogTime > 2.0f)
	{
		LastLogTime = CurrentTime;
		
		if (CurrentThreat)
		{
			float DistToThreat = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentThreat->GetActorLocation());
			UE_LOG(LogTemp, Warning, TEXT("[Flee] 🏃 Fleeing from %s (Distance: %.0f cm)"), 
				*CurrentThreat->GetName(), DistToThreat);
		}
	}
}

void UAction_Flee::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);
	
	// Stop movement
	if (Controller)
	{
		Controller->StopMovement();
	}
	
	// Restore original movement speed and remove fleeing tag
	if (Controller)
	{
		if (APawn* Pawn = Controller->GetPawn())
		{
			Pawn->Tags.Remove(FName("Status.Fleeing"));

			if (OriginalMovementSpeed > 0.0f)
			{
				if (ACharacter* Character = Cast<ACharacter>(Pawn))
				{
					if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
					{
						MovementComp->MaxWalkSpeed = OriginalMovementSpeed;
						AINPC_LOG(Log, "[Flee] Restored movement speed to %.0f", OriginalMovementSpeed);
					}
				}
			}
		}
	}
	
	AINPC_LOG(Warning, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
	AINPC_LOG(Warning, "🛑 FLEE ACTION ENDED - Returning to normal");
	AINPC_LOG(Warning, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}

// =========================================================
// Helper Functions
// =========================================================

bool UAction_Flee::FindClosestThreat(AAIController* Controller)
{
	if (!Controller) return false;
	
	UWorld* World = Controller->GetWorld();
	if (!World) return false;
	
	APawn* ControlledPawn = Controller->GetPawn();
	if (!ControlledPawn) return false;
	
	float MinDistance = FLT_MAX;
	AActor* BestThreat = nullptr;
	
	// Search for actors with threat tags
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!Candidate || Candidate == ControlledPawn) continue;
		if (Candidate->IsPendingKillPending()) continue;
		
		// Check if actor has any of the threat tags
		bool bIsThreat = false;
		for (const FName& ThreatTag : ThreatTags)
		{
			if (Candidate->ActorHasTag(ThreatTag))
			{
				bIsThreat = true;
				break;
			}
		}
		
		if (!bIsThreat) continue;
		
		float Dist = FVector::Dist(ControlledPawn->GetActorLocation(), Candidate->GetActorLocation());
		if (Dist < MinDistance)
		{
			MinDistance = Dist;
			BestThreat = Candidate;
		}
	}
	
	// If no tagged threats found, try player
	if (!BestThreat && bFleeFromPlayerIfNoThreats)
	{
		BestThreat = UGameplayStatics::GetPlayerPawn(World, 0);
		if (BestThreat == ControlledPawn) BestThreat = nullptr;
		
		if (BestThreat)
		{
			MinDistance = FVector::Dist(ControlledPawn->GetActorLocation(), BestThreat->GetActorLocation());
			AINPC_LOG(Warning, "[Flee] No tagged threats, fleeing from player");
		}
	}
	
	if (BestThreat)
	{
		CurrentThreat = BestThreat;
		AINPC_LOG(Warning, "[Flee] ⚠️ Threat identified: %s (Distance: %.0f cm)", 
			*CurrentThreat->GetName(), MinDistance);
		return true;
	}
	
	return false;
}

bool UAction_Flee::CalculateRandomFleeLocation(AAIController* Controller)
{
	if (!Controller || !CurrentThreat) return false;
	
	UWorld* World = Controller->GetWorld();
	if (!World) return false;
	
	APawn* ControlledPawn = Controller->GetPawn();
	if (!ControlledPawn) return false;
	
	FVector PawnLocation = ControlledPawn->GetActorLocation();
	FVector ThreatLocation = CurrentThreat->GetActorLocation();
	
	// =========================================================
	// Calculate random direction away from threat
	// =========================================================
	FVector BaseFleeDirection = GetRandomDirectionAwayFromThreat(PawnLocation, ThreatLocation);
	
	// Random distance between min and max
	float FleeDistance = FMath::RandRange(MinFleeDistance, MaxFleeDistance);
	
	// Calculate target position
	FVector TargetPosition = PawnLocation + BaseFleeDirection * FleeDistance;
	
	// =========================================================
	// Project to NavMesh
	// =========================================================
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys)
	{
		AINPC_LOG_ERROR("[Flee] NavigationSystem not found!");
		return false;
	}
	
	FNavLocation NavLocation;
	FVector QueryExtent(NavSearchRadius, NavSearchRadius, 500.0f);
	
	if (NavSys->ProjectPointToNavigation(TargetPosition, NavLocation, QueryExtent))
	{
		FleeDestination = NavLocation.Location;
		bHasValidFleeLocation = true;
		
		float ActualDistance = FVector::Dist(PawnLocation, FleeDestination);
		AINPC_LOG(Log, "[Flee] ✅ Found flee location %.0f cm away (Direction: %s)", 
			ActualDistance, *BaseFleeDirection.ToString());
		
		return true;
	}
	else
	{
		// Fallback: Try with smaller extent
		FVector SmallExtent(1000.0f, 1000.0f, 500.0f);
		if (NavSys->ProjectPointToNavigation(TargetPosition, NavLocation, SmallExtent))
		{
			FleeDestination = NavLocation.Location;
			bHasValidFleeLocation = true;
			AINPC_LOG(Warning, "[Flee] ⚠️ Found flee location with reduced search radius");
			return true;
		}
		
		AINPC_LOG_ERROR("[Flee] ❌ Failed to project flee location to NavMesh!");
		return false;
	}
}

FVector UAction_Flee::GetRandomDirectionAwayFromThreat(const FVector& PawnLocation, const FVector& ThreatLocation)
{
	// Base direction: away from threat
	FVector AwayFromThreat = (PawnLocation - ThreatLocation).GetSafeNormal();
	
	// Add random offset for variety (±45 degrees)
	float RandomAngle = FMath::RandRange(-45.0f, 45.0f);
	FRotator Rotation(0.0f, RandomAngle, 0.0f);
	FVector RandomizedDirection = Rotation.RotateVector(AwayFromThreat);
	
	// Keep it horizontal (no Z movement)
	RandomizedDirection.Z = 0.0f;
	RandomizedDirection.Normalize();
	
	return RandomizedDirection;
}
