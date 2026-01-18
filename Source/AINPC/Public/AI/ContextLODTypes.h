#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ContextLODTypes.generated.h"

UENUM(BlueprintType)
enum class EContextLOD : uint8
{
	Critical UMETA(DisplayName = "LOD 0: Critical (Survival)"),
	Standard UMETA(DisplayName = "LOD 1: Standard (Routine)"),
	Deep     UMETA(DisplayName = "LOD 2: Deep (Social)")
};
