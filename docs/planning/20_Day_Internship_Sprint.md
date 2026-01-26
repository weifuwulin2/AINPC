# 🏃‍♂️ 20-Day Internship Sprint: "The Supercell Pitch"

## 1. Overview
**Objective**: Build a portfolio-ready demo showcasing "Emergent Gameplay" and "Hybrid AI Architecture" for the Supercell AI Lab application.
**Theme**: "Small Village, Big Chaos."

---

## 📅 Days 1-4: The "Foundation" Phase (Stability)
**Goal**: Ensure the AI can play the game without getting stuck.
*   **Day 1-2: Action Interruption System**
    *   **Priority**: **P0**. Fixing "Zombies not attacking because stuck in Patrol".
    *   **Task**: Implement `ShouldExit()` checks and `Action_Idle` fallback.
    *   **Validation**: Verify NPC successfully interrupts `Action_Patrol` when `Threat > 0.5`.
*   **Day 3-4: Single NPC Debugging**
    *   **Task**: Create a clean `Map_Test_AI`.
    *   **Test Case**: Spawn 1 NPC. Watch it Cycle: Hungry -> Eat -> Sleep -> Patrol.
    *   **Fix**: Resolve any "Stuck" states or "Jittering" between actions.

## 📅 Days 5-9: The "Chaos" Phase (Gameplay)
**Goal**: Create visible, unscripted conflict between NPCs.

*   **Day 5: `UAction_Sabotage`**
    *   **New Action**: Create `Source/Actions/Action_Sabotage.cpp`.
    *   **Logic**: Find a "Functional" SmartObject (Bed, Food) -> Play Animation (Tinker) -> Apply `Broken` Tag.
    *   **Trigger**: High `Neuroticism` (Naughty) or `Hunger` (Desperate).
*   **Day 6-7: `BP_BreakableObject`**
    *   **Logic**: SmartObject that has two states: `Working` and `Broken`.
    *   **Interaction**:
        *   `Working`: Can be used for `Action_Sleep` / `Action_Eat`.
        *   `Broken`: NPC arrives -> Plays "Disappointed" Anim -> Reduces Trust -> Needs `Action_Repair`.
*   **Day 8-9: The "Director" Subsystem**
    *   **Simple Manager**: `ADirectorSubsystem` (C++).
    *   **Function**: Randomly instructs a "Saboteur" NPC to break something every ~2 minutes.
    *   **Why**: Guarantees chaos for the video without waiting.

---

## 📅 Days 10-14: The "Tech" Phase (Architecture)
**Goal**: visualize the "Brain" so recruiters can see it working.

*   **Day 10-12: Visual Debugger ("Thought Bubbles")**
    *   **UI**: specific `WBP_OverheadDebug`.
    *   **Data**: Show the utility logic chain:
        *   `[State: Hungry (0.8)]`
        *   `[Context: Bed is Broken]`
        *   `[Decision: Steal Apple]`
*   **Day 13-14: Neural Utility MVP (The "Tech Flex")**
    *   **Implementation**: "Scheme 1" (Pre-baked Vectors).
    *   **Task**:
        *   Create `UVectorDatabase` (Load JSON).
        *   Modify `UtilityActionBase::CalculateScore` to include `DotProduct`.
        *   **Validation**: Show a console log proving "Hungry Vector" matches "Eat Action".

---

## 📅 Days 15-20: The "Packaging" Phase (Polish)
**Goal**: Record and Document.

*   **Day 15-17: The Demo Map**
    *   **Level Design**: A tiny square village.
    *   **Cast**:
        *   1x **The Mayor** (High Conscientiousness, tries to repair).
        *   1x **The Saboteur** (High Neuroticism, breaks things).
        *   3x **Villagers** (Neutral, react to chaos).
*   **Day 18-19: Recording & Editing**
    *   **Scene 1**: Saboteur poisons the well (Visual Debugger ON).
    *   **Scene 2**: Villager drinks -> Gets Sick -> Mayor panics.
    *   **Scene 3**: Code montage (Show `MentalStateFields.h` and `NeuralUtility`).
*   **Day 20: Submission**
    *   Finalize `README.md`.
    *   Upload Video unlisted to YouTube.
    *   Submit Application.
