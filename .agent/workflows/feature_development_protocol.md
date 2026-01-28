---
description: Protocol for Rigorous Feature Development (List -> Plan -> Design -> Test -> Execute)
---

# Feature Development Protocol

Use this workflow for implementing complex features where architecture, side effects, and strict verification are critical.

## Phase 1: Proposal & Analysis 💡
1.  **Analyze User Request**: Understand the core problem and requirements.
2.  **Feature Listing**:
    *   List key features/components needed.
    *   Identify dependencies and potential risks.
    *   *Artifact*: Brief summary in chat or `task.md`.

## Phase 2: Design & Specification 📐
**CRITICAL**: Do not write code until this phase is approved.

1.  **Create Design Document**:
    *   File: `docs/design/[Feature_Name]_Design.md`
    *   **Architecture**: Component interactions, data flow diagrams (Mermaid).
    *   **Data Structures**: New Enums, Structs, DataTable columns.
    *   **Corner Cases**: How to handle interruptions, invalid states, race conditions.
    *   **Integration**: How it fits with existing systems (Utility AI, Narrative, etc.).
2.  **Verification Plan**:
    *   Define specific **Acceptance Criteria** (e.g., "Boredom must decay by 0.1/s when working").
    *   Designing Test Cases: "Step 1: Spawn NPC. Step 2: Set Time to 8:00. Step 3: Verify Action is 'Work'."

## Phase 3: Implementation Planning 📋
1.  **Task Breakdown**:
    *   Update `task.md` with granular steps based on the Design Document.
    *   Set `task_boundary` to `PLANNING`.
2.  **Implementation Plan**:
    *   Create `implementation_plan.md` mapping design to specific file edits.
    *   **Request User Review**: Ensure the implementation path aligns with the design.

## Phase 4: Execution ⚙️
1.  **Set Mode**: Call `task_boundary` with `Mode: EXECUTION`.
2.  **Iterative Coding**:
    *   Implement strictly according to the Design Document.
    *   If design flaws are found, **STOP**, update Design Doc, notify user, then proceed.
    *   Use `ActiveContextTags` or `AINPC_LOG` features as standard practice.

## Phase 5: Verification & Review 🔍
1.  **Run Verification**:
    *   Execute the Test Cases defined in Phase 2.
    *   Document results in chat (e.g., "Test 1 Passed: NPC ignored player during cutscene").
2.  **Documentation**:
    *   Run `/maintenance_protocol` to update Changelogs and Commit Logs.
    *   Update `docs/README.md` features list.

---
**Guideline**: "Code is easy, systems are hard." Prioritize thinking over typing.
