#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Social/SocialTypes.h"
#include "GameplayTagContainer.h"
#include "SmartObjectManager.generated.h"

// Helper struct for TMap values
USTRUCT()
struct FSmartObjectList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AActor*> Objects;
};

/**
 * Manages the registry of Smart Objects in the world.
 * Handles lookups (Spatial Query) and Reservations (Resource Contention).
 */
UCLASS()
class AINPC_API USmartObjectManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Registration ---

	UFUNCTION(BlueprintCallable, Category = "AI | SmartObjects")
	void RegisterSmartObject(AActor* SmartObject, FGameplayTag ActivityTag);

	UFUNCTION(BlueprintCallable, Category = "AI | SmartObjects")
	void UnregisterSmartObject(AActor* SmartObject, FGameplayTag ActivityTag);

	// --- Queries ---

	/**
	 * Finds the best available (unreserved) Smart Object for the given tag.
	 * @param RequestingActor - The actor looking for the object (used for distance check)
	 * @param ActivityTag - The semantic tag (e.g., Activity.Eat)
	 * @param SearchRadius - Max distance to search (0 = Infinite)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI | SmartObjects")
	AActor* FindBestSmartObject(AActor* RequestingActor, FGameplayTag ActivityTag, float SearchRadius = 0.0f);

	// --- Reservation System ---

	/**
	 * Attempts to reserve a Smart Object for exclusive use.
	 * @return true if reservation succeeded, false if already reserved by someone else.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI | SmartObjects")
	bool TryReserveSmartObject(AActor* SmartObject, AActor* Reserver);

	/**
	 * Releases the reservation on a Smart Object.
	 * Use bForce=true to break someone else's reservation (Admin only).
	 */
	UFUNCTION(BlueprintCallable, Category = "AI | SmartObjects")
	void ReleaseReservation(AActor* SmartObject, AActor* Reserver);

	/**
	 * Checks if an object is currently reserved.
	 */
	UFUNCTION(BlueprintPure, Category = "AI | SmartObjects")
	bool IsReserved(AActor* SmartObject) const;

	UFUNCTION(BlueprintPure, Category = "AI | SmartObjects")
	AActor* GetReserver(AActor* SmartObject) const;

private:
	// Registry: Tag -> List of Actors
	UPROPERTY()
	TMap<FGameplayTag, FSmartObjectList> Registry; 

	// Reservations: SmartObject -> Reserver
	UPROPERTY()
	TMap<AActor*, AActor*> Reservations;
};


