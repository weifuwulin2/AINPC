
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/NarrativeDirectorSubsystem.h" // For FNarrativeEvent
#include "NarrativeSquadSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSceneRoleDef
{
	GENERATED_BODY()

	// The NPC Template ID (e.g. "Orc_Slave") from DT_NPCDefinitions
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName NPCTemplateID;

	// Relative Spawn Location from Scene Origin
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform RelativeTransform;

	// Override Role Description (Optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RoleOverride;
};

USTRUCT(BlueprintType)
struct FScenePropDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> PropClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform RelativeTransform;
};

/** Defines a static scene layout (The Cast & The Script) */
USTRUCT(BlueprintType)
struct FNarrativeSceneDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PlotOutline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSceneRoleDef> Cast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FScenePropDef> Props;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> CompletionTags;
};

USTRUCT(BlueprintType)
struct FNarrativeSceneSquad
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 SquadID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlotOutline;

	// Actors assigned to this scene and their specific Role
	UPROPERTY(BlueprintReadOnly)
	TMap<AActor*, FString> MemberRoles;
	
	UPROPERTY()
	TArray<AActor*> SpawnedProps;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> CompletionTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsActive = false;
	
	UPROPERTY()
	class ANarrativeSceneAnchor* AssignedAnchor = nullptr;

	FNarrativeSceneSquad() : SquadID(-1) {}
};

/**
 * The "Stage Manager".
 * Manages groups of NPCs (Squads) performing a specific narrative scene.
 * Listens to NarrativeDirector for completion events.
 */
UCLASS()
class AINPC_API UNarrativeSquadSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- API ---

	/**
	 * Creates a new Scene Squad.
	 * @param PlotOutline - The "Context" for the whole group.
	 * @param CompletionTags - Events that will cause this scene to end (e.g., "RobberDead").
	 * @return SquadID
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	int32 CreateSceneSquad(FString PlotOutline, TArray<FName> CompletionTags);

	/**
	 * Assigns an NPC to a squad with a specific Role.
	 * Updates the NPC's CognitionComponent immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	void AssignMemberRole(int32 SquadID, AActor* NPC, FString RoleDescription);

	/** Returns a context string for the NPC: "Plot: [X]. Role: [Y]." */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	FString GetMemberContext(AActor* NPC) const;

	/** 
	 * Helper: Finds all Cognition-capable NPCs in radius and assigns them to the squad with the given role.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	void AssignRolesToArea(int32 SquadID, FVector Origin, float Radius, FString RoleDescription);

	/** Manually end a scene */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	void EndScene(int32 SquadID);

	/**
	 * Spawns a full scene from a Data Table template.
	 * Dynamically spawns NPCs and assigns them roles.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	int32 SpawnSceneFromTemplate(UDataTable* SceneTable, FName TemplateID, FTransform Origin, UDataTable* NPCTable, bool bAutoActivate = true);

	/**
	 * Finds an available anchor for the template and starts the scene.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	int32 StartSceneGlobal(UDataTable* SceneTable, FName TemplateID, UDataTable* NPCTable);

	/** Spawns a scene AT a specific anchor. Usually called on BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	int32 SpawnSceneAtAnchor(class ANarrativeSceneAnchor* Anchor, UDataTable* SceneTable, FName TemplateID, UDataTable* NPCTable, bool bAutoActivate = false);

	/** Activates a previously spawned (but inactive) scene squad. */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	void ActivateScene(int32 SquadID);

	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	void RegisterAnchor(class ANarrativeSceneAnchor* Anchor);

	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	void UnregisterAnchor(class ANarrativeSceneAnchor* Anchor);

protected:
	UPROPERTY()
	TArray<class ANarrativeSceneAnchor*> RegisteredAnchors;

	// --- Event Handling ---
	
	UFUNCTION()
	void OnNarrativeEventRecorded(const FNarrativeEvent& Event);

protected:

	UPROPERTY(VisibleAnywhere, Category = "Narrative Squad")
	TMap<int32, FNarrativeSceneSquad> ActiveSquads;

	UPROPERTY()
	TMap<AActor*, int32> ActorSquadMap;

	int32 NextSquadID = 0;
};
