---
description: Protocol for Code Quality Assurance and Test Design
---

# Code Review & Testing Protocol

Run this protocol after completing a significant coding task to verify quality and define test steps.

## Phase 1: Code Quality Audit (Self-Correction) 🔍

Review the modified files against the following checklist. If issues are found, **fix them immediately** before asking for user review.

### 1. Unreal Engine Standards
*   [ ] **Prefixes**: Ensure classes (`U`, `A`, `F`), interfaces (`I`), and templates (`T`) have correct prefixes.
*   [ ] **Booleans**: Ensure boolean variables start with `b` (e.g., `bIsActive`).
*   [ ] **Const Correctness**: Are functions that don't modify state marked `const`? (e.g., `GetBackstoryDef`).
*   [ ] **Inclusions**: Are headers included in the `.cpp` where possible to minimize compile times? (Forward declare in `.h`).

### 2. Safety & Stability (Critical)
*   [ ] **Validity Checks**: Are pointers checked before access? (`if (Comp) ...`).
*   [ ] **Casting**: Is `Cast<T>` used safely? Are results checked?
*   [ ] **Data Tables**: Are `FindRow` lookups safe? Is the ContextString provided?
*   [ ] **Strings**: Are text formatters (`Printf`) safe matching arguments?

### 3. Logic & Architecture
*   [ ] **State Management**: Are variables initialized in the Constructor?
*   [ ] **Race Conditions**: Are Timers cleared if the component is destroyed or state changes?
*   [ ] **Coupling**: did we introduce circular dependencies?

## Phase 2: Test Case Generation 🧪

Based on the changes, generate a structured list of test scenarios for the USER to verify.

### Output Format (in Chat or Verification Plan)
Generate a markdown table or list:

**Feature**: [Feature Name]
| ID | Scenario | Steps to Reproduce | Expected Result |
|----|----------|-------------------|-----------------|
| T1 | [Name]   | 1. Do X<br>2. Do Y| [Result]        |

### Scenario Categories to Cover:
1.  **Happy Path**: The standard use case (e.g., "Assign Profile, Game Starts, NPC has Correct Name").
2.  **Edge Cases**:
    *   Missing Data (e.g., "Profile ID is None").
    *   Invalid Data (e.g., "Backstory ID not in Table").
    *   Rapid State Changes (e.g., "Stimulus triggered 10 times in 1 second").
3.  **Performance**: Is the log spamming? (Check `Tick`).

## Phase 3: Execution
1.  **Refactor**: Apply fixes found in Phase 1.
2.  **Document**: Add the Test Plan to `implementation_plan.md` or `task.md`.
3.  **Notify**: Present the code and test plan to the user.
