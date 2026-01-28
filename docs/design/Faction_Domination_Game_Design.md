# 🌍 Game Design: Faction Survival & Influence

## 1. Core Concept
**"Survive the chaos, or control it."**

The player enters a volatile world where factions are vying for control. Your actions (Dialogue, Combat, Trading) directly impact how these factions perceive you.
*   **Low Reputation**: You are hunted. Survival becomes difficult.
*   **High Reputation**: You gain allies, protection, and the ability to influence global politics.

You can choose to be a **Warlord** (help one faction wipe out others) or a **Peacemaker** (unite the world).

## 2. The Gameplay Loop

### A. The "Butterfly Effect" (Interaction)
Every interaction changes the `FactionReputation` indices:
*   **Combat**: Killing an Orc -> Orcs hate you (-10), Humans/Elves like you (+5).
*   **Dialogue**: Insulting a Leader -> Immediate Hostility.
*   **Assistance**: Healing a wounded NPC / Giving Food -> Trust gain.

### B. Survival Pressure (Response)
The world reacts to your reputation:
*   **Hostile (-50)**: Faction NPCs attack on sight.
*   **Nemesis (-80)**: Faction spawns "Hunter Squads" specifically to track you down.
*   **Friendly (+50)**: You can access their Safe Zones (SmartObjects like Beds/Food).
*   **Allied (+80)**: They follow your commands ("Follow Me", "Attack This").

### C. Territory & Building
To survive or help a faction win, you must establish **Control**:
*   **Capturing**: Taking a `FactionFlag` converts the area.
    *   *Effect*: Nearby spawners produce friendly units.
    *   *Effect*: Nearby Resources (Mines/Farms) become available to that faction.
*   **Building (Stretch Goal)**: Spending resources to upgrade a Flag.
    *   *Tier 1*: Basic Spawn.
    *   *Tier 2*: add Guards.
    *   *Tier 3*: add Auto-Turrets / Shops.

## 3. Win Conditions

### 🏆 Path of the Conqueror (Domination)
*   **Goal**: Ensure your Allied Faction controls **80% of Territory**.
*   **Method**: Combat, Assassination of rival leaders, Sabotage of enemy supplies.

### 🕊️ Path of the Diplomat (Peace)
*   **Goal**: All Factions have **Neutral (0+) or Friendly** relations with each other.
*   **Method**:
    *   Complete "Messenger" missions between leaders.
    *   Use high `Charisma` dialogue options to de-escalate conflicts.
    *   Resource Redistribution (Steal from rich, give to poor to reduce 'Envy' drives).

## 4. Systems Architecture

### Components
1.  **`PlayerReputationTracker`**:
    *   Tracks Player's standing with every Faction.
    *   Triggers "Hunter Squad" events if Rep is too low.
2.  **`FactionDiplomacyNetwork`**:
    *   A graph of Faction-to-Faction relations.
    *   Updated by events (e.g., "Orc killed Elf" -> Relation -5).

### AI Behavior Changes
*   **Aggro System**: Now checks `FactionReputationComponent` for the Player specifically.
*   **Help Logic**: Friendly NPCs will assist the Player if they are attacked near them.

## 5. Implementation Roadmap (Sprint Adjusted)

### Phase 1: The "Reputation" Engine
*   Hook up `OnDeath` / `OnDialogue` to modify Reputation.
*   UI: "Faction Standing" Screen.

### Phase 2: Territory Dynamics
*   Implement `BP_FactionFlag` (Capture Point).
*   Link Spawn Rates to Territory Control.

### Phase 3: The "Peace" Logic
*   Implement `Action_Diplomacy` for Player (Dialogue options).
*   Win Condition Verification Logic.

