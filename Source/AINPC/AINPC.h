// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogAINPC, Log, All);

/**
 * Logging macros with automatic class name prefix.
 * Usage: AINPC_LOG(Warning, "My message with %s", *SomeString);
 * Output: [MyClassName] My message with SomeValue
 */
#define AINPC_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPC, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Verbose version - only shows in verbose mode */
#define AINPC_LOG_VERBOSE(Format, ...) \
    UE_LOG(LogAINPC, Verbose, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Warning version */
#define AINPC_LOG_WARNING(Format, ...) \
    UE_LOG(LogAINPC, Warning, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Error version */
#define AINPC_LOG_ERROR(Format, ...) \
    UE_LOG(LogAINPC, Error, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)