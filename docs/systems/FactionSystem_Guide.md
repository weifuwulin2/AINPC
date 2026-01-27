# Faction System Architecture Guide

## Overview

The Faction System in Project AINPC determines how NPCs perceive and interact with other entities in the world (Players, NPCs, Monsters). It uses a **Dual-Layer Architecture** that combines global "factory settings" with individual, dynamic relationships.

### Core Philosophy
1.  **Macro Layer (Nature)**: "I am a Human, so I naturally fear Orcs."
2.  **Micro Layer (Nurture)**: "But *this specific Orc* saved my life, so I trust him."

---

## 1. Global Faction Matrix (The "Nature" Layer)
**Class**: `UFactionSubsystem`  
**Data**: `DT_Factions`

This layer defines the default starting relationships between entire groups. It is managed by a Game Instance Subsystem, ensuring it persists across levels.

### Data Structure
The `RuntimeFactionMatrix` is a `TMap<FName, TMap<FName, float>>`.
*   **Key**: Source Faction ID (e.g., "Humans")
*   **Value Map**: 
    *   **Key**: Target Faction ID (e.g., "Orcs")
    *   **Value**: Attitude Score (0.0 - 100.0)

### Default Values
*   **Friendly (75+)**: Allies. Will help in combat.
*   **Neutral (25-75)**: Indifferent. Will ignore unless provoked.
*   **Hostile (<25)**: Enemies. Will attack on sight.

**Example Matrix:**
| Source \ Target | Humans | Orcs | Monsters |
| :--- | :--- | :--- | :--- |
| **Humans** | 100 (Self) | 40 (Neutral) | 0 (Hostile) |
| **Orcs** | 20 (Hostile) | 100 (Self) | 10 (Hostile) |
| **Monsters** | 0 (Hostile) | 0 (Hostile) | 100 (Self) |

---

## 2. Personal Reputation (The "Nurture" Layer)
**Class**: `UFactionReputationComponent`  
**attached to**: `BaseNPC`, `PlayerCharacter`

This layer handles specific, individual relationships. It contains a map of *specific actors* that overrides the global defaults.

### Data Structure
`PersonalReputations` is a `TMap<AActor*, float>`.
*   **Key**: Specific Actor Reference (The specific `BP_Player_C_1` or `BP_Merchant_C_5`).
*   **Value**: Attitude Score (0.0 - 100.0).

### Key Logic
When an NPC decides "Do I like this person?", it runs:

```cpp
float UFactionReputationComponent::GetAttitudeTowards(AActor* Target)
{
    // 1. Check Personal Override (Micro)
    if (PersonalReputations.Contains(Target))
    {
        return PersonalReputations[Target]; // "I hate THIS guy specifically"
    }
    
    // 2. Check Global Faction (Macro)
    FName MyFaction = CurrentFactionID;
    FName TargetFaction = GetFactionID(Target);
    
    return FactionSubsystem->GetBaseAttitude(MyFaction, TargetFaction); // "Humans generally like Elves"
}
```

---

## 3. Data Flow & Integration

### A. Sensory Input
The `SensoryComponent` now includes Faction context when generating memory events:
- **Event**: "Player Player_1 killed Zombie_5"
- **Context**: "Player is Human (Ally), Zombie is Monster (Enemy)."
- **Result**: "Good job!" (Positive Reputation boost)

### B. Decision Making (Utility AI)
Utility Actions use `IsHostile(Target)` to filter valid targets.
- **`Action_Attack`**: Only selects targets where `IsHostile() == true`.
- **`Action_TalkTo`**: only selects targets where `IsHostile() == false`.

### C. Example Scenarios

#### Scenario 1: The Bread Thief
1.  **State**: Guard (Human) is Neutral to Player (Human). Attitude: 50.
2.  **Event**: Player steals bread in front of Guard.
3.  **Process**: 
    - Sensory System sees `Event_Crime`.
    - `FactionReputationComponent` reduces `PersonalReputations[Player]` by -30.
4.  **Result**: Guard Attitude = 20 (Hostile). Guard attacks Player.
5.  **Nuance**: Other Guards in the next town *didn't see it*. They check Global Matrix (Human vs Human = 50). They are still Neutral.

#### Scenario 2: The Peace Treaty
1.  **State**: Humans and Orcs are at war. Global Attitude = 0.
2.  **Event**: Quest completed "Treaty of Iron".
3.  **Process**: 
    - `UFactionSubsystem::SetFactionRelation("Humans", "Orcs", 60.0f)` called.
4.  **Result**: 
    - INSTANTLY, every Human NPC in the world checks the matrix and sees Orcs as Neutral (60).
    - Combat stops immediately unless there is a *Personal* grudge (Override < 25).

---

## 4. How to Use

### For Designers (Data)
1.  Open `DT_Factions`.
2.  Add a new Row (e.g., "Bandits").
3.  Set `DefaultReputations` map:
    - `Humans`: 0 (Hostile)
    - `Monsters`: 50 (Neutral - they ignore each other)

### For Programmers (Code)
**Checking Hostility:**
```cpp
if (ReputationComp->IsHostile(TargetActor))
{
    Attack(TargetActor);
}
```

**Modifying Relationships:**
```cpp
// Make this specific NPC like the Player more
ReputationComp->ModifyReputation(PlayerActor, +10.0f);

// Declare global war
GetGameInstance()->GetSubsystem<UFactionSubsystem>()->SetFactionRelation("Elves", "Dwarves", 0.0f);
```
