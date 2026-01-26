# Design Document: Neural Utility Architecture ("The Double Brain")

## 1. Overview
This document outlines the "North Star" architecture for the next generation of AINPC: a **Neuro-Symbolic** system that combines the speed of Utility AI with the reasoning depth of LLMs.

The core philosophy is **"System 1 & System 2"** (Thinking Fast and Slow):
*   **System 1 (Reflexes)**: C++ Vector Math. Extremely fast, handles real-time behavior.
*   **System 2 (Reasoning)**: Local LLM. Slower, handles complex social understanding and strategic shifts.


---

## Architecture Visualized

```mermaid
graph TD
    %% Nodes
    World(World State)
    
    subgraph System 1 ["System 1: Fast Brain (Reflexes)"]
        direction TB
        VectorMap[Topic/Vector Mapping]
        Context[Current Context Vector]
        Matcher[Dot Product Matching]
        Action[Execute Action]
    end
    
    subgraph System 2 ["System 2: Slow Brain (Reasoning)"]
        direction TB
        LLM[Local LLM / Reasoning]
        Inject[Vector Injection]
    end
    
    Library[(Concept Library)]

    %% Connections
    World -->|Raw Data (60fps)| VectorMap
    World -.->|Complex Events (Async)| LLM
    
    VectorMap --> Context
    LLM -->|Decision| Inject
    Inject -.->|Modify Meaning| Context
    
    Context --> Matcher
    Library -.->|Action Embeddings| Matcher
    Matcher -->|Highest Score| Action
```

---

## 2. A Paradigm Shift: From "Scoring" to "Matching"
Traditional Utility AI calculates scores based on hard-coded formulas:
`Score = (Health * 0.5) + (Hunger * 0.2)`

**Neural Utility AI** calculates scores based on **Semantic Similarity**:
`Score = DotProduct(CurrentStateVector, ActionVector)`

If the *Meaning* of the current situation matches the *Meaning* of an action, the NPC does it.

---

## 3. Architecture Components

### 3.1 The Concept Library (Offline / Pre-baked)
We pre-calculate the "Meaning" of every Action and State using an Embedding Model (e.g., `all-MiniLM-L6-v2`).

*   **Action Embeddings (`TMap<ActionName, FVector>`)**:
    *   `Action_Eat` -> `[0.1, 0.8, -0.2, ...]` (Semantically close to "Hunger", "Survival")
    *   `Action_Flee` -> `[0.9, -0.1, 0.5, ...]` (Semantically close to "Danger", "Fear")
    *   `Action_Bargain` -> `[-0.5, 0.4, 0.8, ...]` (Semantically close to "Social", "Trade")

### 3.2 System 1: The Vector Observer (Fast / C++)
Runs every frame (or tick).

1.  **Observation**: Collects raw data (HP=10, Enemy=Orc).
2.  **Vector Mapping**: Maps raw data to *Pre-baked State Vectors*.
    *   `HP < 20` -> retrieves `Vector_Dying`
    *   `Enemy == Orc` -> retrieves `Vector_Threat_High`
3.  **Synthesis**: Combines vectors (weighted sum).
    *   `CurrentContext = (Vector_Dying * 0.8) + (Vector_Threat_High * 0.5)`
4.  **Execution**: Computes Cosine Similarity between `CurrentContext` and all `ActionEmbeddings`.
    *   Winner: `Action_Flee` (Highest Similarity).

**Performance**: Pure math (Multiply & Add). 1000+ NPCs feasible on CPU.

### 3.3 System 2: The Local Reasoning Core (Slow / Python or LocalService)
Runs asynchronously (e.g., once every 2-5 seconds, or on event).

1.  **Complex Input**: "Player is holding a sword, but he saved my sister yesterday."
2.  **LLM Reasoning (Llama-3-8B)**:
    *   *Thought*: "The sword is a threat, but the 'Savior' tag overrides it. This is a friendly interaction."
    *   *Output*: "Shift Context towards 'Gratitude' and 'Trust'."
3.  **Vector Injection**:
    *   The LLM does not call `StopMovement()`.
    *   Instead, it sends a `Vector_Gratitude` modifier to System 1.
4.  **Result**:
    *   System 1 recalculates. `Vector_Dying` + `Vector_Threat` + `Vector_Gratitude`.
    *   Suddenly, `Action_Greeting` scores higher than `Action_Flee`.

---

## 4. Why this is the "Ultimate Move" (The Kill Shot)

1.  **No Hard-Coding**: You don't write `if (SavedSister) return DontFlee`. The *meaning* of "Gratitude" naturally cancels out the *meaning* of "Threat" in the vector space.
2.  **Emergent Behavior**:
    *   If the LLM decides the situation is "Awkward", the vector shifts.
    *   The NPC might choose `Action_LookAway` or `Action_Cough` because those actions are semantically closest to "Awkwardness".
    *   This happens without you explicitly scripting an "Awkward Moment" handler.
3.  **Performance**: The heavy lifting (LLM) happens rarely. The reaction (System 1) happens instantly.

## 5. Implementation Stages

### Stage 1: The MVP (Static Vectors)
*   **Tools**: Python script (export vectors to JSON), C++ JSON Loader.
*   **Goal**: Replicate existing behavior (Hungry -> Eat) using Vector Dot Product instead of Curves.
*   **Verify**: Does it work? Is it fast?

### Stage 2: Hybrid Input
*   **Tools**: Simple mapping.
*   **Goal**: Mix "Personality Vector" (e.g., Brave) into the calculation.
*   **Result**: "Brave" NPCs naturally fight more because `Vector_Brave` is close to `Vector_Attack`.

### Stage 3: The Brain (Local LLM)
*   **Tools**: `llama.cpp` server / ONNX Runtime.
*   **Goal**: Connect the "Slow Brain" to modify the vectors dynamically.
*   **Final Form**: True Neuro-Symbolic AI.

## 6. Integration Guide: Bridging the Gap

### 6.1 Question: How to map ALL game variables to Context?
We do NOT embed every raw number (e.g., `Health=73.5`, `Ammo=12`). We embed **Concepts**.

**The Concept Bucketing Layer**:
1.  **Normalization**: Convert raw values to 0-1 range.
    *   `HealthPct = CurrentHP / MaxHP`
2.  **Semantic Mapping (Bucketing)**:
    *   If `HealthPct < 0.2` -> Add `Vector_Dying` (Weight 1.0)
    *   If `HealthPct > 0.8` -> Add `Vector_Healthy` (Weight 0.5)
    *   If `CanSeeEnemy` -> Add `Vector_Threat` (Weight = 1.0 / Distance)
3.  **Synthesis**:
    *   `ContextVector = (Vector_Dying * 1.0) + (Vector_Threat * 0.8)`

**Why?**
*   Embedding "Health=73" generates a random vector that means nothing semantically.
*   Embedding "I am dying" generates a vector close to "Fear" and "Flee".
*   **Result**: We only need ~50 "Concept Vectors" (Dying, Hungry, Safe, Wealthy) to describe any game state.

### 6.2 Question: Is the existing Utility AI useless?
**No. It is the Foundation.**

1.  **Actions are Reused**:
    *   The **C++ Logic** (`UAction_Attack::Execute`, `UAction_Eat::Enter`) remains 100% unchanged.
    *   Neural Utility only replaces the **Scoring Mechanism** (`CalculateScore`), not the execution.

2.  **Hybrid Evolution**:
    *   You don't throw away your `Considerations`.
    *   Instead of `Score = A + B`, you use them to **build the vector**.
    *   Legacy: `Input(Hunger) -> Score += 50`
    *   Neural: `Input(Hunger) -> ContextVector += Vector_Hungry * 0.8`

**Conclusion**: Your current work is the "Body". Neural Utility is just a better "Brain" to drive that body.

## Appendix A: The Math behind Matching (Dot Product)

**Scenario**: You are both **Dying** (HP < 10) and **Greedy** (Near Gold).
*   **Context Vector**: `V_Ctx = [0.9 (Danger), -0.1 (Social), 0.4 (Greed)]`
    *   *Interpretation*: Danger is the dominant dimension.

**Scoring Actions via Dot Product**:

1.  **Action: Flee** (`V_Flee = [0.95, -0.2, 0.1]`)
    *   Calculation: `(0.9 * 0.95) + (-0.1 * -0.2) + (0.4 * 0.1)`
    *   Result: `0.855 + 0.02 + 0.04` = **0.915** (✅ Very High Match)

2.  **Action: PickUp** (`V_PickUp = [0.1, 0.8, 0.5]`)
    *   Calculation: `(0.9 * 0.1) + (-0.1 * 0.8) + (0.4 * 0.5)`
    *   Result: `0.09 - 0.08 + 0.2` = **0.21** (❌ Low Match)
    *   *Why?* The alignment in the "Greed" dimension (+0.2) was overwhelmed by the mismatch in the "Danger" dimension.

    *   Result: `0.09 - 0.08 + 0.2` = **0.21** (❌ Low Match)
    *   *Why?* The alignment in the "Greed" dimension (+0.2) was overwhelmed by the mismatch in the "Danger" dimension.

**Result**: The system automatically prioritizes Survival over Greed without a single `if` statement. The vector direction handles the trade-off naturally.

## Appendix B: The "TinyStories" Optimzation (Engineering Strategy)

**Core Insight**: We don't need a 7B parameter model. We need a **<30M Parameter "TinySurvivor" Model**.

### 1. The Bottleneck: Latency
*   Cloud LLM: 500ms+ (Unacceptable for combat).
*   Local Llama-7B: 50ms-100ms (Acceptable but heavy).
*   **Tiny Model (30M)**: **<5ms** (Perfect for real-time).

### 2. The Solution: Synthetic Distillation
Instead of training on Wikipedia (General Knowledge), we train on **Synthetic Game Logs**.

1.  **Generate Data (Phase 8)**: Use GPT-4 to generate 50,000 examples of simple game logic.
    *   *Input*: "HP: 10, EnemyDist: 5m, Weapon: Pistol"
    *   *Reasoning*: "Danger Critical. Threat Close. Run away."
    *   *Vector*: `[0.9, -0.8, -0.1...]`
2.  **Train Tiny Model**: Train a 30M parameter transformer to output *only* this reasoning/vector.
3.  **In-Game Inference**: Run this tiny model on CPU every 500ms.

**Result**: An AI that is "Dumb" at poetry but "Genius" at Survival, running with zero lag.

## Appendix C: Social Memory Integration ("Generative Agents" Insight)

**Core Concept**: Relationships are not static variables (`Trust = 50`). Relationships are **Retrieved Memories**.

### 1. The Pipeline
1.  **Sight**: NPC sees `Player`.
2.  **Retrieval**: `MemoryComponent` fetches Top-3 memories about `Player`.
    *   *Memory*: "Player gave me food." (Positive)
    *   *Memory*: "Player killed a zombie." (Protector)
3.  **Reflection (Dreaming)**: The LLM generalizes these into a Concept Tag: **"Protector"**.
4.  **Vector Mapping**:
    *   Look up `Vector_Protector` in the Concept Library.
    *   `Vector_Protector` is semantically close to `Action_Greet` and `Action_Follow`.
5.  **Synthesis**:
    *   `ContextVector += Vector_Protector * 1.0`

**Result**: The NPC treats the player as a friend because of *what happened*, not because of a hard-coded Faction ID.
