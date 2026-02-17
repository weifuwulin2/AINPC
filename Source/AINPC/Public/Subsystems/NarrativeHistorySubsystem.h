
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Events/EventTypes.h"
#include "NarrativeHistorySubsystem.generated.h"

// FNarrativeEvent moved to Events/EventTypes.h for centralized management
// FNarrativeEvent 宸茬Щ鑷?Events/EventTypes.h 杩涜闆嗕腑绠＄悊

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNarrativeEventRecorded, const FNarrativeEvent&, NewEvent);

/**
 * The "Historian" of the world.
 * Tracks global events, deaths, relationships, faction states.
 * Acts as the central hub for "Paimon's Notebook".
 */
UCLASS()
class AINPC_API UNarrativeHistorySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- Core API ---

	/** Record a significant event in the world. (Unified with Semantic Event System) */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	void RecordEvent(FString Description, FGameplayTagContainer Tags);

	/** Legacy overload for FNames (Marked for deprecation but kept for compilation) */
	// UFUNCTION(BlueprintCallable, Category = "Narrative History", meta=(DeprecatedFunction, DeprecationMessage="Use GameplayTags version"))
	void RecordEvent(FString Description, TArray<FName> Tags);

	/** Helper to record a death event specifically */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	void RecordNPCDeath(AActor* Victim, AActor* Killer);

	// --- NEW: Player Actions ---

	/** Record a player action (e.g., "Player attacked Merchant", "Player opened chest") */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	void RecordPlayerAction(FString ActionDescription, TArray<FName> Tags);

	// --- NEW: Relationship Changes ---

	/** Record a relationship change between NPCs or NPC-Player */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	void RecordRelationshipChange(FName SourceActor, FName TargetActor, float OldValue, float NewValue);

	// --- NEW: Faction State ---

	/** Record a faction reputation change */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	void RecordFactionReputationChange(FName FactionID, float Delta, FString Reason);

	/** Update faction population count */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	void UpdateFactionPopulation(FName FactionID, int32 Delta);

	/** Get current faction population */
	UFUNCTION(BlueprintPure, Category = "Narrative History")
	int32 GetFactionPopulation(FName FactionID) const;

	// --- Context Generation ---

	/** Returns a summary of world history for LLM context. */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	FString GetWorldStateDescription(int32 MaxEvents = 5) const;

	/** Returns a structured world snapshot for LLM context (replaces GetWorldStateDescription for WorldDirector use). */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	FString GetStructuredWorldSnapshot(int32 MaxRecentEvents = 5) const;

	/** Returns a list of all living NPCs with brief profiles (Name, Faction, Profession, Emotion). */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	FString GetLivingNPCRoster() const;

	/** Returns recent plot threads (director-generated scenes) for deduplication. */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	FString GetRecentPlotThreads(int32 MaxThreads = 5) const;

	/** Returns the dead NPC list formatted for LLM injection. */
	UFUNCTION(BlueprintPure, Category = "Narrative History")
	FString GetDeadNPCList() const;

	/** Record a completed plot thread (scene outline) for deduplication tracking. */
	UFUNCTION(BlueprintCallable, Category = "Narrative History")
	void RecordPlotThread(const FString& PlotOutline, const FString& DramaticAction);

	// --- Events ---

	/** Broadcasts when any event is recorded. SquadSubsystem listens to this. */
	UPROPERTY(BlueprintAssignable, Category = "Narrative History")
	FOnNarrativeEventRecorded OnEventRecorded;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Narrative History")
	TArray<FNarrativeEvent> HistoryLog;

	UPROPERTY(VisibleAnywhere, Category = "Narrative History")
	TSet<FName> DeadVIPs;

	/** Faction Population: FactionID -> Current Count */
	UPROPERTY(VisibleAnywhere, Category = "Narrative History")
	TMap<FName, int32> FactionPopulation;

	/** Relationship Snapshot: "SourceActor_TargetActor" -> CurrentValue */
	UPROPERTY(VisibleAnywhere, Category = "Narrative History")
	TMap<FName, float> RelationshipSnapshot;

	/** Completed plot threads for deduplication (stores recent scene outlines) */
	struct FPlotThread
	{
		FString PlotOutline;
		FString DramaticAction;
		float Timestamp = 0.0f;
	};
	TArray<FPlotThread> CompletedPlotThreads;

	/** Max plot threads to keep for deduplication */
	static constexpr int32 MaxPlotThreadHistory = 20;
};

