#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingBase.generated.h"

class AVillageAnchor;

/**
 * Minimal building actor that can notify VillageAnchor on construct/remove.
 * Use this as a base for runtime village buildings.
 */
UCLASS(Blueprintable)
class AINPC_API ABuildingBase : public AActor
{
	GENERATED_BODY()

public:
	ABuildingBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Bind this building to a village anchor. Optionally emits constructed event immediately. */
	UFUNCTION(BlueprintCallable, Category = "Building|Village")
	void BindToVillageAnchor(AVillageAnchor* InVillageAnchor, bool bNotifyConstruction = true);

	/** Resolve nearby/overlapping village anchor and bind to it (collision query first, full scan fallback). */
	UFUNCTION(BlueprintCallable, Category = "Building|Village")
	void ResolveAndBindVillageAnchor(bool bNotifyConstruction = true);

	UFUNCTION(BlueprintPure, Category = "Building|Village")
	AVillageAnchor* GetBoundVillageAnchor() const { return BoundVillageAnchor.Get(); }

	/** Enable/disable all auto village notify behavior for this building. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Village")
	bool bAutoNotifyVillageAnchor = true;

	/** Resolve nearest anchor at BeginPlay if VillageAnchorOverride is not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Village")
	bool bAutoResolveVillageAnchorOnBeginPlay = true;

	/** Search range for nearest village anchor resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Village", meta = (ClampMin = "100.0"))
	float MaxVillageAnchorSearchRadius = 20000.0f;

	/** If true, only bind to anchors whose current territory contains this building. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Village")
	bool bRequireInsideVillageTerritory = false;

	/** Optional explicit anchor binding target; if set it wins over auto search. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Village")
	TObjectPtr<AVillageAnchor> VillageAnchorOverride = nullptr;

	/** Override in child BP/C++ for decorative-only buildings that should not affect village scale. */
	UFUNCTION(BlueprintNativeEvent, Category = "Building|Village")
	bool ShouldContributeToVillageTerritory() const;
	virtual bool ShouldContributeToVillageTerritory_Implementation() const;

private:
	AVillageAnchor* FindBestVillageAnchor() const;
	void NotifyConstructedToAnchor();
	void NotifyRemovedToAnchor();

	TWeakObjectPtr<AVillageAnchor> BoundVillageAnchor;
	bool bHasNotifiedConstructed = false;
};
