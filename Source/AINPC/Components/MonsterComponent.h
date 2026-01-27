
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MonsterComponent.generated.h"

/**
 * Helper component for "Brainless" entities (Zombies/Monsters).
 * Automatically configures Faction to 'Monsters' and Personality to 'Feral' on BeginPlay.
 * Simplifies setup so you don't need to manually config every Zombie's DataAsset.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API UMonsterComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMonsterComponent();

protected:
	virtual void BeginPlay() override;

public:	
    // Optional: Override Faction ID specifically for this monster type
    UPROPERTY(EditAnywhere, Category="Monster")
    FName MonsterFactionID = "Monsters";
};
