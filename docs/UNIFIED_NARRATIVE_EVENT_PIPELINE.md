# Unified Narrative Event Pipeline (Proposal)

## 1. Objective
Refactor the current narrative event system from a "Tag-string parsing" model (specifically optimized for Death events) to a fully **Context-Aware Event Pipeline**. 
This will allow **ANY** gameplay event (Damage, Perception, State Change, Interaction) to drive narrative logic based on the actor's Role, Faction, or Identity, configured entirely via DataTables.

## 2. The Problem (Current State)
*   **Death Logic is Special**: Currently, we handle matching by checking if a tag starts with `"Death_"`, extracting the name string, finding the actor, and looking up their role. This works but is fragile and specific only to death.
*   **Other Events are Dumb**: Events like `Event.PlayerDetected` are currently generic. The system doesn't easily know *who* detected the player (was it the Guard Captain or a lowly Slave?).
*   **Hack-y Data Flow**: We pass context (like "Who died") via string manipulation in Tags array, rather than as structured data.

## 3. The Solution (Future State)
We will introduce a standardized pipeline where **Events carry Context**, and the **Narrative System matches Context against Config**.

### A. Architectual Changes

#### 1. Enhanced Event Structure (`FNarrativeEvent`)
Instead of just `Description` and `Tags`, the event struct becomes strongly typed regarding actors.
```cpp
struct FNarrativeEvent
{
    // The main semantic tag (e.g., Event.Combat.Damage, Event.Perception.Seen)
    FGameplayTag EventType;

    // The primary actor involved (The Victim, The Observer, The Speaker)
    FName SubjectName;

    // The secondary actor involved (The Attacker, The Target, The Listener)
    FName TargetName;

    // Human-readable log
    FString Description;
    
    // (Optional) Additional Tags for fallback
    FGameplayTagContainer AdditionalTags;
};
```

#### 2. Smart Recording Interface (`NarrativeDirector`)
A new API that automatically extracts context from Actors so other systems don't have to manually build strings.
```cpp
// In NarrativeDirectorSubsystem
void RecordSmartEvent(AActor* Subject, FGameplayTag EventType, AActor* Target = nullptr)
{
    // 1. Get Subject's Name (for lookup)
    // 2. Log description automatically based on EventType:
    //    - If Damage: "{Subject} took damage from {Target}"
    //    - If See: "{Subject} saw {Target}"
    // 3. Broadcast Event
}
```

#### 3. Generic Matching Logic (`NarrativeSquadSubsystem`)
The `OnNarrativeEventRecorded` function becomes a generic loop that works for EVERYTHING:
```cpp
void OnNarrativeEventRecorded(const FNarrativeEvent& Event)
{
    // 1. Find the Squad that owns 'Event.SubjectName'
    // 2. Get that Subject's Role (e.g., "Guard Captain")
    // 3. Check Scene Config:
    //    "If Event == Event.Combat.Damage AND Payload == 'Guard Captain'"
    //    -> Trigger Next Plot Point!
}
```

### B. Implementation Roadmap

#### Phase 1: Struct & API Update
1.  Modify `FNarrativeEvent` in `NarrativeDirectorSubsystem.h`.
2.  Implement `RecordSmartEvent` in `NarrativeDirectorSubsystem.cpp`.
3.  Update existing `RecordNPCDeath` to use this new pipeline internally (for backward compatibility).

#### Phase 2: Logic Generalization
1.  Refactor `NarrativeSquadSubsystem::OnNarrativeEventRecorded`.
2.  Remove specific `StartsWidth("Death_")` logic.
3.  Replace with generic `SubjectName` lookup -> Role extraction -> `FNarrativeEventMatcher` check.

#### Phase 3: System Integation (Instrumentation)
1.  **Combat**: Update `TakeDamage` to call `RecordSmartEvent(this, Event_Combat_Damage, Instigator)`.
2.  **Perception**: Update `SensoryComponent` or `CognitionComponent` to call `RecordSmartEvent(this, Event_PlayerDetected, Target)`.
3.  **State**: Update `FactionReputation` to call `RecordSmartEvent(this, Event_State_Hostile, Enemy)`.

## 4. Product Value
*   **Unlimited Narrative Flexibility**: Designers can trigger specific plot lines if a *specific role* gets hit, sees the player, or flees.
*   **Cleaner Code**: Removes string parsing hacks.
*   **Consistency**: Death is no longer a special edge case; it's just `Event.Life.Death`.

---
*Created: 2026-01-31*
