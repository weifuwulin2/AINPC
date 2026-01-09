#include "Components/SmartObjectComponent.h"
#include "Interface/SmartObjectInterface.h"

USmartObjectComponent::USmartObjectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USmartObjectComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USmartObjectComponent::GetAffordances_Implementation(TMap<FString, float>& OutAffordances)
{
	// Bridge the new Semantics system to the legacy Affordances map
	// Use the explicit Tag name as the key
	const FString Key = Semantics.SocialTag.IsValid() ? Semantics.SocialTag.ToString() : TEXT("None");
	OutAffordances.Add(Key, Semantics.Intensity);
}

bool USmartObjectComponent::Interact_Implementation(AActor* User)
{
	// In the new architecture (Phase 3), interaction logic will be standardized.
	// For now, we return true to indicate this component handles interaction capabilities.
	// The "Nervous System" will read the Semantics directly.
	return true;
}
