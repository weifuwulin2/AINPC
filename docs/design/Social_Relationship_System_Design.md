# Social Relationship System Design

## Overview
This document outlines a "Stanford Town" style relationship system for AI NPCs. It adds a **Qualitative Layer** (Social Bonds) on top of the existing **Quantitative Layer** (Faction/Reputation).

## Architecture Layers

### Layer 1: Faction (Macro)
*   **Existing**: `FactionSubsystem` / `DT_Factions`.
*   **Role**: Provides the *Initial Default State* for strangers.

### Layer 2: Personal Reputation (Micro Numeric)
*   **Existing**: `FactionReputationComponent`.
*   **Role**: Tracks granular history of positive/negative interactions (-100 to +100).
*   **Mechanic**: Updates instantly via `ModifyReputation()`.

### Layer 3: Social Bonds (Qualitative / Semantic)
*   **NEW**: `SocialComponent`.
*   **Role**: Defines the *Nature* and *Depth* of the relationship.
*   **Evolution**: Driven by **Social Reflection** (LLM).
*   **Storage**: `TMap<FName, FSocialBond>` keyed by persistent NPC identity (`NPCDefinitionComponent::NameID`), not `AActor*`, for save/load support and NPC-to-NPC bonds.

## Data Structures

```cpp
UENUM(BlueprintType)
enum class ESocialBondType : uint8
{
    None,
    Acquaintance, // Met once
    Friend,       // Positive history
    CloseFriend,  // High trust
    BestFriend,   // Ride or die
    Lover,        // Romantic
    Family,       // Blood relation (Static)
    Rival,        // Competitor
    Enemy,        // Negative history
    Nemesis       // Sworn enemy
};

USTRUCT(BlueprintType)
struct FSocialBond
{
    GENERATED_BODY()

    UPROPERTY()
    ESocialBondType Type = ESocialBondType::None;

    // 0-10 Scale. How much mental space this relationship occupies.
    // Not a sentiment score (that's Reputation). A "Rival" and a "Friend" can both be high salience.
    UPROPERTY()
    int32 BondSalience = 0;

    // GUIDs referencing pinned MemoryItems that justify this bond.
    // These memories must have bPinned=true to survive normal decay.
    UPROPERTY()
    TArray<FGuid> FoundationalMemories;

    // Summary of the dynamic (e.g., "We bicker but have each other's backs")
    UPROPERTY()
    FString RelationshipSummary;

    // World time (seconds) of last interaction or reflection involving this bond.
    // Used for staleness decay during Dreaming.
    UPROPERTY()
    float LastInteractionTime = 0.0f;
};
```

### Memory Pinning

Foundational memories must survive `MemoryComponent` decay (half-life 120s, max 30 items). Add a `bPinned` flag to `FMemoryItem`:

```cpp
// In FMemoryItem (SocialTypes.h)
UPROPERTY()
bool bPinned = false; // If true, exempt from decay/cleanup. Set by SocialComponent when a memory becomes foundational.
```

`MemoryComponent::CleanupOldMemories()` must skip items where `bPinned == true`. Pinned memories do not count toward the 30-item cap.

## The Evolution Loop (How it Updates)

Relationships do not update *just* by math. They update through **Reflection**.

### 1. The Interaction (Event)
An actor performs an action (e.g., "Gives a Gift", "Attacks", "Completes Quest").
*   **System Action**: Records a **Memory** in `MemoryComponent`.
*   **System Action**: Adjusts **Reputation Score** (e.g., +10).
*   **Applies to**: Both Player-NPC and NPC-NPC interactions (any actor pair sensed by `SensoryComponent`).

### 2. The Trigger (Reflection Request)
A **Social Reflection** is triggered if:
*   Reputation Score crosses a Tier Threshold (e.g., crosses from Neutral to Friendly).
*   A **High-Impact Event** occurs (Magnitude > 0.8, e.g., "Saved Life").
*   Periodically during "Dreaming" (Daily review).

### 3. The Reflection (LLM Analysis)
The system constructs a prompt for the LLM:
> "Reflect on your relationship with [Player].
> Current Status: Acquaintance.
> Recent Memories:
> - [He gave me a sword] (Positive)
> - [We fought zombies together] (High intensity)
>
> Valid Bond Types: None, Acquaintance, Friend, CloseFriend, BestFriend, Lover, Family, Rival, Enemy, Nemesis.
> Question: Has your relationship changed? If so, what is the new Bond Type and Summary?"

### 4. The Update (State Change)
The LLM returns a JSON:
```json
{
    "NewBond": "Friend",
    "Salience": 3,
    "Summary": "I trust him because he armed me and fought beside me.",
    "FoundationalMemoryIDs": ["<GUID-OF-ZOMBIE-FIGHT>"]
}
```

**Schema Validation** (same pattern as `CognitionComponent` `FMentalState` parsing):
*   `NewBond` must match a valid `ESocialBondType` enum string. If invalid, discard the update.
*   `Salience` clamped to [0, 10].
*   `FoundationalMemoryIDs` validated against `MemoryComponent` — only existing GUIDs are accepted. Matching memories are pinned (`bPinned = true`).

The `SocialComponent` updates the `FSocialBond` struct and sets `LastInteractionTime`.

### 5. The Consequence (Behavior)
*   **Prompt Injection**: The `RelationshipSummary` is injected into `CognitionComponent` prompts alongside faction/personality context.
*   **Utility AI Integration**: Bond type can serve as a precondition or score modifier on `UtilityAIComponent` actions (e.g., `JoinParty` requires `Type >= Friend`, `Betray` requires `Type == Nemesis`).

## Bond Degradation

Bonds are not permanent. During the **Dreaming** cycle:

1.  **Staleness Check**: If `(CurrentWorldTime - LastInteractionTime) > StalenessThreshold` (configurable, default: 3 in-game days), reduce `BondSalience` by 1.
2.  **Salience Floor**: If `BondSalience` reaches 0, trigger a downgrade reflection:
    *   `Friend` → `Acquaintance` → `None`.
    *   `Rival` → `Acquaintance` → `None`.
    *   `Family` and `Nemesis` are exempt from automatic degradation (too significant to forget passively).
3.  **Memory Unpinning**: When a bond degrades to `None`, its `FoundationalMemories` are unpinned and subject to normal decay.

## NPC-to-NPC Bonds

The `SocialComponent` bond map uses persistent identity (`FName` from `NPCDefinitionComponent::NameID`), so NPC-NPC relationships work identically to Player-NPC. However, to control LLM cost:

*   **Reflection Budget**: Only the top N bonds (by salience) are reflected on per dreaming cycle. Default N = 3.
*   **Passive Bonds**: Low-salience NPC-NPC bonds skip LLM reflection and degrade purely by staleness rules.

## Integration Points

| Existing System | Integration |
|---|---|
| `MemoryComponent` | Add `bPinned` flag. `SocialComponent` pins/unpins foundational memories. |
| `CognitionComponent` | Inject `RelationshipSummary` into prompt context for each relevant actor. |
| `SensoryComponent` | No change. Already provides actor relationship tagging (`MY ALLY`/`MY ENEMY`). |
| `FactionReputationComponent` | Reputation threshold crossings trigger social reflection via delegate. |
| `UtilityAIComponent` | Bond type used as action precondition/score modifier in DataTable config. |
| `GoalComponent` / Dreaming | Dreaming cycle triggers staleness checks and scheduled reflections. |
| `UtilityAIController` | Owns and wires `SocialComponent` alongside existing components. |
