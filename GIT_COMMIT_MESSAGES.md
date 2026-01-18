# Git Commit Messages Log

This file contains a log of commit messages for the AINPC project.

---

## [2026-01-18] Work System & Maslow's Hierarchy Directive Implementation
**Type**: feat/fix
**Scope**: GoalComponent, UtilityActionBase, Action_SmartObject, Metabolism, TimeManager

**Description**:
- **Directive System Upgrade**: Changed from binary switch to 2.0x bonus multiplier system
  - Matching directive grants 2.0x score bonus (encouragement)
  - Mismatching directive sets score to 0.0 (forbidden)
  - Removed `GoalDirectiveMatch` consideration (moved to code-level multiplier)
- **Maslow's Hierarchy Implementation**: Complete priority system in `GoalComponent::UpdateArbitration()`
  - Priority 1: Survival (Hunger/Fatigue > 0.6, Threat > 0.5)
  - Priority 2: Social (Loneliness > 0.5 + HasFriendlyNearby)
  - Priority 3: Schedule (Work/Idle based on time)
- **Social Directive**: NPCs now proactively seek social interaction when lonely
  - Uses proximity detection via `SensoryComponent::GetPerceivedActors()`
  - Interrupts work if lonely and friends nearby
- **Work Action System**: Implemented Boredom reduction during Work
  - Work actions decrease Boredom via SmartObject `RestoreValue`
  - Creates fulfillment loop (Boredom → Work → Reduced Boredom)
- **Boredom Reading Fix**: Changed from `Interpolator->GetTargetValue()` to `State->Boredom`
  - Boredom is engine-managed (passive growth), not LLM-controlled
- **Emergency Exit**: Critical needs (Hunger/Fatigue/Threat) override ActionDuration
  - Allows NPCs to interrupt work for survival
- **TimeManager Activation**: Added `TickTimeManager()` in GameMode to enable time flow
- **GoalComponent Initialization**: 
  - Added lazy initialization to handle delayed MentalState acquisition
  - Initial directive setup in BeginPlay to avoid 'None' state
- **Idle Fallback**: Changed Idle's DirectiveTag to None (universal availability)
- **Metabolism Tuning**:
  - CriticalHungerThreshold: 0.9 → 0.6 (earlier Survival trigger)
  - HungerRate: 0.01/s → 0.002/s (5× slower, ~5 min to 0.6)
  - EnergyRate: 0.008/s → 0.0016/s (5× slower, ~6.25 min to 0.6)

**Files Changed**:
- `Source/AINPC/Private/Base/UtilityActionBase.cpp` - Directive multiplier, Boredom fix
- `Source/AINPC/Components/GoalComponent.cpp` - Maslow hierarchy, lazy init, Social layer
- `Source/AINPC/Components/GoalComponent.h` - CriticalHungerThreshold adjustment
- `Source/AINPC/Components/MetabolismComponent.h` - Hunger/Fatigue rate rebalancing
- `Source/AINPC/Private/Actions/Action_SmartObject.cpp` - Boredom reduction, emergency exit
- `Source/AINPC/AINPCGameMode.cpp` - TimeManager tick activation
- `Content/AINPC/Datatable/DT_UtilityActions.json` - Removed GoalDirectiveMatch, Idle DirectiveTag
- `CHANGELOG.md` - Comprehensive update documentation

**Impact**:
- NPCs now follow complete Maslow's Hierarchy of Needs
- Work behaviors feel natural (work reduces boredom, creates fulfillment)
- Social interactions are proactive (not just schedule-based)
- Emergency survival needs correctly interrupt all activities
- NPCs can work 4-5 minutes before needing to eat/sleep

---


## [2026-01-18] Action Switching & Physiological Priority Fixes
**Type**: fix
**Scope**: UtilityAI, Action_SmartObject, Sensory, Metabolism

**Description**:
- **Talk Action Exit**: Implemented need satisfaction (reducing Loneliness/Boredom) in `Test_TalkTo` to allow natural action exit.
- **Mental State Sync**: Fixed `MentalStateInterpolator` resetting values every frame by syncing target values when modified by actions.
- **SmartObject Detection**: Increased `SensoryComponent` fallback scan radius (30m -> 150m) to ensure beds/food are found even after running far.
- **Physiological Priority**: Increased BaseReward for Eat/Sleep (4.0) to prevent Emotion Penalties (e.g. Curiosity) from overriding survival needs.
- **Animation Fix**: Restored missing `InteractionMontage` assignment in `UtilityAIComponent`.

**Files Changed**:
- `Source/AINPC/Private/Test/TestAction_TalkTo.cpp`
- `Source/AINPC/Components/SensoryComponent.cpp`
- `Source/AINPC/Components/UtilityAIComponent.cpp`
- `Source/AINPC/Private/Base/UtilityActionBase.cpp`
- `CHANGELOG.md`

---

## [2026-01-17] Hybrid V4 - GOAP-Lite Foundation & Subsystems
**Type**: feat
**Scope**: Architecture, Subsystems, Components
**Description**:
- **SmartObjectManager**: Added subsystem for registry and reservation of world objects.
- **TimeManager**: Added subsystem for Day/Night cycle and hourly events.
- **GoalComponent**: Added arbitration layer (Survival vs Social vs Schedule) with LOD integration.
- **NPCDefinitionComponent**: Added unified ID container for NPC profiles.
- **UtilityAI**: Added `RequiredProfessionID` filtering for actions.
- **Refactor**: Profession logic moved to GoalComponent; Schedules now active.

**Files Changed**:
- `Source/AINPC/Public/Subsystems/SmartObjectManager.h`
- `Source/AINPC/Private/Subsystems/SmartObjectManager.cpp`
- `Source/AINPC/Public/Subsystems/TimeManager.h`
- `Source/AINPC/Private/Subsystems/TimeManager.cpp`
- `Source/AINPC/Components/GoalComponent.cpp`
- `Source/AINPC/Components/NPCDefinitionComponent.cpp`
- `Source/AINPC/Components/UtilityAIComponent.cpp`
- `Source/AINPC/Public/Base/UtilityActionBase.h`

---

## [2026-01-15] Utility AI Pipeline Fixes & Death Event Handling
**Type**: fix/feat
**Scope**: EmotionEvaluator, UtilityActionBase, SensoryComponent, CognitionComponent

**Description**:
- **EmotionEvaluator**: Refactored from waterfall priority to utility scoring (`Score = Max(0, (Value - Threshold) * Weight)`)
- **Intention Pipeline**: Fixed `Intention` field not being copied from LLM to MentalState
- **Emotion Parsing**: Fixed LLM emotion parsing using fully qualified enum name
- **Detailed Logging**: Added comprehensive score breakdown in `UtilityActionBase::CalculateScore`
- **Death Perception**: Added `ClearFocus` when target dies, filter dead actors from perception
- **LLM Prompt**: Added strict emotion validation rule

**Files Changed**:
- `Source/AINPC/Private/UtilityAI/EmotionEvaluator.cpp`
- `Source/AINPC/Private/UtilityAI/UNPCMentalState.cpp`
- `Source/AINPC/Private/Base/UtilityActionBase.cpp`
- `Source/AINPC/Components/SensoryComponent.cpp`
- `Source/AINPC/Components/CognitionComponent.cpp`
- `Source/AINPC/Controller/UtilityAIController.cpp`

---

## [2026-01-15] Hybrid V3 Architecture - Deficit Model & Emotion Matrix
**Type**: feat/refactor
**Scope**: Architecture, UtilityAI, Metabolism, Emotions

**Description**:
- **Deficit Model**: Unified mental state variables. `Boredom`/`Loneliness` grow (needs), `Indignity`/`Threat` decay (reactions).
- **Variable Rename**: `Anger` -> `Indignity`, `Curiosity` -> `Boredom`.
- **Emotion Matrix**: Implemented `Score *= Matrix[Emotion][Activity]` logic to filter actions based on emotional state.
- **Intention Override**: Implemented `Score *= 3.0` when LLM `Intention` matches `Action.IntentionTag`, giving LLM strategic control.
- **Cleanup**: Removed obsolete variables (`Energy`, `Trust`) and simplified `UNPCMentalState`.
- **Docs**: Updated `5_Stage_AI_Pipeline_Design.md` to be the source of truth.

**Files Changed**:
- `Source/AINPC/Public/UtilityAI/MentalStateFields.h`
- `Source/AINPC/Public/UtilityAI/EmotionTypes.h`
- `Source/AINPC/Components/MetabolismComponent.cpp`
- `Source/AINPC/Private/UtilityAI/EmotionEvaluator.cpp`
- `Source/AINPC/Private/Base/UtilityActionBase.cpp`
- `Source/AINPC/Public/Base/UtilityActionBase.h`
- `Source/AINPC/Controller/UtilityAIController.h`
- `docs/design/5_Stage_AI_Pipeline_Design.md`

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

---

## [2026-01-13] SmartObject Action & UtilityAI Fixes

**Type**: fix
**Scope**: Action_SmartObject, UtilityAIComponent, LLMCommunicator

**Description**:
- Fixed InertiaBonus applying when base score = 0 (prevented action exit)
- Fixed bIsInteracting not being reset in Exit() (caused stuck states)
- Fixed static LastDebugTime variable causing cross-instance interference
- Added MoveToActor error checking with proper logging
- Increased SmartObject acceptance radius from 50cm to 200cm (NavMesh fix)
- Updated LLM System Prompt to include Emotion, Speech, and Intention fields

**Files Changed**:
- `Source/AINPC/Components/UtilityAIComponent.cpp`
- `Source/AINPC/Private/Actions/Action_SmartObject.cpp`
- `Source/AINPC/Public/Actions/Action_SmartObject.h`
- `Source/AINPC/LLM/LLMCommunicator.cpp`
- `CHANGELOG.md`

## [2026-01-19] Refactor Mental State & Metabolism system for natural emotional decay
**Type**: feat/refactor
**Scope**: Metabolism, MentalStateInterpolation

**Description**:
- **MetabolismComponent**: Implemented Asymmetric State Transition (Spike vs Decay). Instant reaction (10.0f) for threats, natural decay for recovery.
- **MentalStateInterpolator**: Changed to only cache/decay target intentions. No longer writes to state directly.
- **Fix**: Solved conflict preventing Perceived_Threat from decaying naturally.

