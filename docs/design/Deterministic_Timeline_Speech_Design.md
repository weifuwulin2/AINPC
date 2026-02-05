# Deterministic Timeline Speech Design

*Status: Approved for Implementation (2026-02-03)*
*Replaces: `Narrative_Dialogue_System_Design.md` (Gemini draft)*

## Overview

Replace the random Ambient Dialogue system with deterministic, Timeline-driven speech. Each Timeline node can optionally force a specific NPC to speak or trigger a mass crowd reaction.

## Rationale

- **Timeline IS the sequence** — no need for a nested `DialogueSequence` array inside timeline nodes. The existing `FNarrativeTimelineEntry` chain with `TimeOffset` + `Trigger` already provides ordered, timed execution.
- **Ambient Dialogue causes "wrong person at wrong time"** — random timer + random speaker selection conflicts with deterministic narrative moments.
- **`bForceImmediate` guarantees delivery** — bypasses `ProcessStimulus` rate limiting so the designated speaker always talks.

## Architecture

### Timeline Node Behavior Matrix

| Field Combination | Behavior |
|---|---|
| All empty | Default: update PlotOutline + Directive only (silent) |
| `ForcedSpeakerRole` + `ForcedSpeakerPrompt` | Specific NPC speaks with given prompt context |
| `MassReactionPercentage > 0` | X% of squad speaks simultaneously using `PlotUpdate` as context |

### Data Structure Changes

**`FNarrativeTimelineEntry`** — New fields (replacing `BarkID`):

```cpp
// Who speaks (role name from MemberRoles, e.g., "Leader", "Victim")
UPROPERTY(EditAnywhere)
FName ForcedSpeakerRole;

// Prompt context for forced speech (e.g., "Rally your allies for the final battle")
UPROPERTY(EditAnywhere, meta=(MultiLine=true))
FString ForcedSpeakerPrompt;

// If > 0, triggers mass reaction: this fraction of squad speaks simultaneously
UPROPERTY(EditAnywhere)
float MassReactionPercentage = 0.0f;
```

### Execution Flow

```
TriggerTimelineNode(SquadID, NodeIndex)
  |
  +-- Update PlotOutline for all members        (existing)
  +-- Apply DirectiveOverride                    (existing)
  |
  +-- IF ForcedSpeakerRole is set:
  |     Find actor by role -> ProcessStimulus(Prompt, bForceImmediate=true)
  |
  +-- ELSE IF MassReactionPercentage > 0:
  |     Calculate count = Ceil(SquadSize * Percentage)
  |     Randomly select `count` members
  |     Each -> ProcessStimulus(PlotUpdate, bForceImmediate=true)
  |
  +-- ELSE: Silent node (context update only)
```

### Rate Limit Bypass

`CognitionComponent::ProcessStimulus` gains a `bool bForceImmediate = false` parameter:
- `true`: Skip rate limit cooldown entirely (used by Timeline forced speech)
- `false`: Normal behavior (default, all existing callers unaffected)

### Removed: Ambient Dialogue System

The following are removed entirely:
- `StartAmbientDialogue()`, `TriggerAmbientDialogue()`, `RequestAmbientDialogue()`
- Ambient dialogue timer handles and configuration fields
- Random speaker selection logic

## Example Timeline Configuration

```
Node 0: TimeOffset=0,  PlotUpdate="Slaves are mining under guard. Tension is high."
         ForcedSpeakerRole="Leader", ForcedSpeakerPrompt="Whisper to nearby allies about the escape plan"

Node 1: TimeOffset=5,  Trigger=Event.PlayerDetected
         PlotUpdate="The hero has arrived! The slaves see hope."
         MassReactionPercentage=0.5  (50% of slaves react)

Node 2: TimeOffset=0,  Trigger=Event.Death (Guard dies)
         PlotUpdate="The guard is dead! Freedom is within reach!"
         ForcedSpeakerRole="Leader", ForcedSpeakerPrompt="Shout a battle cry to inspire the uprising"

Node 3: TimeOffset=3,  DirectiveOverride=Directive.Combat
         MassReactionPercentage=0.8  (80% join the fight with war cries)
```

## File Changes

| File | Changes |
|---|---|
| `CognitionComponent.h/cpp` | Add `bForceImmediate` param to `ProcessStimulus` |
| `NarrativeSquadSubsystem.h` | Replace `BarkID` with 3 new fields, remove ambient dialogue config, add `FindActorByRole` |
| `NarrativeSquadSubsystem.cpp` | Implement forced speech + mass reaction, remove ambient dialogue system (~200 lines) |
