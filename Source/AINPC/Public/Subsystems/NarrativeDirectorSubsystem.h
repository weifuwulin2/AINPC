
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NarrativeDirectorSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FNarrativeEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> Tags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Timestamp;

	FNarrativeEvent() : Timestamp(0.f) {}
	FNarrativeEvent(FString InDesc, float InTime) : Description(InDesc), Timestamp(InTime) {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNarrativeEventRecorded, const FNarrativeEvent&, NewEvent);

/**
 * The "Historian" of the world.
 * Tracks global events, deaths, relationships, faction states.
 * Acts as the central hub for "Paimon's Notebook".
 */
UCLASS()
class AINPC_API UNarrativeDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- Core API ---

	/** Record a significant event in the world. */
	UFUNCTION(BlueprintCallable, Category = "Narrative Director")
	void RecordEvent(FString Description, TArray<FName> Tags);

	/** Helper to record a death event specifically */
	UFUNCTION(BlueprintCallable, Category = "Narrative Director")
	void RecordNPCDeath(AActor* Victim, AActor* Killer);

	// --- NEW: Player Actions ---

	/** Record a player action (e.g., "Player attacked Merchant", "Player opened chest") */
	UFUNCTION(BlueprintCallable, Category = "Narrative Director")
	void RecordPlayerAction(FString ActionDescription, TArray<FName> Tags);

	// --- NEW: Relationship Changes ---

	/** Record a relationship change between NPCs or NPC-Player */
	UFUNCTION(BlueprintCallable, Category = "Narrative Director")
	void RecordRelationshipChange(FName SourceActor, FName TargetActor, float OldValue, float NewValue);

	// --- NEW: Faction State ---

	/** Record a faction reputation change */
	UFUNCTION(BlueprintCallable, Category = "Narrative Director")
	void RecordFactionReputationChange(FName FactionID, float Delta, FString Reason);

	/** Update faction population count */
	UFUNCTION(BlueprintCallable, Category = "Narrative Director")
	void UpdateFactionPopulation(FName FactionID, int32 Delta);

	/** Get current faction population */
	UFUNCTION(BlueprintPure, Category = "Narrative Director")
	int32 GetFactionPopulation(FName FactionID) const;

	// --- Context Generation ---

	/** Returns a summary of world history for LLM context. */
	UFUNCTION(BlueprintCallable, Category = "Narrative Director")
	FString GetWorldStateDescription(int32 MaxEvents = 5) const;

	// --- Events ---

	/** Broadcasts when any event is recorded. SquadSubsystem listens to this. */
	UPROPERTY(BlueprintAssignable, Category = "Narrative Director")
	FOnNarrativeEventRecorded OnEventRecorded;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Narrative Director")
	TArray<FNarrativeEvent> HistoryLog;

	UPROPERTY(VisibleAnywhere, Category = "Narrative Director")
	TSet<FName> DeadVIPs;

	/** Faction Population: FactionID -> Current Count */
	UPROPERTY(VisibleAnywhere, Category = "Narrative Director")
	TMap<FName, int32> FactionPopulation;

	/** Relationship Snapshot: "SourceActor_TargetActor" -> CurrentValue */
	UPROPERTY(VisibleAnywhere, Category = "Narrative Director")
	TMap<FName, float> RelationshipSnapshot;
};
