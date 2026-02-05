#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimeManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHourChanged, float, NewHour);

/**
 * Manages the in-game Day/Night cycle and broadcasts time events.
 */
UCLASS()
class AINPC_API UTimeManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- Configuration ---

	// How many real-world seconds equal one game hour?
	// Default: 10s = 1 hour (4 minutes = 1 day) — showcase speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float RealSecondsPerGameHour = 10.0f;

	// Starting hour of the day
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float StartHour = 8.0f;

	UPROPERTY(BlueprintAssignable, Category = "Time")
	FOnHourChanged OnHourChanged;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Time")
	float GetCurrentHour() const { return CurrentHour; }

	UFUNCTION(BlueprintPure, Category = "Time")
	float GetTimeOfDay01() const { return CurrentHour / 24.0f; }

	// Advance time manually (called by GameMode or Level Blueprint usually, or Self-Ticked)
	UFUNCTION(BlueprintCallable, Category = "Time")
	void AdvanceTime(float DeltaSeconds);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Time")
	float CurrentHour = 0.0f;

	// Helper to detect hour change
	int32 LastIntegerHour = -1;
};
