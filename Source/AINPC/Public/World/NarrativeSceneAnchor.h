#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NarrativeSceneAnchor.generated.h"

/**
 * A marker actor placed in the level to designate where Narrative Scenes can take place.
 * Registers itself with the NarrativeSquadSubsystem upon BeginPlay.
 */
UCLASS()
class AINPC_API ANarrativeSceneAnchor : public AActor
{
	GENERATED_BODY()
	
public:	
	ANarrativeSceneAnchor();

	// The ID of the Scene Template this anchor supports (e.g. "Scene_OrcRescue").
	// Leave empty if this anchor can support generic scenes (future expansion).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FName SupportedSceneID;

	// Is this anchor currently occupied by an active scene?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative")
	bool bIsOccupied = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative")
	int32 CurrentSquadID = -1;

	// --- Trigger Support ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative Trigger")
	class USphereComponent* TriggerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Trigger")
	class UDataTable* SceneTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Trigger")
	class UDataTable* NPCTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Trigger")
	bool bAutoTriggerOnOverlap = true;

	/** Activation radius for scene trigger (in Unreal units). Default 2000 = 20 meters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Trigger")
	float ActivationRadius = 2000.0f;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);};
