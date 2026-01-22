#pragma once

#include "CoreMinimal.h"

/**
 * Global constant names for Mental State variables.
 * Use these instead of hardcoded strings like TEXT("Hunger").
 */
namespace MentalStateNames
{
    // Physiological
    static const FString Hunger = TEXT("Hunger");
    static const FString Fatigue = TEXT("Fatigue");

    // Safety
    static const FString Threat = TEXT("Perceived_Threat");

    // Social / Belonging
    static const FString Loneliness = TEXT("Loneliness");

    // Esteem
    static const FString Indignity = TEXT("Indignity");

    // Self-Actualization
    static const FString Boredom = TEXT("Boredom");
}
