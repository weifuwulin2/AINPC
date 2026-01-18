// Copyright Epic Games, Inc. All Rights Reserved.

#include "AINPCGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/TimeManager.h"

AAINPCGameMode::AAINPCGameMode()
{
	// Enable ticking
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AAINPCGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Advance time using the TimeManager subsystem
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTimeManager* TimeManager = GI->GetSubsystem<UTimeManager>())
		{
			TimeManager->AdvanceTime(DeltaSeconds);
		}
	}
}
