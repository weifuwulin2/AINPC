# 🕊️ Project Peacemaker: The Isekai Hero Design Document

> *"You are the anomaly in a dying world. Will you conquer the chaos, or cure it?"*

## 1. Vision & Narrative
The world of **Aethon** is trapped in a **Cycle of Hatred**. Factions fight not because they are inherently evil, but because they are desperate—driven by Hunger, Fear, and Pride. 
As an **Isekai Hero** (an outsider from another world), you possess a unique power: **The Administrator Eye**. You can see the *hidden variables* (Hunger, Trust, Neuroticism) that drive these conflicts. Your mission is not to kill the "Demon King", but to break the cycle and achieve the **True Ending: Eternal Peace**.

---

## 2. World State: The Spiral
The simulation begins in a state of decay. If left alone, the world will destroy itself in ~20 minutes.

### The Factions
*   **The Orcs (The Desperate)**:
    *   **Drive**: 🔴 High Hunger, Low Trust.
    *   **Conflict**: They raid human farms because their own lands are barren. To them, it's survival.
*   **The Humans (The Vengeful)**:
    *   **Drive**: 🔴 High Neuroticism, High Aggression (defensive).
    *   **Conflict**: They see Orcs as monsters. Every raid confirms their bias. They hoard food out of fear.
*   **The Elves (The Arrogant)**:
    *   **Drive**: 🟡 High Pride, Zero Openness.
    *   **Conflict**: They have abundant magic (Water/Healing) but refuse to share with "lesser races".

---

## 3. Core Mechanics: The Hero's Kit

### I. The "Why" Engine (Empathy)
You don't just see "Enemy". You see **Why**.
*   **Visual Debugger**: Above an attacking Zombie's head, you see `[Hungry: 0.9]`.
*   **Insight**: You realize the Zombie isn't attacking you; it's attacking your *sandwich*.

### II. The Bridge (Diplomacy Actions)
You are the only entity that can speak to everyone.
*   **Action_Translate**: An Orc growls. You tell the Human: *"He says his child is starving."*
*   **Action_Gift**: You take bread from the Human warehouse (Steal?) and give it to the Orc.
    *   *Result*: Orc Hunger ↓, Human Trust (in you) ↓, **Global Tension ↓**.

### III. The Summit (Resolution)
The ultimate goal is to bring Leaders to the **Negotiation Table**.
*   **Prerequisite**: You must have 0+ Reputation with both factions.
*   **The Event**:
    1.  **Escort**: Guide the Orc Chief to the Human Village (don't let guards shoot him!).
    2.  **Debate**: Use "Truth Bullets" (Facts you learned) to counter their prejudice.
    3.  **Defense**: Surviving the "Saboteurs" (Nihilists who want war) during the meeting.

---

## 4. Gameplay Loop

### Phase 1: Survival (The Outcast)
*   **Status**: Factions hate you (-50 Rep). You have nothing.
*   **Gameplay**: Scavenge, Hide, Listen.
*   **Objective**: Learn the **Root Cause** (e.g., "The River Logic" - The Elves blocked the river upstream).

### Phase 2: Intervention (The Catalyst)
*   **Status**: Neutral (0 Rep). You have resources.
*   **Gameplay**:
    *   **Side Quest**: Break the Elf Dam (Sabotage) OR Convince the Elf Highlord (Charisma).
    *   **Side Quest**: Cure the Orc Plague (Magic/Medicine).
    *   **Objective**: Gain the trust of Leader A and Leader B.

### Phase 3: Unification (The Peacemaker)
*   **Status**: Hero (+80 Rep). Leaders listen to you.
*   **Gameplay**: Trigger **The Summit**.
*   **Win Condition (True Ending)**:
    *   Orcs and Humans share a meal.
    *   Elves open the river.
    *   **System Message**: `[Chaos Level: 0%] [World Stabilized]`

---

## 5. System Requirements (Technical)

### AI Systems
1.  **Reasoning Layer**: NPCs must weigh `Physical Needs` vs `Social Hatred`.
    *   *Scenario*: If `Hunger > 0.9`, an Orc will eat bread offered by a Human instead of attacking.
2.  **Faction Memory**: Factions must "remember" your good deeds.

### New Components
1.  **`WorldHarmonySubsystem`**: Tracks Global Tension (0-100). Spawns Disasters.
2.  **`EmpathyHUD`**: The UI that shows NPC hidden stats.

---

## 6. The Pitch (Demo Scene)
We record a 3-minute video:
1.  **Chaos**: A battle starts. Music is tense.
2.  **Intervention**: You jump in. You don't attack. You **Block**.
3.  **Connection**: You drop a **Feast**.
4.  **Resolution**: The AI logic switches from `State_Attack` to `State_Eat`. The music resolves to a peaceful theme.
5.  **Tagline**: *"Don't just kill monsters. Save them."*
