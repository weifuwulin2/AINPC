// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogAINPC, Log, All);

/** Dedicated Log Categories for Phase 4 Debugging */
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCBrain, Log, All);      // Decision making (LOD, Directives)
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCMemory, Log, All);     // Memory system (Store, Decay, Retrieve)
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCSocial, Log, All);     // Social interactions (Gossip, Speak)
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCUtility, Log, All);    // Utility AI (Scoring, Actions)
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCNarrative, Log, All);  // Narrative Timeline (Squad, Scene, Events)
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCTargetSelection, Log, All);  // Target Selection (Combat, Social, etc.)
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCLLM, Log, All);             // LLM Communication (Roleplay, Functional, HTTP)
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCDirector, Log, All);       // WorldDirector (Tension, Beats, Scene Generation)
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCVillage, Log, All);        // Village systems (territory, registry, bootstrap)
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCBuilding, Log, All);       // Building systems (binding, placement, lifecycle)
DECLARE_LOG_CATEGORY_EXTERN(LogAINPCCombatStateTree, Log, All); // Combat StateTree (tasks, conditions, transitions)

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

// ===================================
// Specialized Logging Macros
// ===================================

/** Brain Log: High-level state changes, LOD switches, Directive updates. */
#define BRAIN_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCBrain, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Memory Log: Memory commitments, decay calculations, retrievals. */
#define MEMORY_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCMemory, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Social Log: Dialogue generation, gossip exchange, player interactions. */
#define SOCIAL_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCSocial, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Utility Log: Action scoring, consideration values, winning action selection. */
#define UTILITY_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCUtility, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Narrative Log: Timeline progression, scene activation, event triggers. */
#define NARRATIVE_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCNarrative, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Target Selection Log: Target selection process, scoring, caching. */
#define TARGET_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCTargetSelection, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** LLM Log: API requests, responses, prompt building, token usage. */
#define LLM_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCLLM, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Director Log: WorldDirector tension evaluation, dramatic beats, scene generation. */
#define DIRECTOR_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCDirector, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Combat StateTree Log: Task enter/exit, condition checks, runtime transition debug. */
#define COMBAT_ST_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCCombatStateTree, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Village Log: village membership, territory claims, social bootstrap. */
#define AINPC_VILLAGE_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCVillage, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Village Log Verbose helper. */
#define AINPC_VILLAGE_LOG_VERBOSE(Format, ...) \
    UE_LOG(LogAINPCVillage, Verbose, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Village Log Warning helper. */
#define AINPC_VILLAGE_LOG_WARNING(Format, ...) \
    UE_LOG(LogAINPCVillage, Warning, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Village Log Error helper. */
#define AINPC_VILLAGE_LOG_ERROR(Format, ...) \
    UE_LOG(LogAINPCVillage, Error, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Building Log: building to village binding and lifecycle notifications. */
#define AINPC_BUILDING_LOG(Verbosity, Format, ...) \
    UE_LOG(LogAINPCBuilding, Verbosity, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Building Log Verbose helper. */
#define AINPC_BUILDING_LOG_VERBOSE(Format, ...) \
    UE_LOG(LogAINPCBuilding, Verbose, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Building Log Warning helper. */
#define AINPC_BUILDING_LOG_WARNING(Format, ...) \
    UE_LOG(LogAINPCBuilding, Warning, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)

/** Building Log Error helper. */
#define AINPC_BUILDING_LOG_ERROR(Format, ...) \
    UE_LOG(LogAINPCBuilding, Error, TEXT("[%s] " Format), *FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT("::"))), ##__VA_ARGS__)
