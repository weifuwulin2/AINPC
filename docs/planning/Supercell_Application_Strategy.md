# 🦅 Operation: Supercell AI Lab (20-Day Sprint)

## 1. Feasibility Analysis
**Is it feasible?**
**YES.** Your project hits the "Sweet Spot" for modern Game AI research:
1.  **Systemic AI ("Chaos is Fun")**: This is Supercell's DNA. They love simulation-heavy interactions (Clash of Clans, Hay Day). Your "Emergent Gameplay" aspect is your strongest selling point.
2.  **Hybrid Architecture**: Pure LLM agents are too slow/expensive for mobile games (Supercell's market). Your **Phase 9 (Local Edge AI)** + **Utility AI (performance)** architecture shows you understand the *constraints* of commercial game development, not just the hype.
3.  **Modern Tech Stack**: Unreal Engine 5 + Python Bridge + C++ is the industry standard for Technical Designers/Engineers.

**What they look for:**
*   **"Playable" Innovation**: Not just a paper, but something that is *fun to watch*.
*   **Optimization**: Can this run on mobile? (Your Context LOD and C++ logic prove this).
*   **Game Design Sense**: Do you understand *why* AI is fun? (Your "Saboteur" and "Bard" scenarios prove this).

---

## 2. 20-Day Execution Roadmap

### 📅 Phase 1: The "Visual Proof" (Days 1-7)
**Goal**: A video that explains *what* the AI is thinking.
*   **Visual Debugger**: Polish your `GameplayDebuggingComponent` or Widget overhead.
    *   Show Floating Text: "Goal: Survival (Hungry)" -> "Action: Steal Apple" -> "Context: Guard Watching" -> "Response: Whistle (Pretend)".
    *   *Why*: Engineers love seeing the decision tree.
*   **The "Micro-World" Demo**:
    *   Set up a small Village map.
    *   Spawn 10 NPCs with different Personalities (Trait Tags).
    *   Spawn 1 Monster (or a Player Saboteur).
    *   **Record**: Let it run for 10 minutes. Cut a 30-second highlight reel of emergent interactions (e.g., A Coward running away vs. A Hero fighting).

### 📅 Phase 2: The "Technical Flex" (Days 8-14)
**Goal**: Prove the architecture is robust and future-proof.
*   **Local LLM Integration (Prototype)**:
    *   Even if it's slow, hook up `llama.cpp` or a Python script to control *one* special NPC (The "Mayor").
    *   Show the "System 1 vs System 2" duality: Utility AI handles the crowd, LLM handles the Mayor's complex orders.
*   **Optimization Showcase**:
    *   Document your "Context LOD" system. Show a graph: "Request Count vs. Distance".
    *   This proves you care about battery life and CPU budgets (Mobile DNA).

### 📅 Phase 3: The Pitch (Days 15-20)
**Goal**: Packaging.
*   **The Video**: 2-3 Minutes max.
    *   0:00-0:30: **The Hook**. "I built a town where chaos is the fun." (Show Emergent clip).
    *   0:30-1:00: **The Tech**. "Hybrid V3 Architecture: C++ Reflexes + LLM Reasoning." (Show Debug UI).
    *   1:00-1:30: **The Future**. "Neural Utility & Local Distillation." (Show your Roadmap/Design Docs).
    *   1:30-2:00: **The Code**. Flash your C++ structs, Interface designs.
*   **The Resume/Portfolio**:
    *   Link to this GitHub repo.
    *   **Crucial**: Include the `docs/design/` folder. Your design docs (`Neural_Utility`, `Advanced_AI`) are high-quality artifacts that show *Product Thinking*.

---

## 3. Killer Features to Highlight (Your "Unique Value")
1.  **The "Veto" System**: Explain how C++ protects the game from LLM hallucinations. This shows reliability.
2.  **Emotional Contagion**: If you implemented the "Panic Spreading" (Monster Invasion), show it. Crowd simulation is huge.
3.  **Data-Driven Design**: Show your `MentalStateFields.h` macro or `DT_UtilityActions`. Show that your system is *designer-friendly*.

## 4. Key Advice for the Interview/Application
*   **Be Honest about Latency**: Don't pretend LLMs are instant. Highlight your *solutions* (Async architecture, System 1/2 split).
*   **Focus on "Fun"**: Always tie the tech back to Gameplay. "I made this AI so players can have unique stories," not just "I like Transformers."
