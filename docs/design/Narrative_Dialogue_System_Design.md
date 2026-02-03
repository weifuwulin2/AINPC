# Narrative Dialogue System Design

## Overview
This document outlines a system for **Deterministic Narrative Dialogue**, replacing the random "Ambient Dialogue" system for key plot moments. It introduces two mutually exclusive modes: **Mass Reactions** (Crowd Ambience) and **Dialogue Beats** (Scripted Sequences).

*Status: Active Design (2026-02-03)*

## Core Concepts

The system operates on **Mutual Exclusion** based on the `MassReactionPercentage` field in a Timeline Node.

### 1. Mass Reaction Mode (Crowd Ambience)
Used when a timeline node represents a chaotic or group event (e.g., "The crowd gasps", "Everyone cheers").

*   **Trigger Condition**: `MassReactionPercentage > 0.0`
*   **Logic**:
    1.  Calculate target number of speakers: `Count = Ceil(SquadSize * MassReactionPercentage)`.
    2.  **Minimum 1**: If the calculated count is less than 1 (e.g., 10% of 5 people = 0.5), force it to **1**.
    3.  Select `Count` actors randomly from the squad.
    4.  Force them all to speak **simultaneously** (0.0s delay).
    5.  **Context**: Uses the `PlotUpdate` string as the prompt context.
*   **Note**: In this mode, any `DialogueSequence` defined in the node is **IGNORED**.

### 2. Dialogue Sequence Mode (Scripted Conversation)
Used when a timeline node represents a specific, structured interaction between characters.

*   **Trigger Condition**: `MassReactionPercentage <= 0.0` and `DialogueSequence` is not empty.
*   **Logic**:
    *   System starts a **Sequence Coordinator**.
    *   Executes beats one by one:
        1.  **Beat 1**: Actor A speaks. Wait `Beat1.Duration` (pacing delay).
        2.  **Beat 2**: Actor B speaks. Wait `Beat2.Duration`.
        3.  **Done**: Sequence ends.

## Proposed Architecture

### Data Structures (`NarrativeSquadSubsystem.h`)

```cpp
USTRUCT(BlueprintType)
struct FNarrativeDialogueBeat
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere)
    FName SpeakerRole; // e.g. "Leader", "Victim"
    
    UPROPERTY(EditAnywhere)
    FString Prompt;    // e.g. "Scream in pain"
    
    UPROPERTY(EditAnywhere)
    float Duration;    // Time to wait AFTER this beat before next one starts
};

USTRUCT(BlueprintType)
struct FNarrativeTimelineEntry
{
    // ... existing fields ...
    
    /** 
     * If > 0, triggers Mass Reaction Mode (Simultaneous). 
     * If <= 0, triggers Sequence Mode (Sequential).
     */
    UPROPERTY(EditAnywhere)
    float MassReactionPercentage = 0.0f;

    /** Ordered list of beats. Ignored if MassReactionPercentage > 0. */
    UPROPERTY(EditAnywhere)
    TArray<FNarrativeDialogueBeat> DialogueSequence;
};
```

### Execution Logic (`NarrativeSquadSubsystem.cpp`)

```cpp
void TriggerTimelineNode(int32 SquadID, int32 NodeIndex)
{
    // ... get node ...
    
    // BRANCH A: MASS REACTION
    if (Node.MassReactionPercentage > 0.f)
    {
        int32 TotalMembers = Squad.SquadMembers.Num();
        int32 TargetCount = FMath::CeilToInt(TotalMembers * Node.MassReactionPercentage);
        TargetCount = FMath::Max(1, TargetCount); // Ensure at least 1 person speaks
        
        // Randomly select TargetCount members
        // Force them to speak immediately
        return; // DONE
    }
    
    // BRANCH B: SEQUENCE
    if (Node.DialogueSequence.Num() > 0)
    {
        // Store sequence in Squad state
        Squad.PendingDialogueSequence = Node.DialogueSequence;
        
        // Start Coordinator
        ProcessNextDialogueBeat(SquadID);
    }
}
```

## Future Implementation Steps
1.  Implement structs in Header.
2.  Implement `ForceDialogueBeat` (Integration with `CognitionComponent`).
3.  Implement `ProcessNextDialogueBeat` (Timer logic).
4.  Update `TriggerTimelineNode` to wire it all together.
