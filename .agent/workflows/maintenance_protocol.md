---
description: Protocol for updating documentation, changelogs, and git logs after coding tasks.
---

# Documentation Maintenance Protocol

Run this workflow after completing *every* significant coding task or bug fix.

## 1. Update Changelog
[ ] Open [CHANGELOG.md](file:///d:/CombatDemos/AINPC/CHANGELOG.md).
[ ] Add a new entry under the "Unreleased" or current version section.
[ ] Format: `- Type: Description (Impact)`.
    *   Types: `Fixed`, `Added`, `Changed`, `Removed`, `Docs`.

## 2. Update Commit Log
[ ] Open [GIT_COMMIT_MESSAGES.md](file:///d:/CombatDemos/AINPC/GIT_COMMIT_MESSAGES.md).
[ ] Append a new commit message entry at the bottom using the standard template:
    ```markdown
    ## [YYYY-MM-DD] Short Title
    **Type**: feat/fix/docs/refactor/chore
    **Scope**: ComponentName (e.g., Metabolism, Sensory)
    **Description**:
    - Detail 1
    - Detail 2
    ```

## 3. Check Documentation Index
[ ] If you created NEW documentation files:
    [ ] Open [docs/README.md](file:///d:/CombatDemos/AINPC/docs/README.md).
    [ ] Add the new file to the appropriate section in the index.
    [ ] Verify links are relative and correct.

## 4. Updates to Root README
[ ] If the change affects high-level features (e.g., new major system, new requirements):
    [ ] Open [README.md](file:///d:/CombatDemos/AINPC/README.md).
    [ ] Update the "Features" or "Getting Started" section.

## 5. Verification
[ ] Verify all modified markdown files render correctly (links work, formatting is clean).
