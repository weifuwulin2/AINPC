# Example Scenario: The Liberation of Orc Camp (Dynamic Spawning)
This guide demonstrates how to use the **Narrative System** to create a dynamic "Save the Oppressed" scenario using the **Template Spawning System**.

## 1. Data Setup (The "Cast" & "Script")

You need to define **Professions**, **Personalities**, **NPC Templates**, and the **Scene Template**.

### A. Define Professions (`DT_Professions`)
*Updated based on latest design:*

| Row Name | Profession Name | Description (Role) | VIP? |
| :--- | :--- | :--- | :--- |
| **Leader** | Leader | "You are a leader. You command others..." | **YES** (Auto-tagged) |
| **Guard** | Guard | "You are a guard. You protect your post..." | No |
| **Slave** | Slave | "You are a laborer. You perform hard work..." | No |

### B. Define NPC Templates (`DT_NPCDefinitions`)
*Create these rows to define WHO spawns:*

| Row Name | Pawn Class | Profession | Personality | Faction |
| :--- | :--- | :--- | :--- | :--- |
| **Narrative_Orc_Leader** | `BP_HumanoidNPC` | **Leader** | Ambitious | Human |
| **Narrative_Human_Guard** | `BP_HumanoidNPC` | **Guard** | Brave | Human |
| **Narrative_Orc_Slave** | `BP_HumanoidNPC` | **Slave** | Paranoid | Orc |

### C. Define Scene Template (`DT_NarrativeScenes`)
*Create a row in your Scene Data Table to define the LAYOUT:*

**Row Name:** `Scene_OrcRescue`
*   **Plot Outline:** "The cruel Human Lord is giving a speech about human supremacy. The Orc slaves are cowering in fear. The Guards are watching for trouble."
*   **Completion Tags:** `["Status.Dead"]` (or specific tag like `Death_Leader`)
*   **Cast (Array):**
    1.  **Role 1 (Leader):**
        *   TemplateID: `Narrative_Orc_Leader`
        *   RelativeTransform: `(0, 0, 100)` (Standing on box)
        *   RoleOverride: "Tyrant Lord"
    2.  **Role 2 (Guard 1):**
        *   TemplateID: `Narrative_Human_Guard`
        *   RelativeTransform: `(200, 100, 0)`
    3.  **Role 3 (Slave 1):**
        *   TemplateID: `Narrative_Orc_Slave`
        *   RelativeTransform: `(300, -200, 0)`

---

## 2. Level Setup (The "Stage")

1.  **Place Anchor**: Place an instance of `ANarrativeSceneAnchor` in the level.
    *   **SceneTable**: Assign `DT_NarrativeScenes`.
    *   **NPCTable**: Assign `DT_NPCDefinitions`.
    *   **SupportedSceneID**: Set to `Scene_OrcRescue` (or leave empty for random).
    *   **TriggerComponent**: Adjust Sphere Radius to cover the activation area.
    *   **bAutoTriggerOnOverlap**: Ensure it is checked (`True`).
2.  **Place Companion**: Place `BP_Paimon` nearby.

*(No Level Blueprint logic is required. The Anchor handles triggering automatically.)*

---

## 3. How it Plays Out
*(This process is automated by the Anchor Actor)*

### Phase 1: Preparation (BeginPlay)
*   **System Action**: The Reference NPCs and Props are automatically spawned at the Anchor location when the level loads.
*   **State**: They remain `Inactive` (Idle functionality determined by profession) until triggered.

### Phase 2: Action! (Trigger)
*   **Trigger**: Player walks into the Anchor's trigger zone.
*   **System Action**: Narrative Subsystem sends activation signal.
*   **Cognition**: NPCs receive "Scene Started" stimulus and begin roleplaying the plot.
*   **Status**: Participating NPCs get `Status.InScene` tag, suppressing hostile factions.

---

## 4. How it Plays Out

### Phase 1: The Scene
*   **System Action**: NPCs spawn at offsets relative to the Anchor.
*   **LLM Behavior**: 
    *   **Leader**: Acts arrogant (Ambitious + Leader + "Tyrant").
    *   **Slaves**: Act fearful (Paranoid + Slave).

### Phase 2: The Climax
*   **Player Action**: Player kills the Leader.
*   **System Action**:
    *   `NarrativeCompanion` sees death. Checks `VIP` tag (from Leader profession).
    *   `NarrativeDirector` records: `Leader was killed by Player`.

### Phase 3: The Aftermath
*   **System Action**:
    *   `NarrativeSquad` sees Death -> **Ends Scene**.
    *   **Orcs** (Cognition) next update:
        *   Read `WorldHistory`: "Leader was killed".
        *   Cognition Prompt: "Scene Ended".
*   **Result**: Orcs behave gratefully or flee to freedom.

---

## 5. Verification Checklist
- [ ] `SpawnSceneFromTemplate` spawns all actors correct relative to anchor?
- [ ] Lord has `VIP` tag automatically applied? (Check Output Log).
- [ ] Companion logs "Witnessed Death" when Lord dies?
- [ ] Orcs change behavior after the death?
