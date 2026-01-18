#include "Subsystems/SmartObjectManager.h"
#include "VisualLogger/VisualLogger.h"

// Helper struct definition needs to be known if we use it, but I put it in Header.
// Wait, I put it as FSmartObjectListWrapper in the map decl but defined FSmartObjectList. Fixed in cpp logic? 
// No, I need to fix the header first or assume the next write_to_file fixes it.
// Actually, I can just use TMap<FGameplayTag, FSmartObjectList> in the header if I define the struct inside or before.
// I will rely on the next step to fix the C++ compilation if the header was messy, but let's try to write a clean CPP that assumes the Header is fixed/valid.

// Wait, I messed up the header struct name in the tool call: `FSmartObjectListWrapper` vs `FSmartObjectList`.
// I should fix the header file first to be correct.

#include "Engine/World.h"
#include "GameFramework/Actor.h"

void USmartObjectManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USmartObjectManager::Deinitialize()
{
	Registry.Empty();
	Reservations.Empty();
	Super::Deinitialize();
}

void USmartObjectManager::RegisterSmartObject(AActor* SmartObject, FGameplayTag ActivityTag)
{
	if (!IsValid(SmartObject) || !ActivityTag.IsValid()) return;

	FSmartObjectList& List = Registry.FindOrAdd(ActivityTag);
	if (!List.Objects.Contains(SmartObject))
	{
		List.Objects.Add(SmartObject);
	}
}

void USmartObjectManager::UnregisterSmartObject(AActor* SmartObject, FGameplayTag ActivityTag)
{
	if (!IsValid(SmartObject)) return;

	if (FSmartObjectList* List = Registry.Find(ActivityTag))
	{
		List->Objects.Remove(SmartObject);
		
		// If using map cleanup
		if (List->Objects.Num() == 0)
		{
			// Registry.Remove(ActivityTag); // Optional
		}
	}
	
	// Force release reservation if registered
	if (Reservations.Contains(SmartObject))
	{
		Reservations.Remove(SmartObject);
	}
}

AActor* USmartObjectManager::FindBestSmartObject(AActor* RequestingActor, FGameplayTag ActivityTag, float SearchRadius)
{
	if (!RequestingActor || !ActivityTag.IsValid()) return nullptr;

	FSmartObjectList* List = Registry.Find(ActivityTag);
	if (!List || List->Objects.Num() == 0) return nullptr;

	AActor* BestCandidate = nullptr;
	float BestDistSq = (SearchRadius > 0.0f) ? (SearchRadius * SearchRadius) : MAX_flt;

	FVector Origin = RequestingActor->GetActorLocation();

	for (int32 i = List->Objects.Num() - 1; i >= 0; --i)
	{
		AActor* Candidate = List->Objects[i];
		
		// Cleanup stale ptrs
		if (!IsValid(Candidate)) 
		{
			List->Objects.RemoveAt(i);
			continue;
		}

		// 1. Check Reservation
		if (IsReserved(Candidate))
		{
			// Skip if reserved (unless reserved by me? No, FindBest usually implies finding a NEW one)
			if (GetReserver(Candidate) != RequestingActor)
			{
				continue;
			}
		}

		// 2. Distance Check
		float DistSq = FVector::DistSquared(Origin, Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			// Optional: Check Reachability (NavMesh) - expensive, maybe skip for now
			BestDistSq = DistSq;
			BestCandidate = Candidate;
		}
	}

	return BestCandidate;
}

bool USmartObjectManager::TryReserveSmartObject(AActor* SmartObject, AActor* Reserver)
{
	if (!IsValid(SmartObject) || !IsValid(Reserver)) return false;

	// Check if already reserved
	if (AActor* CurrentReserver = GetReserver(SmartObject))
	{
		if (CurrentReserver == Reserver) return true; // Already reserved by me
		return false; // Reserved by someone else
	}

	// Make reservation
	Reservations.Add(SmartObject, Reserver);
	return true;
}

void USmartObjectManager::ReleaseReservation(AActor* SmartObject, AActor* Reserver)
{
	if (!SmartObject) return;

	if (AActor* CurrentReserver = GetReserver(SmartObject))
	{
		// Only the owner can release, unless null passed (Force)
		if (Reserver == nullptr || CurrentReserver == Reserver)
		{
			Reservations.Remove(SmartObject);
		}
	}
}

bool USmartObjectManager::IsReserved(AActor* SmartObject) const
{
	return Reservations.Contains(SmartObject);
}

AActor* USmartObjectManager::GetReserver(AActor* SmartObject) const
{
	if (AActor* const* ReserverPtr = Reservations.Find(SmartObject))
	{
		return *ReserverPtr;
	}
	return nullptr;
}
