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

	// 恢复速率 (每秒恢复多少，0.0~1.0)
	// Restoration rate per second (how much Hunger/Energy to restore)
	// Example: 0.1 = slow restore (Apple), 0.5 = fast restore (Feast)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RestoreValue = 0.2f;

    // 交互点相对于Actor的偏移量 (可以在编辑器视口中直接拖拽调整)
    // Interaction point offset relative to the Actor (Can be dragged in Editor viewport)
    // Useful when the object is underground or unreachable directly.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social", meta = (MakeEditWidget = true))
    FVector InteractionOffset;

    // 获取实际的世界坐标交互点
    // Get the actual world space interaction location
    UFUNCTION(BlueprintCallable, Category = "Social")
    FVector GetInteractionLocation() const;

	// --- ISmartObjectInterface Implementation ---
	virtual void GetAffordances_Implementation(TMap<FString, float>& OutAffordances) override;
	virtual bool Interact_Implementation(AActor* User) override;

protected:
	virtual void BeginPlay() override;
};
