# Git Commit Messages Log

This file contains a log of commit messages for the AINPC project.

---

## [2026-01-13] Metabolism System Overhaul - Engine Variable Protection

**Type**: fix/refactor
**Scope**: Metabolism, UtilityAI, MentalState

**Description**:
- Fixed LLM overwriting Engine-exclusive variables (Hunger, Fatigue)
- Added skip logic to `UNPCMentalState::UpdateFromStruct()` 
- Added skip logic to `MentalStateInterpolator::SetCurrentValue()`
- Modified `UtilityActionBase::GetConsiderationValue()` to read from State directly
- Renamed `Energy` to `Fatigue` for clearer semantics
- Fixed metabolism direction logic (higher value = more urgent need)
- Removed montage playing check that blocked action scoring
- Fixed action duration auto-exit for SmartObject actions

**Files Changed**:
- `Source/AINPC/Public/UtilityAI/MentalStateFields.h`
- `Source/AINPC/Public/UtilityAI/EngineManagedValues.h`
- `Source/AINPC/Private/UtilityAI/UNPCMentalState.cpp`
- `Source/AINPC/Private/UtilityAI/MentalStateInterpolation.cpp`
- `Source/AINPC/Private/Base/UtilityActionBase.cpp`
- `Source/AINPC/Private/Actions/Action_SmartObject.cpp`
- `Source/AINPC/Components/MetabolismComponent.cpp`
- `CHANGELOG.md`

**Breaking Changes**:
- `Energy` field renamed to `Fatigue` across all code and DataTables
