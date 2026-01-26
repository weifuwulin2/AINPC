#include "Components/SmartObjectComponent.h"
#include "Interface/SmartObjectInterface.h"
#include "Subsystems/SmartObjectManager.h"
#include "Engine/World.h"
#include "Components/ArrowComponent.h"
#include "AINPC.h"

USmartObjectComponent::USmartObjectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// Default: Single slot at actor origin
	SlotOffsets.Add(FVector::ZeroVector);
}

void USmartObjectComponent::BeginPlay()
{
	Super::BeginPlay();

	// ✅ Initialize Slots from configured offsets
	InitializeSlots();

#if WITH_EDITOR
	// Create debug arrows for visualization in PIE
	CreateDebugArrows();
#endif

	// ✅ Auto-Register with SmartObjectManager
	if (UWorld* World = GetWorld())
	{
		if (USmartObjectManager* Manager = World->GetSubsystem<USmartObjectManager>())
		{
			if (Semantics.SocialTag.IsValid())
			{
				Manager->RegisterSmartObject(GetOwner(), Semantics.SocialTag);
				AINPC_LOG(Log, "%s registered as %s (Slots: %d)", 
					   *GetOwner()->GetName(), *Semantics.SocialTag.ToString(), Slots.Num());
			}
		}
	}
}

void USmartObjectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ✅ Auto-Unregister
	if (UWorld* World = GetWorld())
	{
		if (USmartObjectManager* Manager = World->GetSubsystem<USmartObjectManager>())
		{
			if (Semantics.SocialTag.IsValid())
			{
				Manager->UnregisterSmartObject(GetOwner(), Semantics.SocialTag);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void USmartObjectComponent::InitializeSlots()
{
	Slots.Empty();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Get Actor's ground-level Z (we'll use this for all slots to avoid underground issues)
	const float ActorZ = Owner->GetActorLocation().Z;

	// Use configured SlotOffsets
	if (SlotOffsets.Num() > 0)
	{
		for (int32 i = 0; i < SlotOffsets.Num(); ++i)
		{
			FSmartObjectSlot NewSlot;
			
			// Transform offset to world space, but KEEP Z at Actor level
			// This prevents slots from ending up underground if the mesh origin is below ground
			FVector WorldLoc = Owner->GetActorTransform().TransformPosition(SlotOffsets[i]);
			WorldLoc.Z = ActorZ; // ✅ Force Z to Actor's Z level (ground)
			
			NewSlot.Location = WorldLoc;
			NewSlot.Rotation = Owner->GetActorRotation();
			NewSlot.bIsReserved = false;
			NewSlot.Reserver = nullptr;

			Slots.Add(NewSlot);
		}

		AINPC_LOG(Log, "%s: Initialized %d slots (Z=%.0f)", *Owner->GetName(), Slots.Num(), ActorZ);
	}
	else
	{
		// Fallback: Single slot at actor location
		FSmartObjectSlot DefaultSlot;
		DefaultSlot.Location = Owner->GetActorLocation();
		DefaultSlot.Rotation = Owner->GetActorRotation();
		DefaultSlot.bIsReserved = false;
		DefaultSlot.Reserver = nullptr;

		Slots.Add(DefaultSlot);

		AINPC_LOG(Log, "%s: No SlotOffsets defined, using single slot at origin", *Owner->GetName());
	}
}

#if WITH_EDITOR
void USmartObjectComponent::CreateDebugArrows()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Clear existing arrows
	for (UArrowComponent* Arrow : DebugArrows)
	{
		if (Arrow)
		{
			Arrow->DestroyComponent();
		}
	}
	DebugArrows.Empty();

	// Create an arrow for each slot
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		UArrowComponent* Arrow = NewObject<UArrowComponent>(Owner);
		if (Arrow)
		{
			Arrow->SetupAttachment(Owner->GetRootComponent());
			Arrow->SetWorldLocation(Slots[i].Location);
			Arrow->SetWorldRotation(Slots[i].Rotation);
			Arrow->SetArrowColor(FLinearColor::Green);
			Arrow->ArrowSize = 0.5f;
			Arrow->bIsEditorOnly = true;
			Arrow->RegisterComponent();
			
			DebugArrows.Add(Arrow);
		}
	}
}
#endif

FVector USmartObjectComponent::GetInteractionLocation(int32 SlotIndex) const
{
	if (Slots.IsValidIndex(SlotIndex))
	{
		return Slots[SlotIndex].Location;
	}

	// Fallback to actor location
	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorLocation();
	}
	return FVector::ZeroVector;
}

int32 USmartObjectComponent::TryReserveSlot(AActor* Reserver)
{
	if (!Reserver) return -1;

	// First check if this actor already has a slot
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].Reserver == Reserver)
		{
			return i; // Already reserved by this actor
		}
	}

	// Find first available slot
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (!Slots[i].bIsReserved)
		{
			Slots[i].bIsReserved = true;
			Slots[i].Reserver = Reserver;

			AINPC_LOG(Log, "%s: Slot %d reserved by %s", 
				   *GetOwner()->GetName(), i, *Reserver->GetName());

			return i;
		}
	}

	// No available slots
	AINPC_LOG(Warning, "%s: All %d slots are occupied!", 
		   *GetOwner()->GetName(), Slots.Num());

	return -1;
}

void USmartObjectComponent::ReleaseSlot(int32 SlotIndex, AActor* Reserver)
{
	if (!Slots.IsValidIndex(SlotIndex)) return;

	// Only the reserver (or nullptr for force-release) can release
	if (Reserver == nullptr || Slots[SlotIndex].Reserver == Reserver)
	{
		AINPC_LOG(Log, "%s: Slot %d released by %s", 
			   *GetOwner()->GetName(), SlotIndex, 
			   Slots[SlotIndex].Reserver ? *Slots[SlotIndex].Reserver->GetName() : TEXT("Force"));

		Slots[SlotIndex].bIsReserved = false;
		Slots[SlotIndex].Reserver = nullptr;
	}
}

bool USmartObjectComponent::HasAvailableSlot() const
{
	for (const FSmartObjectSlot& Slot : Slots)
	{
		if (!Slot.bIsReserved)
		{
			return true;
		}
	}
	return false;
}

AActor* USmartObjectComponent::GetSlotReserver(int32 SlotIndex) const
{
	if (Slots.IsValidIndex(SlotIndex))
	{
		return Slots[SlotIndex].Reserver;
	}
	return nullptr;
}

void USmartObjectComponent::GetAffordances_Implementation(TMap<FString, float>& OutAffordances)
{
	const FString Key = Semantics.SocialTag.IsValid() ? Semantics.SocialTag.ToString() : TEXT("None");
	OutAffordances.Add(Key, Semantics.Intensity);
}

bool USmartObjectComponent::Interact_Implementation(AActor* User)
{
	return true;
}
