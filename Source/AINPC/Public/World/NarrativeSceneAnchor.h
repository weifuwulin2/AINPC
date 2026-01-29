#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h" // For FGameplayTag
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
	
	/** Outer trigger: Activates the scene when player enters (larger radius) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative Trigger")
	class USphereComponent* SceneActivationTrigger;

	/** Inner trigger: Broadcasts Event.PlayerDetected when player gets close (smaller radius) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative Trigger")
	class USphereComponent* EventTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Trigger")
	class UDataTable* SceneTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Trigger")
	class UDataTable* NPCTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Trigger")
	bool bAutoTriggerOnOverlap = true;

	/** Outer trigger radius for scene activation (in Unreal units). Default 2000 = 20 meters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Trigger")
	float SceneActivationRadius = 2000.0f;

	/** Inner trigger radius for event broadcasting (in Unreal units). Default 800 = 8 meters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Trigger", meta=(EditCondition="bAutoTriggerOnOverlap"))
	float EventTriggerRadius = 800.0f;

	// --- Dynamic Control API ---

	/** 
	 * Applies a GameplayTag to all actors with the specified RoleID in the current scene.
	 * Delegates to NarrativeSquadSubsystem::ApplyTagToRole.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Control")
	void ApplyTagToRole(FString RoleID, FGameplayTag Tag);

	/**
	 * Helper: Applies 'Event.Danger.Combat' to the specified RoleID.
	 * Forces actors to enter combat mode, ignoring Scene Safety.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Control")
	void EnableCombatForRole(FString RoleID);
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Callback for outer trigger: Activates the scene */
	UFUNCTION()
	void OnSceneActivationOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Callback for inner trigger: Broadcasts Event.PlayerDetected */
	UFUNCTION()
	void OnEventTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	/** Helper: Check if actor is a player */
	bool IsPlayerActor(AActor* Actor) const;
};
