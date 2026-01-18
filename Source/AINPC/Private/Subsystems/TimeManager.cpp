#include "Subsystems/TimeManager.h"
#include "Engine/World.h"

void UTimeManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentHour = StartHour;
	LastIntegerHour = FMath::FloorToInt(CurrentHour);
}

void UTimeManager::Deinitialize()
{
	Super::Deinitialize();
}

void UTimeManager::AdvanceTime(float DeltaSeconds)
{
	if (RealSecondsPerGameHour <= 0.0f) return;

	float GameHoursDelta = DeltaSeconds / RealSecondsPerGameHour;
	CurrentHour += GameHoursDelta;

	// Loop day
	if (CurrentHour >= 24.0f)
	{
		CurrentHour -= 24.0f;
		LastIntegerHour = -1; // Reset to allow 0 hour trigger
	}

	// Check for hour change
	int32 CurrentIntegerHour = FMath::FloorToInt(CurrentHour);
	if (CurrentIntegerHour != LastIntegerHour)
	{
		LastIntegerHour = CurrentIntegerHour;
		OnHourChanged.Broadcast((float)CurrentIntegerHour);
		
		// Optional: Log time change
		// UE_LOG(LogTemp, Log, TEXT("[TimeManager] 🕰️ Hour Changed: %d:00"), CurrentIntegerHour);
	}
}
