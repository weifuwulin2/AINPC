# Narrative System Architecture Design

## 1. Overview
This system introduces a multi-layered narrative control structure designed to manage the game world's story, NPC performances, and player party context.

### Core Pillars
1.  **Macro Recorder (World History)**: "What *has* happened?" managed by `NarrativeDirectorSubsystem`.
2.  **Micro Director (Scene Staging)**: "What *is* happening here?" managed by `NarrativeSquadSubsystem`.
3.  **Player Context (The Party)**: "Who is with the player?" managed by `PlayerSquadSubsystem`.
4.  **The Observer (Companion)**: The in-world link that records events.

---

## 2. Subsystem Breakdown

### A. Narrative Director Subsystem (`UNarrativeDirectorSubsystem`)
*The "Historian" & "World Brain".*
-   **Scope**: Global / World.
-   **Responsibility**:
    -   Tracks **World State**: NPC deaths, major plot events, faction reputation shifts.
    -   Acts as the central database for "Paimon's Notebook".
    -   Provides the **World Context** string to the LLM (e.g., "The King is dead. War has started.").
-   **Key Data**:
    -   `TArray<FNarrativeEvent> EventLog`
    -   `TMap<FName, int32> FactionPopulation`
    -   `TSet<FName> DeadVIPs`

### B. Narrative Squad Subsystem (`UNarrativeSquadSubsystem`)
*The "Stage Manager".*
-   **Scope**: Local / Scene.
-   **Responsibility**:
    -   Manages **NPC Acting Groups**.
    -   Allows "Staging a Scene": Designers (or the Director) can designate an area, a plot outline, and assign roles.
    -   **Injection**: Pushes these Role Descriptions directly to the NPC's `CognitionComponent`.
-   **Scene Completion (The "Witness -> Judge" Loop)**:
    -   *User Question: "Is it judged based on narrative squad and companion together?" - **Yes**.*
    -   **The Witness (Companion)**: Observes the outcome (e.g., "The Robber is dead" or "The money was given"). Calls `RecordEvent`.
    -   **The Judge (Squad Subsystem)**: Listens for that specific Event Tag. If it matches the Scene's completion criteria, the Scene ends.
    -   *Flow*: Action -> Companion Records -> Director Broadcasts -> Squad Completes.
-   **Key Data**:
    -   `TMap<int32, FNarrativeSceneSquad>` (Active Scenes)

### C. Player Squad Subsystem (`UPlayerSquadSubsystem`)
*The "Adventure Party".*
-   **Scope**: Player Group.
-   **Responsibility**:
    -   Tracks the **Active Party** (Player + Companions).
    -   Monitors collective statuses (Health, Combat State).
    -   Provides **Party Context** (e.g., "Player is low health. Companion is down.").
-   **Key Data**:
    -   `TArray<AActor*> PartyMembers`

---

## 3. The Narrative Companion (`ANarrativeCompanion`)
*The "Observer".*
-   **Role**:
    -   **Follower**: Member of the **Player Squad**. Follows the player automatically.
    -   **Recorder**: Observes the world. When Significant Events happen (Death, Quest Step), it calls `NarrativeDirectorSubsystem::RecordEvent()`.
-   **Behavior**:
    -   Uses `CognitionComponent` to comment on situations.
    -   Uses `PlayerSquadSubsystem` to know who to follow.

## 4. AI vs. Logic (Agent Architecture)
*User Question: "Do we need multiple agents? One for Squad, one for World?"*

**Clarification**: In this architecture, the **Subsystems are C++ Managers**, not AI Agents. They *manage* the Simulation State to feed the actual NPC Agents.

1.  **NPCs (The Actors)**: **[Active AI Agents]**
    -   Each NPC has a `CognitionComponent` (LLM).
    -   They receive Role/Plot info from the Subsystems and "Act" it out.
2.  **Narrative Director (The Database)**: **[Deterministic Logic]**
    -   It is a **System**, not an AI. It strictly records history and provides strings.
    -   *Future Upgrade*: You *could* hook an LLM here to dynamically generate new plots, but for now, it's just the storage.
3.  **Narrative Squad (The Script Holder)**: **[Deterministic Logic]**
    -   It acts as the "Script" holder. It tells NPC A "You are the robber".
    -   It does not "think"; it just assigns property values.

### The "Loop"
`Director System (Data)` -> `Squad System (Assignment)` -> **`NPC Agent (LLM Execution)`** -> `Companion (Observer)` -> `Director System (Data)`
