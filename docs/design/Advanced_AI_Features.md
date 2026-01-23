# Design Document: Advanced NPC "Soul Injection"

## 1. Overview
The "Soul Injection" initiative aims to elevate NPC behavior from functional utility to deeper, character-driven interactions. This involves introducing backstory-driven personalities, internal conflicts in decision making, and dynamic player interactions.

## 2. Phase 1: The Soul Layer (Lore & Context)
### 2.1 Concept
Inject rich narrative context into the LLM and Utility AI evaluations.

### 2.2 Data Structures
**New Table: `DT_Backstories`**
*   **Key**: `BackstoryID` (e.g., `WarVeteran`, `DisgracedNoble`)
*   **Fields**:
    *   `OriginHistory` (Text): long description of past.
    *   `TraumaTags` (Array<Tag>): `Trauma.Fire`, `Trauma.Betrayal`.
    *   `SocialStatus` (Enum): `Low`, `Middle`, `High`.
    *   `KeyValues` (Array<String>): "Honor", "Survival", "Greed".

### 2.3 Integration
*   `NPCDefinitionComponent` holds `BackstoryID`.
*   `LLMCommunicator` fetches backstory text and injects into System Prompt.
*   **Outcome**: LLM responses reflect social status (tone) and trauma (fear responses).

## 3. Phase 2: The Conflict Layer (Decision Context)
### 3.1 Concept
Simulate internal struggle by exposing the "Runner-up" choice to the Cognition layer.

### 3.2 Logic Flow
1.  **Utility Evaluation**: Calculate scores for all actions.
2.  **Sort**: Rank actions by Score.
3.  **Context Creation**:
    *   `Winner`: Action A (Score 0.9) - Driven by Hunger.
    *   `RunnerUp`: Action B (Score 0.85) - Driven by Curiosity.
    *   `ConflictMagnitude`: (Winner - RunnerUp) / Winner. (Low diff = High Conflict).
4.  **Cognition Injection**:
    *   Pass this context to LLM.
    *   LLM generates speech reflecting the conflict: *"I really want to check that out, but I'm starving."*

## 4. Phase 3: Player Interaction System
### 4.1 Concept
Enable 2-way communication between Player and NPC.

### 4.2 Architecture
*   `WBP_DialogUI`: Simple text input box.
*   `Action_PlayerDialog`: A Utility Action triggered by Player interaction.
    *   Interrupts current action.
    *   Sets `FocusActor` to Player.
*   `CognitionComponent`: Handles streaming LLM response.

## 5. Phase 4: Self-Evolution (Automated RL)
### 5.1 Concept
Automated optimization of NPC behavior based on "Survival & Social Success" metrics.

### 5.2 Mechanisms
*   **Nightly Reflection**: When `Sleep` action is active > 10s:
    *   Read daily log (Damage taken, Social positive outcomes).
    *   LLM generates a "Learned Lesson".
    *   Update `DynamicValues` (e.g., `Trust` decrements if hurt).

## 6. Phase 5: Emergent Behavior (Bard & Stage)
### 6.1 Concept
Create a dynamic event system where a Performer NPC attracts an Audience purely through Utility AI mechanisms, without scripted sequences.

### 6.2 The Dynamic Stage (SmartObject)
*   **Component**: `BP_Stage_SmartObject` (Conditional SmartObject).
*   **Slots**:
    *   **Performer Slot**: Tag `Interaction.Work.Perform`. Always enabled.
    *   **Audience Slots**: Tag `Interaction.Watch`. **Conditional**.
*   **Logic**:
    1.  **Trigger**: `OnSlotOccupied(PerformerSlot)` -> **Enable** Audience Slots.
    2.  **Reset**: `OnSlotFreed(PerformerSlot)` -> **Disable** Audience Slots.
*   **Emergent Flow**:
    1.  Bard receives `Directive.Work` and occupies Performer Slot.
    2.  Stage broadcasts availability of `Interaction.Watch`.
    3.  Nearby NPCs (Villagers) scan environment, find `Interaction.Watch`.
    4.  If `Boredom` is high, `Action_Watch` scores high.
    5.  Crowd naturally forms around the Bard.

## 7. Phase 6: Generic Animation System (Idle Variety)
### 7.1 Concept
Allow NPCs to perform diverse "Flavor Actions" (Dance, Cry, Stretch) without creating unique C++ classes for each.

### 7.2 Implementation: `Action_SimpleAnim`
*   **Class**: New C++ class `UAction_SimpleAnim : UUtilityActionBase`.
*   **Functionality**:
    *   Does NOT require a SmartObject (can play in place).
    *   Plays the `InteractionMontage` defined in `DT_UtilityActions`.
    *   Applies `EmotionMatrix` multipliers (e.g., Happy -> Dance).
*   **Data Driven Variety**:
    *   Define many actions in JSON: `Idle_Twist`, `Idle_Sneeze`, `Idle_CheckPhone`.
    *   All driven by `Boredom` but with slight random weights or Personality tags.
    *   **Result**: NPCs exhibit "Life" through varied idle behaviors instead of static standing.

## 8. Phase 7: Human-in-the-Loop RLHF (Dialogue Tuning)
### 8.1 Concept
A "Character Authoring Tool" where developers can chat with an NPC, "Like" or "Edit" their responses, and the system automatically updates the NPC's style prompt. This ensures high-quality personas without unpredictable self-evolution.

### 8.2 Workflow
1.  **Chat**: Dev interacts with NPC via Debug UI.
2.  **Feedback**: Dev clicks "Upvote" or "Edit" on a specific response.
3.  **Collection**: System stores `{Context, Response, Rating}` pairs.
4.  **Distillation**: Meta-LLM extracts style keywords and rewrites prompt.

## 9. Phase 8: Agent-in-the-Loop (Automated "Director")
### 9.1 Concept
Replace the human developer in Phase 7 with a specialized "Director Agent". This allows for fully automated, massive-scale personality tuning overnight.

### 9.2 Architecture
*   **The Director (Critic Agent)**: An LLM instance configured with a "Casting Brief" (e.g., "A grumpy dwarf who hates elves").
*   **The Actor (NPC Agent)**: The NPC being tuned.
*   **The Loop**:
    1.  **Director** initiates conversation: *"Hello there, sir."*
    2.  **Actor** responds: *"Hi."*
    3.  **Director** evaluates (Internal Monologue): *"Too bland. The brief says 'Grumpy'. Rating: 1/5."*
    4.  **Director** provides Feedback: *"Try again. Sound more annoyed."*
    5.  **Optimization**: System updates Actor's prompt based on Director's critique.
    6.  *Repeat 100 times*.
*   **Result**: High-fidelity personas generated without human intervention.

## 10. Phase 9: Local Model Distillation (Edge AI)
### 10.1 Concept
Transition from Cloud APIs (Development) to Local LLMs (Production/Shipping). Use the high-quality dataset generated by the "Director Agent" (Phase 8) to fine-tune a small, efficient local model (e.g., Llama-3-8B-Quantized).

### 10.2 Workflow
1.  **Dataset Generation**: The Director Agent generates 10,000+ "Perfect Interactions" where the Actor correctly embodied the persona.
2.  **Fine-Tuning**: Train a base model (Mistral/Llama) on this dataset using LoRA (Low-Rank Adaptation).
3.  **Quantization**: Compress the model to 4-bit (GGUF format) for consumer hardware.
4.  **Deployment**: Ship the local model with the game, removing the need for an internet connection or API costs.

## 11. Phase 10: Future Research (Utility AI Self-Calibration)
### 10.1 Concept
Use Inverse Reinforcement Learning (IRL) to automatically tune the "Magic Numbers" of Utility AI (Weights, Curves) based on observed behavior.

### 10.2 Workflow
1.  **Observation**: Record NPC behavior logs (State -> Action chosen).
2.  **Evaluation**: Director Agent evaluates if the behavior matches the Personality Profile (e.g., "Did the 'Workaholic' farmer keep working when hungry?").
3.  **Calibration**: System adjusts the underlying Utility parameters (e.g., increasing `Conscientiousness_Work_Multiplier`) to align future decisions with the desired profile.
4.  **Result**: An AI that "learns" how to be itself, not just in dialogue, but in behavior.

## 12. Implementation Roadmap
1.  Verify Profession System functionality.
2.  Implement `DT_Backstories` structure.
3.  Modify `UtilityAIComponent` to track Top-2 Actions.
4.  Build Dialog UI.
5.  Implement Conditional Stage SmartObject.
6.  Implement `Action_SimpleAnim` class.
7.  Build RLHF Tooling (Human).
8.  Build "Director Agent" Harness (Automated).
9.  Train & Quantize Local Model (Distillation).
10. Research Utility AI Self-Calibration.
