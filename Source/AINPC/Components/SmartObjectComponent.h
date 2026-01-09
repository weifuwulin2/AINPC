#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Social/SocialTypes.h"
#include "Interface/SmartObjectInterface.h"
#include "SmartObjectComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API USmartObjectComponent : public UActorComponent, public ISmartObjectInterface
{
	GENERATED_BODY()

public:	
	USmartObjectComponent();

	// The social semantics description for this object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social")
	FSmartObjectSemantics Semantics;

	// --- ISmartObjectInterface Implementation ---
	virtual void GetAffordances_Implementation(TMap<FString, float>& OutAffordances) override;
	virtual bool Interact_Implementation(AActor* User) override;

protected:
	virtual void BeginPlay() override;
};
