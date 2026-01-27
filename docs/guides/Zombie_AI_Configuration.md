# Zombie & Monster AI Configuration Guide

## Overview
This guide documents the configuration steps required to set up aggressive, "mindless" enemies (like Zombies) within the Utility AI system. Unlike standard NPCs that use complex reasoning and social considerations, Monsters rely on a streamlined logic path (referred to as the "Amygdala Hijack") to react instantly to threats.

---

## 1. Actor & Controller Setup

### Pawn Class
*   **Class**: Should inherit from `ACombatEnemy` (or `ALumiosCombatCharacter`).
*   **Essential Components**:
    *   `MonsterComponent`: The presence of this component is the primary trigger for "Monster" behavior.
    *   `NPCDefinitionComponent`: Defines the generic data (Faction, ID).
    *   `StateTreeAIComponent`: Executes the decision logic.
    *   `SensoryComponent`: Handles sight and hearing.
    *   `CognitionComponent`: Handles state tracking (Reasoning is disabled for Monsters).

### AI Controller
*   **Class**: MUST be `AUtilityAIController` (or a subclass that calls `Super::OnPossess`).
*   **Why?**: The `MonsterComponent` initialization logic resides in `AUtilityAIController::OnPossess`. If you use a basic `ACombatAIController` or `AAIController`, the Monster logic will **not** trigger, and the Zombie will essentially be lobotomized.

---

## 2. Component Configuration Details

### MonsterComponent
*   Add this component to the Pawn.
*   **Effect**:
    *   Disables `bEnableReasoning` on the `CognitionComponent`.
    *   Forces specific Personality settings suitable for aggression.
    *   Bypasses complex social checks.

### NPCDefinitionComponent
*   **FactionID**: Set to `Monsters` (Case-sensitive, Plural).
    *   *Note*: The code maps `Monsters` to `EFactionType::Monster`.
*   **PersonalityID**: Can be set to specific monster personalities (e.g., `Zombie_Standard`), though `MonsterComponent` logic may override this in code.

### SensoryComponent
*   **Config**: Ensure `AIPerception` options are set (Sight range, etc.).
*   **Hostility**:
    *   The `SensoryComponent` uses `FactionReputationComponent` (on the Controller) or `NPCDefinitionComponent` (on the Pawn) to determine hostility.
    *   Ensure the `Monsters` faction is configured as Hostile to `Players`, `Villagers`, etc., in the centralized Faction System/Data.

---

## 3. The "Amygdala Hijack" (Threat Logic)
Since Zombies have `bEnableReasoning = false`, they do not process the standard `CognitionComponent::ProcessStimulus` loop for creating memories or complex judgments.

**How they detect enemies:**
1.  **Perception**: `SensoryComponent` detects an Actor.
2.  **Hostility Check**: Checks Faction (`Monsters` vs `Players` -> Hostile).
3.  **Direct Override**:
    *   Inside `USensoryComponent::HandleTargetPerceived`, if the target is **Hostile**:
    *   The code **directly** sets the `Perceived_Threat` blackboard key/Utility input to `0.9` (High).
    *   This bypasses the Cognition layer entirely, allowing instant reaction.

---

## 4. Data Configuration (DT_NPC_Actions.json)
For the Zombie to actually *attack*, the Utility AI must select the Attack action.

*   **Action Name**: `Attack` (or similar).
*   **Considerations**:
    *   Must rely on inputs that Zombies actually have.
    *   **Good**: `Context.HasEnemyNearby`, `World.DistanceToTarget` (Inverse), `Target.Health`.
    *   **Bad**: `Self.Indignity`, `Social.Reputation` (These will be 0 or null).
*   **Example Setup**:
    *   *Action*: `AttackEnemy`
    *   *Consideration 1*: Type `Curve`, Input `PerceivedThreat`, Curve `Linear`.
    *   *Consideration 2*: Type `Curve`, Input `DistanceToTarget`, Curve `InverseLinear` (Closer is better).

---

## 5. Troubleshooting Checklist

| Issue | Cause | Fix |
| :--- | :--- | :--- |
| **Zombie stands still / ignores Player** | Wrong Controller Class | Change AIControllerClass to `AUtilityAIController`. |
| **Zombie is Neutral (Blue Debug)** | Wrong FactionID | Set `NPCDefinitionComponent->FactionID` to `Monsters`. |
| **Zombie sees Player but doesn't attack** | Action Score is 0 | Check `DT_NPC_Actions` for `Attack`. Ensure it doesn't use missing values like Indignity. |
| **Zombie logic overwritten** | Race Condition | `BeginPlay` might be overwriting `OnPossess`. Code has a fix to skip `ApplyDefinition` if `MonsterComponent` exists. |
