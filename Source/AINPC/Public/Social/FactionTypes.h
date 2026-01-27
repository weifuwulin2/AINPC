
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FactionTypes.generated.h"

/**
 * Defines a Faction and its default relationships.
 * Data-Driven via DT_Factions.
 */
USTRUCT(BlueprintType)
struct AINPC_API FFactionDef : public FTableRowBase
{
	GENERATED_BODY()

public:
	// Display Name (e.g., "The Royal Guard")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FString DisplayName;

	// Description for UI/Tooltips
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FString Description;

	// Default Attitude towards other Factions (0-100)
	// Key: Other Faction ID (Row Name)
	// Value: Attitude (0=Hostile, 50=Neutral, 100=Friendly)
	// Missing keys imply Neutral (50).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	TMap<FName, float> DefaultReputations;

	// Reputations below this are considered Hostile (Enemies)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	float HostilityThreshold = 25.0f;

	// Reputations above this are considered Friendly (Allies)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	float FriendlinessThreshold = 75.0f;

	FFactionDef()
	{
		DisplayName = "New Faction";
		HostilityThreshold = 25.0f;
		FriendlinessThreshold = 75.0f;
	}
};
