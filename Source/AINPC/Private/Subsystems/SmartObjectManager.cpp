#include "Subsystems/SmartObjectManager.h"
#include "VisualLogger/VisualLogger.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SmartObjectComponent.h"

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

		// ✅ NEW: Check Slot Availability via SmartObjectComponent
		// 新增：通过 SmartObjectComponent 检查槽位可用性
		USmartObjectComponent* SmartComp = Candidate->FindComponentByClass<USmartObjectComponent>();
		if (SmartComp)
		{
			if (!SmartComp->HasAvailableSlot())
			{
				// All slots occupied, skip this object
				continue;
			}
		}
		else
		{
			// Legacy: Fallback to simple reservation check
			if (IsReserved(Candidate))
			{
				if (GetReserver(Candidate) != RequestingActor)
				{
					continue;
				}
			}
		}

		// 2. Distance Check
		float DistSq = FVector::DistSquared(Origin, Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
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
