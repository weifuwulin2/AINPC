
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PlayerSquadSubsystem.generated.h"

/**
 * Manages the Player's Party (Player + Companions).
 * Tracks collective health and context.
 */
UCLASS()
class AINPC_API UPlayerSquadSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- API ---

	UFUNCTION(BlueprintCallable, Category = "Player Squad")
	void RegisterPartyMember(AActor* Member);

	UFUNCTION(BlueprintCallable, Category = "Player Squad")
	void UnregisterPartyMember(AActor* Member);

	UFUNCTION(BlueprintPure, Category = "Player Squad")
	TArray<AActor*> GetPartyMembers() const { return PartyMembers; }

	/** Returns a summary of the party's status (e.g. "Player is Critical. Companion is Healthy.") */
	UFUNCTION(BlueprintCallable, Category = "Player Squad")
	FString GetPartyContextDescription() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Player Squad")
	TArray<AActor*> PartyMembers;
};
