// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/SmartObjectInterface.h"
#include "SmartFoodActor.generated.h"

UCLASS()
class AINPC_API ASmartFoodActor : public AActor, public ISmartObjectInterface
{
	GENERATED_BODY()
	
public:	
	ASmartFoodActor();

	// --- 实现接口 ---
	// Override 这里要用 _Implementation，这是 UE BlueprintNativeEvent 的规则
	virtual void GetAffordances_Implementation(TMap<FString, float>& OutAffordances) override;
	virtual bool Interact_Implementation(AActor* User) override;

protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;

};
