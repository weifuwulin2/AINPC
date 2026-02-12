#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/ArrowComponent.h"
#include "Social/SocialTypes.h"
#include "Interface/SmartObjectInterface.h"
#include "SmartObjectComponent.generated.h"

/**
 * Smart Object Slot - Defines a single interaction point
 * 交互槽位 - 定义单个交互点
 */
USTRUCT(BlueprintType)
struct FSmartObjectSlot
{
	GENERATED_BODY()

	// World location of this slot
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	// World rotation for this slot (NPC facing direction)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotation = FRotator::ZeroRotator;

	// Is this slot currently reserved?
	bool bIsReserved = false;

	// Who reserved this slot?
	UPROPERTY(Transient)
	AActor* Reserver = nullptr;
};

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RestoreValue = 0.2f;

	// ✅ Slot Offsets: Configure multiple interaction points in the Editor
	// 在编辑器中配置多个交互点的偏移位置
	// Each offset will create an ArrowComponent for visualization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slots", meta = (MakeEditWidget = true))
	TArray<FVector> SlotOffsets;

	// ✅ Runtime Slot data (populated at BeginPlay)
	// 运行时槽位数据 (在 BeginPlay 时填充)
	UPROPERTY(BlueprintReadOnly, Category = "Slots")
	TArray<FSmartObjectSlot> Slots;

	// --- Slot API ---
	
	// Get number of available slots
	UFUNCTION(BlueprintCallable, Category = "Slots")
	int32 GetNumSlots() const { return Slots.Num(); }

	// Get interaction location for a specific slot
	UFUNCTION(BlueprintCallable, Category = "Slots")
	FVector GetInteractionLocation(int32 SlotIndex = 0) const;

	// Try to reserve a slot, returns slot index or -1 if all full
	UFUNCTION(BlueprintCallable, Category = "Slots")
	int32 TryReserveSlot(AActor* Reserver);

	// Release a slot
	UFUNCTION(BlueprintCallable, Category = "Slots")
	void ReleaseSlot(int32 SlotIndex, AActor* Reserver);

	// Check if any slot is available
	UFUNCTION(BlueprintCallable, Category = "Slots")
	bool HasAvailableSlot() const;

	// Get the actor who reserved a specific slot
	UFUNCTION(BlueprintCallable, Category = "Slots")
	AActor* GetSlotReserver(int32 SlotIndex) const;

	// --- ISmartObjectInterface Implementation ---
	virtual void GetAffordances_Implementation(TMap<FString, float>& OutAffordances) override;
	virtual bool Interact_Implementation(AActor* User) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRegister() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void RefreshEditorArrows();
#endif

#if WITH_EDITORONLY_DATA
	// Visual debug arrows (Editor only)
	UPROPERTY()
	TArray<UArrowComponent*> DebugArrows;
#endif

private:
	// Populate Slots array from SlotOffsets
	void InitializeSlots();
};
