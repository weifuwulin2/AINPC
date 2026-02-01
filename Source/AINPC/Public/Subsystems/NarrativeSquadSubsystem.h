#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/NarrativeDirectorSubsystem.h" // For FNarrativeEvent
#include "GameplayTagContainer.h" // For FGameplayTag
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

USTRUCT(BlueprintType)
struct FNarrativeEventMatcher
{
	GENERATED_BODY()

	/** The event type to listen for (e.g. Event.Death) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories="Event"))
	FGameplayTag Tag;

	/** Optional parameter to filter by (e.g. "Tyrant Lord", "Guard") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Payload;

	bool IsValid() const { return Tag.IsValid(); }
    
    // Serialization for maps/sets if needed
	bool operator==(const FNarrativeEventMatcher& Other) const
	{
		return Tag == Other.Tag && Payload == Other.Payload;
	}
};

/**
 * Defines a single node in a Narrative Scene's Timeline.
 * Allows plot and directives to evolve over time.
 */
USTRUCT(BlueprintType)
struct FNarrativeTimelineEntry
{
	GENERATED_BODY()

	/** Time offset from scene start (seconds). Acts as minimum wait time if TriggerCondition is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline")
	float TimeOffset = 0.0f;

	/** Optional event condition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline")
	FNarrativeEventMatcher Trigger;

	/** New plot outline to inject into NPC context (LLM). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline", meta=(MultiLine=true))
	FString PlotUpdate;

	/** Optional directive override for the squad (e.g., Directive.Combat). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline", meta=(Categories="Directive"))
	FGameplayTag DirectiveOverride;

	/** Optional bark ID for squad leader to speak. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline")
	FName BarkID;

	/** If true, NPCs in this scene will NOT observe other NPCs' action changes during this timeline node.
	 * 如果为 true，场景中的 NPC 在此 timeline 节点期间将不会观察其他 NPC 的动作变化。
	 * Useful for preventing distraction during important plot moments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline")
	bool bSuppressActionObservation = false;
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

	/** Conditions to end the scene (OR logic: if any matches, scene ends) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FNarrativeEventMatcher> CompletionConditions;

	/** Timeline of plot evolution nodes. Enables dynamic scene progression. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline")
	TArray<FNarrativeTimelineEntry> Timeline;

	/** If true, spawned props will NOT be destroyed when the scene ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bKeepPropsOnEnd = false;

	// 场景结束后发送给所有成员的刺激信息（例如："你们自由了！"）
	/** Message sent to NPC Cognition when scene ends (e.g. "The Tyrant is dead! We are free!"). Overrides default routine method. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FString PostSceneStimulus;

	// 场景结束后强制转职的目标职业ID（例如："Slave" -> "Citizen"）
	// Target ProfessionID to switch to after scene ends (e.g. "Slave" -> "Citizen")
	// If set to None, profession remains unchanged.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FName PostSceneProfessionID;

	// 随机转职池：如果有内容，将覆盖 PostSceneProfessionID，从中随机选择一个
	// Random Profession Pool: If not empty, overrides PostSceneProfessionID with a random pick
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	TArray<FName> PostSceneProfessionPool;
};

// 运行时的 Squad 实例状态
// Runtime Squad Instance State
USTRUCT(BlueprintType)
struct FNarrativeSceneSquad
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 SquadID = -1;

	UPROPERTY(BlueprintReadOnly)
	FName SquadName;

	UPROPERTY(BlueprintReadOnly)
	TArray<AActor*> SquadMembers;

	UPROPERTY(BlueprintReadOnly)
	TMap<AActor*, FName> MemberRoles; // Actor -> RoleID

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CurrentPhase;

	UPROPERTY()
	TArray<AActor*> SpawnedProps;

	UPROPERTY()
	bool bKeepPropsOnEnd = false; // Runtime copy

	UPROPERTY()
	FString PostSceneStimulus; // Runtime copy

	UPROPERTY()
	FName PostSceneProfessionID; // Runtime copy

	UPROPERTY()
	TArray<FName> PostSceneProfessionPool; // Runtime copy
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlotOutline;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FNarrativeEventMatcher> CompletionConditions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsActive = false;
	
	UPROPERTY()
	class ANarrativeSceneAnchor* AssignedAnchor = nullptr;

	// --- Ambient Dialogue Configuration ---
	
	/** Enable ambient dialogue for this scene */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ambient Dialogue")
	bool bEnableAmbientDialogue = true;

	/** Minimum interval between ambient dialogue triggers (seconds) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ambient Dialogue")
	float AmbientDialogueIntervalMin = 5.0f;  // Changed from 30.0f for testing

	/** Maximum interval between ambient dialogue triggers (seconds) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ambient Dialogue")
	float AmbientDialogueIntervalMax = 10.0f;  // Changed from 60.0f for testing

	/** Number of NPCs that speak per trigger (1-2 recommended) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ambient Dialogue")
	int32 AmbientSpeakersPerTrigger = 2;

	/** Player must be within this distance for ambient dialogue to trigger */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ambient Dialogue")
	float PlayerActivationRadius = 2000.0f;

	/** Runtime timer handle for ambient dialogue */
	FTimerHandle AmbientDialogueTimer;

	// --- Timeline System ---

	/** Copy of the scene's timeline definition (from DT_NarrativeScenes) */
	UPROPERTY()
	TArray<FNarrativeTimelineEntry> SceneTimeline;

	/** Current timeline node index */
	UPROPERTY()
	int32 CurrentTimelineIndex = 0;

	/** Accumulated time since scene activation (seconds) */
	UPROPERTY()
	float AccumulatedSceneTime = 0.0f;

	/** Runtime flag: if true, members should NOT observe action changes from other NPCs.
	 * 运行时标志：如果为 true，成员不应观察其他 NPC 的动作变化。
	 * DEFAULT: true (suppress by default, let Timeline enable observation when needed).
	 * 默认：true（默认禁止，让 Timeline 在需要时启用观察）
	 * Set by active timeline node's bSuppressActionObservation. */
	UPROPERTY()
	bool bCurrentlySuppressingActionObservation = true;

	/** Set of timeline nodes waiting for event triggers (index -> tag) */
	UPROPERTY()
	TMap<int32, FGameplayTag> PendingEventTriggers;

	/** Timer handle for timeline updates */
	FTimerHandle TimelineTickTimer;

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
	int32 CreateSceneSquad(FString PlotOutline, TArray<FNarrativeEventMatcher> CompletionConditions);

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

	// --- Ambient Dialogue API ---

	/** 
	 * Manually configure ambient dialogue settings for a scene.
	 * Call before activating the scene.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	void ConfigureAmbientDialogue(int32 SquadID, bool bEnabled, float MinInterval, float MaxInterval, int32 SpeakersPerTrigger, float ActivationRadius);

	/** Manually trigger ambient dialogue for testing */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	void TriggerAmbientDialogueNow(int32 SquadID);

	/** 
	 * Applies a GameplayTag to all squad members with a specific Role (e.g., "Slave", "Guard").
	 * Useful for triggering state changes like combat or behavior overrides.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	void ApplyTagToRole(int32 SquadID, FString RoleID, FGameplayTag Tag);

	/** Returns all members of the squad the ContextActor belongs to. */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	bool GetSquadMembers(const AActor* ContextActor, TArray<AActor*>& OutMembers) const;

	/**
	 * Check if action observation should be suppressed for a given actor.
	 * Returns true if the actor is in a scene that currently has bSuppressActionObservation enabled.
	 * 检查是否应该禁止观察指定 actor 的动作。
	 * 如果 actor 当前在一个启用了 bSuppressActionObservation 的场景中，则返回 true。
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative Squad")
	bool ShouldSuppressActionObservation(const AActor* ObservedActor) const;

protected:
	UPROPERTY()
	TArray<class ANarrativeSceneAnchor*> RegisteredAnchors;

	// --- Event Handling ---
	
	UFUNCTION()
	void OnNarrativeEventRecorded(const FNarrativeEvent& Event);

	// --- Ambient Dialogue Internal ---

	/** Start the ambient dialogue timer for a scene */
	void StartAmbientDialogue(int32 SquadID);

	/** Trigger ambient dialogue (timer callback) */
	void TriggerAmbientDialogue(int32 SquadID);

	/** Request a specific NPC to generate ambient dialogue */
	void RequestAmbientDialogue(AActor* Speaker, const FNarrativeSceneSquad* Squad);

	/** Check if player is near the scene anchor */
	bool IsPlayerNearScene(const FNarrativeSceneSquad* Squad) const;

	// --- Timeline System Internal ---

	/** Update timeline progression for a scene (called every second) */
	void TickTimeline(int32 SquadID);

	/** Execute a timeline node (update plot/directive) */
	void TriggerTimelineNode(int32 SquadID, int32 NodeIndex);

	/** 
	 * Checks if a timeline trigger condition is ALREADY met by the current world state.
	 * Returns true if the condition is met (e.g., Player is already inside the trigger zone).
	 */
	bool CheckStateCondition(int32 SquadID, int32 NodeIndex, const FNarrativeEventMatcher& Matcher);

	/** Clean up destroyed/invalid actors from all squads */
	void CleanupInvalidActors();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Narrative Squad")
	TMap<int32, FNarrativeSceneSquad> ActiveSquads;

	UPROPERTY()
	TMap<AActor*, int32> ActorSquadMap;

	int32 NextSquadID = 0;
};
