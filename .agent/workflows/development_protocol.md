---
description: Protocol for Development Lifecycle (Plan -> Execute -> Feedback -> Resolve)
---

# Development Protocol

Follow this protocol for every development cycle.

## Phase 1: Planning 🧠
1.  **Analyze Request**: Understand the user's objective.
2.  **Break Down Tasks**:
    *   Initialize/Update `task.md` with granular tasks.
    *   Call `task_boundary` with `Mode: PLANNING`.
3.  **Create/Update Plan**:
    *   Create `implementation_plan.md` detailing changes.
    *   **Request Review**: Use `notify_user` to get approval before writing code.

## Phase 2: Execution ⚙️
1.  **Enter Execution Mode**: Call `task_boundary` with `Mode: EXECUTION`.
2.  **Implement**: Write code, run commands, create files.
3.  **Update Task List**: Mark items as `[x]` in `task.md` as you go.

## Phase 3: Feedback Loop 🗣️
*   **If User Feedback is POSITIVE**:
    *   Proceed to **/maintenance_protocol** to finalize docs and logs.
*   **If User Feedback is NEGATIVE (Bug/Issue Reported)**:
    1.  Open [docs/BUG_TRACKER.md](file:///d:/CombatDemos/AINPC/docs/BUG_TRACKER.md).
    2.  Log the issue under **Active Issues**.
        *   Format:
            ```markdown
            ### [Bug Name]
            **Reported**: [Date]
            **Description**: [User's feedback]
            ```
    3.  Return to **Phase 1 (Planning)** to fix the bug.

## Phase 4: Resolution ✅
1.  Once the fix is implemented and verified:
2.  Open [docs/BUG_TRACKER.md](file:///d:/CombatDemos/AINPC/docs/BUG_TRACKER.md).
3.  Move the issue to **Resolved Issues**.
    *   Add `**Resolved**: [Date]` and `**Solution**: [Brief fix description]`.
4.  Proceed to **/maintenance_protocol**.
