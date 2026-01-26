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

## [2026-01-19] Phase 3: Player Interaction & Modular Identity Refactor
**Type**: feat/refactor
**Scope**: Sensory, Cognition, NPCDefinition, UI, PlayerController

**Description**:
- **Dialogue Action**: Implemented `Action_TalkTo` utility action for LLM-driven conversations.
- **Social Input**: Created `UPlayerDialogueWidget` and integrated chat input logic into `AINPCPlayerController`.
- **Speech Perception**: Enhanced `SensoryComponent::ReceiveSpeech` to generate semantic events and trigger visual speech bubbles.
- **Identity Refactor**: Renamed Backstory to `SocialProfileDef` and refactored `CognitionComponent` to use modular prompt assembly (Name/Event/Profile).
- **Optimization**: Implemented Prompt LOD system to optimize context window usage.

**Files Changed**:
- `Source/AINPC/Public/Actions/Action_TalkTo.h/.cpp`
- `Source/AINPC/Public/UI/PlayerDialogueWidget.h/.cpp`
- `Source/AINPC/AINPCPlayerController.h/.cpp`
- `Source/AINPC/Components/SensoryComponent.cpp`
- `Source/AINPC/Components/CognitionComponent.cpp`
- `Source/AINPC/Components/NPCDefinitionComponent.h/.cpp`
- `Source/AINPC/Public/UtilityAI/BackstoryConfig.h` (Renamed struct to SocialProfile)
- `Content/AINPC/Datatable/DT_Names.json` (New)
- `Content/AINPC/Datatable/DT_PastEvents.json` (New)
- `Content/AINPC/Datatable/DT_SocialProfiles.json` (Replaced Backstories)
- `Content/AINPC/Widgets/WBP_playerDialog` (New)

---

## [2026-01-20] AINPC_LOG Debug Macro System & Chat UI Fix
**Type**: feat/fix
**Scope**: Debugging, AINPCPlayerController, Core Components

**Description**:
- **AINPC_LOG Macros**: Created centralized logging macros in `AINPC.h` with automatic class name extraction
  - Uses `__FUNCTION__` to automatically prepend class name (e.g., `[USensoryComponent]`)
  - Variants: `AINPC_LOG`, `AINPC_LOG_VERBOSE`, `AINPC_LOG_WARNING`, `AINPC_LOG_ERROR`
  - All logged to `LogAINPC` category for unified filtering
- **Component Updates**: Migrated 9 core components to use new AINPC_LOG macros:
  - SensoryComponent, UtilityAIComponent, CognitionComponent
  - PersonalityComponent, MemoryComponent, MetabolismComponent
  - GoalComponent, NPCDefinitionComponent, EmotionDisplayComponent
- **Toggle Chat Fix**: Fixed `AINPCPlayerController::ToggleChat()` input mode issue
  - Changed `FInputModeUIOnly` → `FInputModeGameAndUI`
  - Allows player to press T to close chat UI (previously keyboard was blocked)
- **Documentation**: Created `Phase4_AI_System_Enhancement_Plan.md` covering:
  - Memory System Refactor (Decay Model, Resolution Tags, GC)
  - Agency & Soul (Gossip Network, Hidden Agendas)
  - Debug Protocol (Log Channels, JSON Snapshots, Visual Overlay)
  - Prompt Optimization (Layer Architecture, LOD-based Stripping)

**Files Changed**:
- `Source/AINPC/AINPC.h` - Added AINPC_LOG macros
- `Source/AINPC/AINPCPlayerController.cpp` - Fixed input mode
- `Source/AINPC/Components/SensoryComponent.cpp`
- `Source/AINPC/Components/UtilityAIComponent.cpp`
- `Source/AINPC/Components/CognitionComponent.cpp`
- `Source/AINPC/Components/PersonalityComponent.cpp`
- `Source/AINPC/Components/MemoryComponent.cpp`
- `Source/AINPC/Components/MetabolismComponent.cpp`
- `Source/AINPC/Components/GoalComponent.cpp`
- `Source/AINPC/Components/NPCDefinitionComponent.cpp`
- `Source/AINPC/Components/EmotionDisplayComponent.cpp`
- `docs/design/Phase4_AI_System_Enhancement_Plan.md` (New)
- `CHANGELOG.md`

**Impact**:
- Debug logs now automatically show which class emitted them
- Easier to trace issues across component boundaries
- Chat UI toggle works correctly for player dialogue

---

## [2026-01-20] Phase 4 P0 - Log Channels & Nameplate Fix
**Type**: chore/fix
**Scope**: Debugging, UI, EmotionDisplay

**Description**:
- **Log Channels**: Implemented dedicated log categories (`LogAINPCBrain`, `LogAINPCMemory`, `LogAINPCSocial`, `LogAINPCUtility`) in `AINPC.h` to isolate debug traffic.
- **Nameplate Fixes**: 
  - Disabled collision on all EmotionDisplay widget components to prevent blocking NPC navigation. 
  - Implemented robust initialization logic to find `NPCDefinitionComponent` across Pawn/Controller hierarchy.
- **Docs**: Started Phase 4 documentation.

**Files Changed**:
- `Source/AINPC/AINPC.h`
- `Source/AINPC/AINPC.cpp`
- `Source/AINPC/Components/EmotionDisplayComponent.cpp`
- `Source/AINPC/Components/MemoryComponent.cpp`
- `Source/AINPC/Components/UtilityAIComponent.cpp`
- `Docs/design/Phase4_AI_System_Enhancement_Plan.md`

---

## [2026-01-21] Speech Bubble System Redesign
**Type**: feat/refactor
**Scope**: EmotionDisplay, SensoryComponent, Action_TalkTo, UtilityAIController

**Description**:
- **Simplified Speech Gate**: Changed from whitelist (only certain actions) to blacklist (only block during Sleep). NPCs now speak in any context except sleeping.
- **Conversation Mode Flag**: Added `bInConversation` to `UtilityAIController` for tracking active TalkTo conversations.
- **Vision Suppression**: During conversation, suppress player vision events (prevents "seen player 3 times" noise). Enemy events still pass through for combat interruption.
- **Auto-Speech Timer**: Added `ConversationTimer` and `AutoSpeakInterval` (8s) to Action_TalkTo. NPCs auto-speak if no player reply.
- **Player Reply Reset**: Player speech resets conversation timer via `ResetConversationTimer()`.
- **Player Interruption**: Player speaking to busy NPC (sleeping/eating) boosts Loneliness +0.5, triggering Social directive.
- **Topic Guidance**: Auto-speak prompts now focus on light small talk (weather, work, local news).
- **Conversation Continuation**: ReceiveSpeech event now instructs LLM to respond naturally to what was just said.

**Files Changed**:
- `Source/AINPC/Controller/UtilityAIController.h` (added bInConversation)
- `Source/AINPC/Components/EmotionDisplayComponent.cpp` (simplified gate)
- `Source/AINPC/Components/SensoryComponent.cpp` (vision suppression, timer reset, topic guidance)
- `Source/AINPC/Public/Actions/Action_TalkTo.h` (timer members)
- `Source/AINPC/Private/Actions/Action_TalkTo.cpp` (auto-speak logic, flag management)

---

## [2026-01-21] Phase 6 & 7 - LLM Latency Optimization & Death Handling Fixes
**Type**: perf/fix
**Scope**: CognitionComponent, SensoryComponent, CombatEnemy
**Description**:
- **Amygdala Hijack**: Implemented immediate threat response in `CognitionComponent`.
  - Spikes `Perceived_Threat` to 0.9 instantly if `HOSTILE`/`DANGER` detected in input.
  - Bypasses LLM latency for combat initialization.
- **Request Throttling**: Added Rate Limiting to `ProcessStimulus`.
  - High Priority (Danger) Cooldown: 1.5s.
  - Normal Priority Cooldown: 4.0s.
  - Prevents LLM request spam from repetitive perception events.
- **Death Event Perception Fixes**:
  - Removed `ShouldPerceiveTarget` cooldown check for death events to ensure they are always processed.
  - Implemented `GetSmartActorName` to resolve memories using both DisplayName and ObjectName.
  - Fixed issue where "Hostile" memories were not cleared upon enemy death due to name mismatch.
- **Hostile Memory Cleanup**:
  - Added logic in `MetabolismComponent` to clear Hostile memories when Threat decays (0.4->0.1).
- **Delegate Refactor**: `ACombatEnemy::OnEnemyDied` now passes DeadEnemy and Killer for precise event handling.

**Files Changed**:
- `Source/AINPC/Components/CognitionComponent.h/.cpp`
- `Source/AINPC/Components/SensoryComponent.h/.cpp`
- `Source/AINPC/Components/MemoryComponent.h/.cpp`
- `Source/AINPC/Components/MetabolismComponent.h/.cpp`
- `Source/AINPC/Variant_Combat/AI/CombatEnemy.h/.cpp`

## [2026-01-21] Fixes - PAM, Threat Architecture & Self-Interruption
**Type**: fix/refactor
**Scope**: UtilityAI, Sensory
**Description**:
- **Fixed PAM**: Resolved `PersonalityComponent` lookup failure by checking Controller. Corrected `Brave` vs `Cautious` Neuroticism values.
- **Threat Architecture**: Implemented "Hybrid Control" for `Perceived_Threat`. Engine now strictly resets Threat to 0 when a **Hostile** enemy dies, preventing "scared of corpse" bugs.
- **Cooldown Fix**: Modified `UtilityActionBase` to bypass Cooldown check if the action is currently running (preventing self-interruption).
- **Docs**: Added `docs/design/Action_Transition_System_Plan.md` outlining future Refactor for centralized transition logic.

## [2026-01-22] Documentation Localization & License Update
**Type**: docs
**Scope**: Documentation
**Description**:
- **Localization**: Translated README.md to English.
- **License**: Updated license to Limited License in README and badge.
- **Docs**: Synchronized README Latest Updates with Changelog.


## [2026-01-23] Future Plans & Roadmap Update
**Type**: docs
**Scope**: Documentation
**Description**:
- **Roadmap**: Updated README.md with Phase 5 'Emergent Gameplay' plans.
  - Added 'The Saboteur' mechanics (sabotage Smart Objects).
  - Added 'Disaster Events' (monster invasion, chaos simulation).
  - Added 'Social Engineering' (gossip, faction manipulation).


## [2026-01-23] Readme Refresh - Core Features
**Type**: docs
**Scope**: Documentation
**Description**:
- **Readme**: Rewrote 'Core Features' section to reflect current architecture.
  - Added 'OCEAN Personality Engine' (Big Five Model).
  - Added 'Maslow's Hierarchy of Needs' (Physiological, Safety, Social, Self-Actualization).
  - Added 'Hybrid Architecture V3' (LLM Intention + Utility Execution).
  - Added 'Smart Object Ecosystem' (Semantic Tagging, Offsets).
  - Removed outdated technical details from the top level (Single-Point Config, Concurrent Mgmt).


## [2026-01-23] Readme Update - Full Feature Set
**Type**: docs
**Scope**: Documentation
**Description**:
- **Readme**: Added comprehensive feature descriptions.
  - **Narrative & Identity**: Soul Injection, Trauma System, Modular Backstories.
  - **Optimization**: Context LOD, Request Throttling, Amygdala Hijack.
  - **Living Schedule**: Time Manager, Professional Shifts, Work Fulfillment.
  - **GOAP-Lite**: Goal Arbitration, Contextual LOD, Emergency Overrides.


## [2026-01-23] Readme Maintenance - TOC Update
**Type**: docs
**Scope**: Documentation
**Description**:
- **TOC**: Updated Table of contents to match renamed sections.
  - Linked 'Future Plans & Roadmap'.
  - Ensured navigation anchors are correct.


## [2026-01-23] Readme Update - Technical Roadmap Links
**Type**: docs
**Scope**: Documentation
**Description**:
- **Roadmap**: Added links to detailed design documents in Technical Roadmap.
  - Linked 'Phase 4: Optimization & Agency' plan.
  - Linked 'Action Transition System 2.0' plan.
  - Removed duplicate header caused by encoding issues.


## [2026-01-23] Readme Update - Phase 5 Alignment
**Type**: docs
**Scope**: Documentation
**Description**:
- **Roadmap**: Aligned Phase 5 plans with 'Advanced_AI_Features.md'.
  - Added 'The Soul Layer': Lore & Context injection.
  - Added 'The Conflict Layer': Internal decision conflicts.
  - Added 'Self-Evolution': Nightly reflection & dynamic value adjustment.
  - Added 'Emergent Behavior': Bard & Stage mechanism.


## [2026-01-23] Readme Update - AI Roadmap Expansion
**Type**: docs
**Scope**: Documentation
**Description**:
- **Roadmap**: Added Phase 7 (RLHF), Phase 8 (Director), **Phase 9 (Local Distillation)**, Phase 10 (Self-Calibration), and **Phase X (Neural Utility)** to `README.md`.
- **Sync**: Fully aligned `README.md` with `Advanced_AI_Features.md`.


## [2026-01-23] Design - Neural Utility Architecture
**Type**: docs
**Scope**: Documentation
**Description**:
- **Design**: Created `docs/design/Neural_Utility_Architecture.md`.
  - Defined "System 1" (Vector Utility) and "System 2" (Local LLM) architecture.
  - Added "Appendix A: The Math" (Dot Product Logic).
  - Added "Appendix B: TinyStories" (Small Model Strategy).
  - Added "Appendix C: Social Memory" (Generative Agents Insight).
- **Roadmap**: Added "Phase X: Neural Utility Architecture" to `README.md` as the long-term North Star.
