# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.6.6] - 2026-02-01

### 🏗️ Major Architecture - Unified Event Bus
- **New Subsystem**: `EventBusSubsystem` (World Subsystem)
  - **Single Source of Truth**: Centralized routing for all AI/Gameplay events.
  - **Features**: Tag-based filtering, Priority Queues (Critical > Low), Auto-cleanup.
  - **Performance**: Replaces multiple delegate chains with a single optimized subscription model.
- **Documentation**: Added `docs/systems/UnifiedEventBus_Guide.md`.

### ⚡ Refactors - Action Observation & Helpers
- **Action Observation System**: Unified framework for NPCs observing each other.
  - **Components**: Enhanced `AttentionBudgetComponent` to process observations.
  - **Flow**: `SensoryComponent` -> `AttentionBudget` -> `Cognition`.
  - **Documentation**: Added `docs/systems/ActionObservationSystem_Guide.md`.
- **Faction Logic Unification**: Created `FactionHelpers` namespace.
  - **Centralization**: Moved scattered faction logic from `SensoryComponent`/`NarrativeSquad` to `FactionHelpers`.
  - **API**: `AreActorsHostile`, `GetFaction`, `GetReputation`, `GetAttitudeDescription` (String Helper).

### 🐛 Bug Fixes - Narrative & Event System
- **Initialization Race Condition**: Fixed `NarrativeSquadSubsystem` spawning NPCs and allowing them to broadcast events before being registered in the Squad.
  - **Resolution**: Switched to `SpawnActorDeferred` + explicit `ActivateScene` call to ensure suppression flags are set before `BeginPlay`.
- **Direct Sensory Suppression**: Added `bSuppressActionObservation` to `SensoryComponent` as a hard override.
  - Bypasses subsystem dependency to guarantee suppression (e.g., for cutscenes).
- **Event System Refactor**: Unified `FObservedActionEvent` into `FSemanticEvent`.
  - `AttentionBudgetComponent` now processes `FSemanticEvent` natively.
- **Regression Note**: Narrative progression is currently reported as unstable/broken. Cause under investigation (possibly related to `ActivateScene` changes).

### 🐛 Bug Fixes - Combat & Movement
- **Fixed Attack Logic**: Resolved issue where AI would get stuck in "Moving to target" loop.
  - **Collision Range**: Increased default `AttackRange` to 250.0f to account for large collision capsules.
  - **Montage Fallback**: Added failsafe for `PlayAnimMontage` failing (returns 0 duration). Now properly applies instant damage and resets state to prevent freezing.
  - **Montage Loading**: Fixed `LoadActionsFromTable` to correctly assign `InteractionMontage` for `Action_Attack`.

### ⚡ Refactors - Logging
- **Unified Logging**: Cleaned up `UtilityActionBase` logs.
  - Replaced ad-hoc `LogTemp` with `LogAINPCUtility` for consistent filtering.

### 🔧 Fixes - Narrative System
- **Event Matching**: Fixed `NarrativeSquadSubsystem` not recognizing squad member deaths.
  - Replaced raw `GetName()` check with `GetSmartActorName()` to match event payloads correctly.

### ✨ Features - Sensory System
- **Action Awareness**: Updated `SensoryComponent` to include the observed actor's current action (e.g., "Mining", "Attacking") in the sensory description string.
  - Provides LLM with crucial context about what others are doing.

### 🐛 Bug Fixes - AI Action Transitions
- **Fixed Attack Action Stalling**: Resolved critical bug where `Action_Attack` would remain active even after target died.
  - **Root Cause**: `Execute` detected invalid/dead target but only returned early without clearing `TargetActor` or `FocusActor`.
  - **Fix**: Now clears both `TargetActor` and `Controller->ClearFocus()` when target is invalid/dead.
  - **Impact**: `HasAttackTarget` correctly returns 0, triggering action transition to Idle/Eat.

- **Fixed Priority Lock Preventing Exit**: High-priority actions (Combat=50) blocked lower-priority actions (Idle=10) even when their score was near 0.
  - **Rule 0 Added**: Actions with score ≤0.1 now immediately yield regardless of priority.
  - **Rule 1 Exception**: Priority shield is broken if current action's score <0.25, allowing lower-priority actions to interrupt.
  - **Impact**: NPCs no longer get "soft-locked" in invalid combat states.

- **Fixed Idle Action Navigation Retries**: `TestAction_Idle` could stall if `MoveToLocation` failed.
  - **Fix**: Added return value check and faster retry (0.5s) on navigation failure.

### 🧹 Cleanup - Status.InScene Tags
- **EndScene Tag Removal**: Enhanced `NarrativeSquadSubsystem::EndScene` to robustly remove `Status.InScene` tags using explicit `FName`.
  - Added logging to confirm tag removal.
  - Prevents NPC behavior anomalies after scene completion.

### 🔇 Log Verbosity Adjustments
- **TargetSelectionSubsystem**: Reduced `GetTargetCandidates` logs from Warning to Verbose.
- **HandleDeath**: Added confirmation logs for Dead tag application.

### 🐛 Bug Fixes - AI Perception
- **Fixed HasEnemyNearby with Dead Actors**: Resolved critical bug where `HasEnemyNearby` returned `1` for dead actors.

### 🔥 Feature - Narrative System Overhaul
- **Scene Completion & Event Logic (Critical Fixes)**:
  - **Tag Mismatch**: Fixed `NarrativeDirector` broadcasting hardcoded `"Death"` string vs expected `Event.Death` GameplayTag.
  - **Event Source**: Fixed `CombatEnemy::HandleDeath` failing to report death events to the Narrative Director.
  - **Identity Resolution**: Replaced `GetActorLabel` (Editor-only/Unstable) with `GetName` (Runtime ID) for checking actor roles.
  - **Pending Kill Safety**: Moved `CleanupInvalidActors` to post-event processing. Previously, actors killed in the same frame were removed from the Squad *before* their death event could trigger scene progression.
  - **Directive Cleanup**: `EndScene` now explicitly removes `Directive.Combat` override tags, preventing NPCs from getting stuck in combat mode after a scene ends.

- **Gameplay & Immersion**:
  - **Persisted Props**: Implemented `bKeepPropsOnEnd` in Scene Defs. Props (like corpses) now remain in the world after the scene finishes instead of vanishing.
  - **Rule by Decree**: Added `PostSceneStimulus`. Scenes can now broadcast a final context update (e.g., "You are free from slavery!") to surviving NPCs, overriding default routines.
  - **Faction Logic Fix**: Refactored `AreActorsHostile` to use the Data-Driven `FactionSubsystem` before falling back to legacy checks.
    - **Issue**: Previously, Orcs attacked Players by default because Legacy Logic assumed `Human != Monster` is always Hostile.
    - **Fix**: Now queries the Global Faction Matrix. If no relationship is defined (e.g. Orcs <-> Player), it defaults to **Neutral**, respecting the intended reputation system.

- **Lifecycle Management (Job Promotion)**:
  - **Mechanic**: Added `PostSceneProfessionID` to `FNarrativeSceneDef`.
  - **Feature**: Scenes can now fundamentally change an NPC's life path. Upon scene completion, NPCs can be "Promoted" to a new Profession (e.g., Slave -> Citizen).
  - **Content**: Updated `Scene_OrcRescue` so freed Orcs become `Citizen` (Standard Schedule) instead of staying `Slave` (Mining Schedule).

  - **End Scene Directive**: Changed post-scene directive from `Directive.Social` to `Directive.Idle` to prevent forced celebration logic.

### 🏗️ Refactor - CalculateScore Function (SRP Compliance)
- **Before**: Monolithic 516-line function with 8+ mixed responsibilities.
- **After**: Clean 90-line orchestrator calling 6 focused helper functions:
  - `CheckCooldown()`, `CalculateConsiderations()`, `CalculateIntentionBonus()`
  - `CalculateScheduleBonus()`, `ApplyPersonalityModifier()`, `ApplyEmotionMatrix()`, `ApplyDirectiveModifier()`
- **Benefit**: Each function handles one concern, making debugging and maintenance much easier.


### 🐛 Fix - TargetSelectionSubsystem Cache Bug
- **Bug**: `FTargetCacheKey::Timestamp` was ignored in `operator==`, causing incorrect cache behavior.
- **Fix**: Separated into `FTargetCacheKey` (lookup) and `FTargetCacheEntry` (data + timestamp).

### 🏗️ Refactor - Unified Target Management
- **Single Source of Truth**: Removed `TargetActor` local variable from `Action_Attack`. Now relies entirely on `Controller->GetFocusActor()` and `TargetSelectionSubsystem`.
- **Target Recovery**: `Action_Attack::Execute` now attempts to recover target via Subsystem if Focus is lost but Action is still active.
- **Invalidation Handling**: Added `OnTargetInvalidated` event to TargetSelectionSubsystem. Actions subscribe to this to immediately abort when targets die or vanish.

### 🧠 Feature - Brain-Body Connection (Goal x Cognition)
- **Intention Override**: `GoalComponent` was completely ignoring the Brain's decisions.
  - **Fix**: Added high-priority logic in `UpdateArbitration` to check `MentalState->Intention`.
  - **Result**: If the LLM decides to "Celebrate" (Intention.Social) upon freedom, the Goal System immediately prioritizes `Directive.Social` over the default Schedule (which would have forced mining work).
  - **Compilation Fix**: Resolved private member access error (`Cognition->CurrentIntention` -> `MentalState->Intention`).

### 🔧 System Refactors
- **Unified Logging**: Converted all scattered `UE_LOG(LogTemp)` in Narrative subsystems to `NARRATIVE_LOG` for clean filtering.
- **Compilation Stability**: 
  - Fixed missing include `NarrativeDirectorSubsystem.h` in `CombatEnemy.cpp`.
  - Fixed `FNativeGameplayTag` struct access syntax (`GetTag().GetTagName()`).

## [Unreleased] - 2026-01-29

### 🐛 Major Fixes - Combat & Target Selection
- **Fixed Silent Target Selection Failure**: Identified and fixed critical architecture flaw where Guards (`BP_NPC_Human_Base`) and Player (`BP_ThirdPersonCharacter`) lacked `FactionReputationComponent`.
  - **Resolution**: Hard-coded the component into C++ base classes `AAINPCCharacter` and `ACombatEnemy`.
  - **Impact**: All characters now correctly process Faction Attitudes (Hostile/Friendly) instead of default Neutral behavior.
  - **Fixed Faction Initialization**: Updated `NPCDefinitionComponent` to actively push `FactionID` to `FactionReputationComponent`. Previously, the component existed but remained Neutral even for hostility-configured NPCs.

- **Fixed Narrative Combat Visibility**: Solved issue where plot-relevant actors couldn't attack each other if perceived line-of-sight was blocked.
  - **Narrative Target Injection**: Implemented `GetSquadMembers` in `NarrativeSquadSubsystem` and integrated it into `TargetSelectionSubsystem`.
  - **Shared Awareness**: Narrative Squad members now share "Omniscient" awareness of each other, ensuring combat triggers regardless of physical perception.

### 🐛 Fixes - Narrative System
- **Fixed Death Event Logging**: NPCs now report death events using their Narrative Name (e.g., "Grommash") instead of internal Personality IDs (e.g., "Paranoid").
  - Updated `AINPCHelpers::GetSmartActorName` to prioritize `NPCDefinitionComponent::GetDisplayName()`.
- **Fixed Timeline Configuration**: Corrected `DT_NarrativeScenes` Node 2 using `Directive.Social` instead of `Directive.Combat`.
## [Unreleased] - 2026-02-02

### 📖 Documentation - Neural Utility
- **Design Clarification**: Added `Neural_Utility_Architecture.md` to the documentation index.
- **Implementation Strategy**: Clarified "Stage 1" approach: use Manual Vector Definition (Explicit Dimensions) for MVP before moving to full Embedding models.

## [Unreleased] - 2026-01-29

### ✨ Features - Universal Target Selection
- **New Subsystem**: Implemented `TargetSelectionSubsystem` to centralize target acquisition.
  - **Hybrid Selection Strategy**: Combines Rule-Based Scoring (Fast, Combat) with Async LLM Selection (Slow, Narrative).
  - **Caching System**: Reduces CPU/LLM load by caching valid targets for 5s (configurable).
  - **LLM Fallback**: Automatically falls back to Rule-Based if LLM is too slow or fails.
- **Cognition Integration**: Added `SuggestTarget(Candidates, Context)` to `CognitionComponent`.
  - Generates context-aware prompts ("Who should I attack?", "Who should I help?").
  - Includes Memory (Revenge/Gratitude) and Social Context in the decision process.
- **Action Configuration**: Updated `UtilityActionBase` with Data-Driven Target Config.
  - `bNeedsTarget`: Switch to enable/disable targeting.
  - `TargetContext`: Combat, Social, Trade, Follow.
  - `TargetConfigOverride`: Per-action customization of distance and selection rules.

### 🔧 Fixes & Refactors
- **Action_Attack Refactor**: Migrated `Action_Attack` to use the new `TargetSelectionSubsystem`, removing ad-hoc `GetFocusActor` dependencies (retained as fallback).
- **Compilation Fixes**: 
  - Fixed `UHT` error in `TargetSelectionSubsystem` (Config=Game).
  - Fixed `FMemoryEntry` type mismatch (migrated to `FMemoryItem` from SocialTypes).
  - Added `GetSmartActorName` helper to `AINPCHelpers`.

### ✨ Features - Narrative Evolution System
- **Timeline Architecture**: Implemented `FNarrativeTimelineEntry` and sequence logic in `NarrativeSquadSubsystem`.
  - Supports phased story beats (e.g., Intro -> Action -> Climax) driven by a Director.
  - **Hybrid Triggers**: Beats can be triggered by Time (Delay) OR Events (Gameplay Tags).
- **Event Broadcasting**: Implemented `Event.PlayerDetected` and `BroadcastEvent` API.
  - Allows world interactions (Trigger Volumes) to advance the narrative timeline.
- **Prompt Optimization**: Refactored `CognitionComponent` prompt assembly order.
  - Context/Plot now injected *after* Identity to ensure scene instructions override base behavior.

### 🔧 Logging & Debugging
- **NARRATIVE_LOG**: Added specialized log category `LogAINPCNarrative` for clean story debugging.

## [Unreleased] - 2026-01-29

### 🔧 Fixes - Prompt Caching \& Cognition System
- **Fixed Neutral Faction Blocking**: Modified `CognitionComponent::IsDataReady()` to allow `Faction="Neutral"` for civilian NPCs.
  - Previously blocked all NPCs with Neutral faction from sending LLM requests (infinite retry loops).
  - Now only blocks `Faction="None"` or empty strings, treating "Neutral" as valid.
  - Impact: Narrative Squad NPCs (civilians, slaves, guards) can now speak properly.
- **Refactored Amygdala Hijack to Use Faction System**: Replaced unreliable string scanning (`"HOSTILE"` keyword) with proper Faction Attitude queries.
  - `CheckAmygdalaHijack()` now queries `AIController->GetFocusActor()` and checks `FactionSubsystem->GetBaseAttitude()`.
  - Only triggers threat response when Reputation ≤ 25.0 (truly hostile actors).
  - Eliminates false positives from narrative scene descriptions (e.g., "rescue from HOSTILE territory").
  - Ensures threat detection is based on actual enemy presence, not text content.

### 🔧 Refactors - Code Quality
- **ProcessStimulus Refactoring**: Extracted helper methods from 400+ line monolithic function:
  - `CheckAmygdalaHijack()`: Immediate threat response logic.
  - `IsDataReady()`: Data validation and retry scheduling.
  - `BuildIdentityBlock()`: Static identity and backstory assembly.
  - `BuildWorldviewBlock()`: Faction relationship descriptions.
  - `BuildContextBlock()`: Job and narrative plot context.
  - `BuildVolatileBlock()`: Dynamic situation, memories, and global history.
  - Main `ProcessStimulus` reduced to ~50 lines with clear execution flow.

## [Unreleased] - 2026-01-29

### ✨ Features - Narrative & Utility AI Integration
- **Narrative Suppression**: Implemented mechanism in `GoalComponent` to block Social needs (e.g., Loneliness) when NPC is in a Narrative Scene (`AINPCTags::Status_InScene`), ensuring focus on roles.
- **Enhanced Role Adherence**: Tripled `DirectiveMultiplier` (x1.5 -> x3.0) for actions matching directives when in a narrative scene.
- **LLM Agency Boost**: Increased `IntentionBonus` to 2.5f, empowering LLM-driven intentions to override basic utility scores.

### 🔧 Refactors - Code Quality
- **Gameplay Tags Migration**: Replaced all string-based "Status.InScene" tag usage with Native Gameplay Tag `AINPCTags::Status_InScene` and `GoalComponent` context management functions.
- **Goal Component API**: Added `ActiveContextTags` container and `Add/Remove/HasContextTag` API to `GoalComponent`.

### ⚡ Refactors - P0 Stability (Optimization)
- **Externalized Settings**: Created `UAINPCSettings` (Project Settings -> Plugins -> AINPC).
  - Extracted 8+ magic numbers (utility weights, needs thresholds) into configurable assets.
  - Allows tuning logic without recompilation.
- **Shared Helpers**: Created `AINPCHelpers` namespace to replace duplicated static actor lookups.
  - Unified logic for finding components across Pawn/Controller hierarchy.
  - Refactored `NarrativeSquadSubsystem` to use these helpers.

### 🔧 Fixes
- **Ambient Dialogue Context**: Fixed `NarrativeSquadSubsystem` to use the currently executing action from `UtilityAIComponent` instead of the potentially stale scheduled activity.
- **Narrative Scene Data**: Updated `DT_NarrativeScenes` PlotOutline for "Scene_OrcRescue" to accurately reflect the mining camp scenario.
- **Include Fix**: Added missing `NarrativeSceneAnchor.h` include to `NarrativeSquadSubsystem.cpp`.

## [Unreleased] - 2026-01-28

### ✨ Features - Narrative & Squad System Architecture
- **New Narrative Core**: Implemented a multi-tier subsystem architecture to handle dynamic storytelling:
  - `NarrativeDirectorSubsystem`: Orchestrates global story beats and event recording.
  - `NarrativeSquadSubsystem`: Manages local NPC groups, their roles, and plot contexts.
  - `PlayerSquadSubsystem`: Reserved for managing companion/mercenary groups.
- **Narrative Scene Anchors**: Created `ANarrativeSceneAnchor` and `UNarrativeCompanion` classes to define spatial context for stories.
- **Social Tag Expansion**: Updated `SocialGameplayTags` to include specific tags for narrative states (`Status.InScene`, etc.), allowing for easier behavior filtering.

### 🔧 Fixes & Refinement - NPC Components
- **Cognitive Fidelity**: Updated `CognitionComponent` and `NPCDefinitionComponent` to ensure consistent faction/identity parsing during narrative scenes.
- **Reputation Persistence**: Modified `FactionReputationComponent` to respect narrative-driven relationship overrides.
- **Targeted Communications**: Refactored `AINPCPlayerController` and `SensoryComponent` to implement the new targeted cone-based chat system.
- **Action Robustness**: Updated `UtilityActionBase` and `Action_SmartObject` with improved exit conditions and animation looping fallbacks.

### ⚙️ System & Config
- **LLM Configuration Update**:
  - Migrated `DefaultGame.ini` to use the `deepseek-chat` model with optimized API endpoints.
- **New Project Documentation**:
  - Created `docs/design/Example_Scenario_OrcRescue.md` and `docs/planning/Sprint_Jan28_Feb9.md`.

## [Unreleased] - 2026-01-27

### � Fixes - NPC Identity \& Faction System
- **Fixed Faction Identity Mismatch**: NPCs now correctly identify their race/faction in dialogue.
  - Updated `CognitionComponent::ProcessStimulus` to prioritize Pawn's `NPCDefinitionComponent` over Controller's when retrieving FactionID.
  - Added auto-sync logic in `FactionReputationComponent::BeginPlay` to pull FactionID from Pawn's NPCDefinitionComponent if default.
  - Added detailed Faction descriptions to `DT_Factions.json` that guide LLM roleplay (speech patterns, racial traits, attitudes).
  
- **Refactored Personality DataTable**: Removed job/faction references from `DT_Personalities.json` to prevent AI hallucinations.
  - Personalities now only describe pure personality traits (Brave, Cautious, Friendly, Cunning, etc.) without job titles or racial identities.
  - Added 7 new personality types: Cunning, Cheerful, Grumpy, Ambitious, Lazy, Paranoid, Compassionate.
  - Faction and Profession information now exclusively come from their respective DataTables, preventing conflicting prompts.

### �💄 UI & Feedback
- **Faction Display**: Updated `EmotionDisplayComponent` to show Faction Name in the overhead nameplate (e.g., `[Monsters] [Zombie] Walker`).
- **Nameplate Update**: Now accepts FactionID as an argument and formats the display string efficiently.

### 🔧 Fixes - Zombie Behavior
- **Re-enabled Reasoning for Monsters**: Commented out the forced `bEnableReasoning = false` for Monsters.
  - Allows Zombies to process "Talk" stimuli and generate speech (e.g., Grunts, Hisses) instead of being mute.
  - Relying on LLM Prompt "Limitation" sections (Brain Rot) to enforce mindless behavior.

### 🔧 Fixes - Faction System
- **Plural Faction ID Support**: Updated `SensoryComponent` to support plural FactionIDs (Humans, Orcs, Elves) to match `DT_Factions` row names.


## [Unreleased] - 2026-01-26

### ✨ Features - Faction & NPC Identity System
- **Unified NPC Definition**: `NPCDefinitionComponent` now acts as the central "Passport".
  - **Template Support**: Added `DefinitionTemplateID` to auto-load configs from `DT_NPCDefinitions` (e.g., "Orc_Warrior").
  - **Auto-Randomization**: If Name/Backstory/Trauma are not set, `RandomizeModularIdentity()` automatically picks diverse options from DataTables.
- **Rich Content Generation**:
  - `DT_Names.json`: 50 diverse fantasy names.
  - `DT_SocialProfiles.json`: 50 unique backgrounds (Social Class + Values + Motivation).
  - `DT_PastEvents.json`: 50 life events generating specific Phobias and Mental Scars.
- **Faction System 2.0**:
  - **Data-Driven Hostility**: `DT_Factions` defines relationships (War/Neutral/Alliance).
  - **Two-Layer Reputation**: `FactionSubsystem` (Global) + `FactionReputationComponent` (Personal Overrides).
  - **Triangular Conflict**: Humans vs Orcs vs Elves (all hostile to Monsters).

### ✨ Features - Smart Object Slot System
- **Multi-Slot Support**: Smart Objects can now support multiple users simultaneously.
  - Added `SlotOffsets` array to `SmartObjectComponent` for configuring multiple interaction points.
  - Editor-draggable offsets via `MakeEditWidget` meta.
  - Runtime debug arrows visualize slot positions (Editor/PIE only).
- **Slot Reservation**: NPCs now reserve individual slots instead of entire objects.
  - `TryReserveSlot()` / `ReleaseSlot()` API on `SmartObjectComponent`.
  - `SmartObjectManager::FindBestSmartObject()` checks `HasAvailableSlot()` before returning.
- **Auto-Registration**: Smart Objects automatically register/unregister with `SmartObjectManager` on BeginPlay/EndPlay.

### 🔧 Fixes - Navigation & Pathfinding
- **NavMesh Projection**: Added `ProjectToNavMesh()` helper to project underground targets to walkable surfaces.
- **Improved Retry Logic**: Progressive retry with increasing acceptance radius (150cm → 300cm).
  - Fallback to `MoveToActor` if `MoveToLocation` returns Failed.
  - Force-start interaction if NPC is within 500cm after 5 retries.
- **Z-Coordinate Fix**: Slot locations now use Actor's Z instead of transformed Z (prevents underground targets).

### 🔄 Refactor - Logging & Code Cleanup
- **Unified Logging**: Replaced `UE_LOG(LogTemp, ...)` with `AINPC_LOG(...)` macros across Smart Object system.
- **Removed Legacy Fallback**: Removed `SensoryComponent::FindBestSmartObject` fallback from `Action_SmartObject`.

### 🔄 Refactor - Action Transition System
- **Unified Action Transition Logic**: Implemented a centralized, priority-based transition system.
  - Replaced scattered `ShouldExit()` overrides with data-driven configuration in `DT_UtilityActions`.
  - **Priority System**: Added `Priority` column (0-10) to govern action interruption rules.
  - **Commitment System**: Added `CommitmentTime` to Actions to prevent rapid switching (flickering).
- **Legacy Code Removal**:
  - Removed `bInConversation` flags from `Action_TalkTo` and `UtilityAIController` (now handled by Priority System).
  - Removed `bIsInteracting` duration checks in `Action_SmartObject` (replaced by `CommitmentTime`).
- **Configuration**: Updated `DT_UtilityActions` with new Priority, CommitmentTime, and ExitConditions.

## [Unreleased] - 2026-01-22

### ✨ Features - SmartObject Interaction
- **Implemented InteractionOffset**: Added `InteractionOffset` to `SmartObjectComponent` (with 3D widget).
  - Allows level designers to specify an exact interaction point (e.g., above ground for buried ore).
  - `Action_SmartObject` use 2D Distance Check (`DistSquaredXY`), ignoring height differences (Z-axis).
  - **Dynamic Facing**: NPC moves naturally to the offset, but turns to face the Smart Object only upon arrival (before animation starts).
  - Supports `MoveToLocation` with navigation projection to ensure reachability.

### 📝 Documentation
- **Localization**: Translated README.md to English.
- **License**: Changed project license to "Limited License".
- **Updates**: Synchronized README Latest Updates with Changelog.
- **Roadmap**: Added Phase 7 (RLHF), Phase 8 (Director), **Phase 9 (Local Distillation)**, Phase 10 (Self-Calibration), and **Phase X (Neural Utility)**.
- **Design**: Expanded `Neural_Utility_Architecture.md` with **Vector Math**, **TinyStories Strategy**, and **Social Memory Integration**.

## [Unreleased] - 2026-01-21

### 🔧 Fixes - Utility AI & Architecture
- **Fixed PAM Application**: Resolved `PersonalityComponent` lookup failure by checking Controller first.
- **Fixed DataTable Config**: Corrected `Neuroticism` values for `Brave` (0.1) and `Cautious` (0.9) profiles.
- **Fixed Threat Architecture**: Implemented "Hybrid Control" for `Perceived_Threat`. Engine now resets Threat to 0 when a **Hostile** enemy dies.
- **Fixed Self-Interruption**: Modified `UtilityActionBase` to bypass Cooldown check if the action is currently running.
- **Docs**: Updated `project_notes.md` with new jurisdiction rules.

### 🧹 Code Cleanup - Legacy Removal
- **Removed**: `OnStimulusProduced` legacy delegate from `SensoryComponent`.
  - Unified all events to use `OnSemanticEventSensed` single path.
  - Removed `RelaySensoryToCognition` function from `UtilityAIController`.
  - Cognition now triggered directly from `OnSemanticEventReceived`.
- **Changed**: Reduced `PerceptionCooldown` from 5s to 2s for faster response.

### 🐛 Bug Fix - Death Event Filtering
- **Fixed**: Death events now bypass `ProcessEventFilter` completely.
  - Added fast-path for `Event.Death`, `Event.Death.Self`, `Event.Death.Witnessed` tags.
  - Ensures death events always reach Memory and Cognition.

### 🔴 P0 Critical Fix - Cross-Instance Interference
- **Fixed**: Static variables causing multi-NPC behavior corruption.
  - `UtilityAIComponent`: `LastStatusLog` → `LastStatusLogTime` (member variable)
  - `MetabolismComponent`: `LastEmotionDecayTime` (member variable)
- **Fixed**: `MemoryComponent` lookup in `MetabolismComponent` was using Pawn instead of Controller.
  - Hostile memories now correctly resolved when threat decays.


### 🚀 Optimization - LLM Latency & Memory 
- **Added**: "Amygdala Hijack" Mechanism in `CognitionComponent`.
  - Immediate threat response: bypasses LLM latency by detecting `HOSTILE`/`DANGER` keywords in localized stimuli.
  - Spikes `Perceived_Threat` to 0.9 instantly before sending LLM request.
- **Added**: Request Throttling System.
  - Prevents LLM spam from repetitive perception events.
  - High Priority (Danger): 1.5s cooldown.
  - Normal Priority: 4.0s cooldown.
- **Fixed**: Memory Logic for Death Events.
  - Removed `ShouldPerceiveTarget` cooldown check for death events to ensure they are always committed.
  - Implemented `GetSmartActorName` to resolve memories using both Display Name (e.g. "zombie") and Object Name.
  - Fixed logic where existing "Hostile Enemy" memories were not being resolved upon enemy death.

### 💀 Gameplay - Death Handling
- **Refactored**: `ACombatEnemy::OnEnemyDied` delegate now passes `DeadEnemy` and `Killer` parameters.
- **Added**: `SensoryComponent` binds to `OnEnemyDied` upon perceiving a hostile actor.
- **Added**: Automatic Memory Cleanup.
  - When `Perceived_Threat` decays (0.4 -> 0.1), all Hostile memories are marked as `[RESOLVED]`.
  - Simulates NPC "calming down" and realizing the danger has passed.

## [Unreleased] - 2026-01-20

### 🔧 Logging & Debugging (Phase 4 P0)
- **Dedicated Log Channels**: Implemented specialized log categories in `AINPC.h` to reduce console noise:
  - `LogAINPCBrain`: High-level directives and LOD switches.
  - `LogAINPCMemory`: Memory storage, decay, and retrieval.
  - `LogAINPCSocial`: Dialogue and social interactions.
  - `LogAINPCUtility`: Action scoring and selection.
- **Component Logging**: Updated `MemoryComponent`, `UtilityAIComponent`, `GoalComponent` to use these new channels.

### 💄 UI & Feedback
- **Robust Nameplate Initialization**: Fixed nameplate UI initialization failure by implementing a robust search for `NPCDefinitionComponent` across both Pawn and Controller.
- **Navigation Fix**: Disabled collision on all EmotionDisplay widgets (Nameplate, Emoji, SpeechBubble) to prevent UI elements from blocking NPC navigation and raycasts.

### 🗣️ Speech Bubble System Redesign
- **Simplified Speech Gate**: Changed from whitelist (only TalkTo/Attack/Flee) to blacklist (only block during Sleep). NPCs can now speak in any context except sleeping.
- **Conversation Mode**: Added `bInConversation` flag to `UtilityAIController` for tracking active conversations.
- **Vision Suppression**: During TalkTo action, suppress player vision events to prevent "seen player 3 times" noise. Enemy events still pass through for combat interruption.
- **Auto-Speech Timer**: NPCs now auto-speak every 8 seconds during conversation if player doesn't respond.
- **Player Reply Reset**: When player speaks, conversation timer resets for immediate NPC response.
- **Player Interruption**: Player speaking to sleeping/eating NPC boosts Loneliness, triggering Social directive switch.
- **Topic Guidance**: Improved auto-speak prompts to focus on light small talk (weather, work, local news).
- **Conversation Continuation**: ReceiveSpeech now instructs LLM to respond naturally to what was just said.

### 🔧 Debug & Quality of Life Improvements

#### AINPC_LOG Macro System
- **Added**: Centralized logging macros in `AINPC.h` with automatic class name prefixing
  - `AINPC_LOG(Verbosity, Format, ...)` - General logging with auto class name
  - `AINPC_LOG_WARNING()` - Warning level
  - `AINPC_LOG_ERROR()` - Error level  
  - `AINPC_LOG_VERBOSE()` - Verbose level
- **Changed**: Updated 9 core NPC components to use new AINPC_LOG macros:
  - `SensoryComponent`, `UtilityAIComponent`, `CognitionComponent`
  - `PersonalityComponent`, `MemoryComponent`, `MetabolismComponent`
  - `GoalComponent`, `NPCDefinitionComponent`, `EmotionDisplayComponent`
- **Impact**: Debug logs now show class names automatically (e.g., `[USensoryComponent] Perception: ...`)

#### Toggle Chat UI Fix
- **Fixed**: Player Chat UI toggle not working properly in `AINPCPlayerController`
  - Changed `FInputModeUIOnly` to `FInputModeGameAndUI`
  - Player can now press T to close the chat UI while it's visible
  - Previously, keyboard input was blocked when UI was open

### 📝 Documentation
- **Added**: `docs/design/Phase4_AI_System_Enhancement_Plan.md` - Comprehensive implementation plan for:
  - Memory System Refactor (Mathematical Decay, Resolution Tags, GC)
  - Agency & Soul (Gossip Network, Hidden Agendas, Obsession Evolution)
  - AI-Friendly Debug Protocol (Log Channels, JSON Snapshots, Visual Overlay)
  - Prompt Optimization (Layer Architecture, LOD-based Stripping)

---

## [Unreleased] - 2026-01-19

### 🎯 Phase 3 - Player Interaction & Social Identity
- **Added**: `UAction_TalkTo` utility action for AI-initiated dialogue.
  - Integrates with `CognitionComponent` for LLM-generated speech based on personality/context.
  - Automatically reduces `Loneliness` and `Boredom` upon successful interaction.
- **Added**: Player-to-AI Dialogue System in `AINPCPlayerController`.
  - `UPlayerDialogueWidget`: New C++ base class for chat input.
  - `ToggleChat()`: Handles UI/Game mode switching and widget visibility.
  - `HandleChatMessage()`: Broadcasts player speech to nearby NPCs within 10m radius.
- **Changed**: Enhanced `SensoryComponent::ReceiveSpeech` logic.
  - Now generates high-magnitude (0.8) `Social_Chat` semantic events for the AI brain.
  - Integrated `EmotionDisplayComponent` to show speech bubbles above speakers.
- **Changed**: Architecture Refactor - Modular Identity.
  - Renamed `FBackstory` -> `FSocialProfileDef` (focusing on Role, Motivation, and Values).
  - Updated `NPCDefinitionComponent` and `CognitionComponent` to use modular prompt building (Name + PastEvent + SocialProfile).
  - Implemented Prompt LOD logic to reduce token usage by filtering static profile data based on distance/relevance.
- **Added**: New Data-Driven Content.
  - `DT_Names.json`: Modular name reservoir.
  - `DT_PastEvents.json`: Significant past event descriptions for NPCs.
  - `DT_SocialProfiles.json`: Replaced `DT_Backstories` with cleaned modular data.
- **Added**: Player UI components.
  - `WBP_playerDialog`: Player chat input interface (Blueprint).

[Unreleased] - 2026-01-18

### 🎯 Major - Work System & Directive Refinements

#### Directive System Upgrade (Maslow's Hierarchy Implementation)
- **Changed**: Directive system from binary switch (allow/forbid) to **2.0x bonus multiplier**
  - Matching directive: `Score × 2.0` (encouragement)
  - Mismatching directive: `Score × 0.0` (forbidden)
  - Provides stronger guidance while preserving Utility AI autonomy
- **Added**: Complete Maslow's Hierarchy priority system in `GoalComponent::UpdateArbitration()`
  - Priority 1 (Highest): **Survival** - Hunger > 0.6 OR Fatigue > 0.6 OR Threat > 0.5
  - Priority 2 (Medium): **Social** - Loneliness > 0.5 AND HasFriendlyNearby
  - Priority 3 (Lowest): **Work/Schedule** - Time-based daily routines
- **Removed**: `GoalDirectiveMatch` consideration from `DT_UtilityActions.json` (logic moved to code-level multiplier)

#### Social Directive Implementation
- **Added**: Social layer in `GoalComponent` triggered by Loneliness threshold with proximity check
  - Social directive activates when `Loneliness > 0.5` AND friendly NPCs are nearby
  - NPCs proactively seek social interaction when lonely during work hours
  - Uses `SensoryComponent::GetPerceivedActors()` for proximity detection
- **Added**: `Directive.Social` as medium-priority layer between Survival and Schedule

#### TimeManager Activation
- **Fixed**: TimeManager not ticking - added `TickTimeManager()` call in `AINPCGameMode::Tick()`
  - Time now flows correctly, triggering schedule-based directive changes
  - `OnHourChanged` delegate broadcasts properly

#### GoalComponent Initialization Fixes
- **Fixed**: MentalState null reference causing `UpdateArbitration` failures
  - **Added**: Lazy initialization - dynamically acquires MentalState if null
  - Handles delayed initialization in `CombatEnemy` BeginPlay
  - Added retry logic in `UpdateArbitration` to gracefully handle race conditions
- **Added**: Initial directive setup in `GoalComponent::BeginPlay()`
  - Immediately calls `CheckSchedule()` and `UpdateArbitration()` to avoid 'None' state
  - Prevents first few seconds of incorrect directive

#### Work Action Behavior System
- **Added**: Boredom reduction during Work actions in `Action_SmartObject::RestoreStats()`
  - Work actions now decrease Boredom based on SmartObject's `RestoreValue`
  - Creates meaningful "work fulfillment" loop (Boredom → Work → Reduced Boredom)
- **Fixed**: Boredom reading - changed from `Interpolator->GetTargetValue()` to direct `State->Boredom`
  - Boredom is engine-managed (passive growth via Metabolism), not LLM-controlled
  - Utility AI now correctly sees actual Boredom value instead of LLM target (always 0)
- **Added**: Emergency exit conditions in `Action_SmartObject::ShouldExit()`
  - Critical needs override `ActionDuration`: Hunger > 0.8 OR Fatigue > 0.8 OR Threat > 0.5
  - Allows NPCs to interrupt work for survival needs

#### Idle as Universal Fallback
- **Changed**: Idle action's `DirectiveTag` from `Directive.Idle` to `None`
  - Idle now available during all Directives as a safe fallback
  - Prevents NPCs from being stuck when primary actions are unavailable

### ⚙️ Configuration - Metabolism Tuning

#### Survival Trigger Threshold Adjustment
- **Changed**: `CriticalHungerThreshold` from `0.9` → `0.6` in `GoalComponent.h`
  - NPCs now switch to Survival mode earlier (more proactive)
  - Better gameplay balance - less risk of starvation/exhaustion

#### Metabolism Rate Rebalancing
- **Changed**: `HungerRate` from `0.01/s` → `0.002/s` (5× slower)
  - Time to reach 0.6 hunger: 60s → **5 minutes**
- **Changed**: `EnergyRate` from `0.008/s` → `0.0016/s` (5× slower)
  - Time to reach 0.6 fatigue: 75s → **6.25 minutes**
- **Impact**: NPCs can work for ~4-5 minutes before needing to eat/sleep

### 📝 Documentation
- **Philosophical**: Confirmed Boredom as **Self-Actualization need** (Maslow's top layer)
  - Boredom drives meaningful work, not survival
  - Work provides purpose and fulfillment, reducing Boredom
  - Complete Maslow hierarchy now implemented in AI system

### 🐛 Bug Fixes
- **Fixed**: UtilController redefinition in `Action_SmartObject::ShouldExit` (Line 237 & 275)
- **Fixed**: TimeManager subsystem type mismatch (`UWorldSubsystem` → `UGameInstanceSubsystem`)
  - Changed from `World->GetSubsystem<>()` to `GameInstance->GetSubsystem<>()`
- **Fixed**: Variable shadowing warning in `GoalComponent::BeginPlay` (TimeManager → TimeMgr)

### 🎯 Impact
#### Before:
- NPCs stuck in Idle due to Directive.None blocking all actions
- Work never triggered because Boredom read as 0
- Survival needs ignored until 0.9 threshold (too late)
- No Social directive - only schedule-based social time

#### After:
- **Survival**: Hunger/Fatigue/Threat > 0.6 → Immediate directive change
- **Social**: Loneliness > 0.5 + Friendly nearby → Proactive social interaction
- **Work**: Boredom > 0.5 → Start working (2.0× directive bonus)
- **Natural Loops**: Work reduces Boredom → Idle → Boredom grows → Work again
- **Emergency Overrides**: Critical needs interrupt any action instantly

### 📊 Behavior Examples

**Morning Work Cycle:**
```
08:00 - Directive.Work (schedule)
08:25 - Boredom 0.5 → Mining (2.0× bonus)
12:00 - Hunger 0.6 → SURVIVAL override → Eat
12:10 - Return to Work
```

**Social Interruption:**
```
10:00 - Working (Boredom low)
10:30 - Loneliness 0.6 + Player nearby → SOCIAL override → Talk
11:00 - Loneliness satisfied → Return to Work
```

**Emergency Override:**
```
14:00 - Working
14:45 - Hunger 0.85 → EMERGENCY EXIT → Eat (ignores ActionDuration)
```

---

---

---

## [0.5.2] - 2026-01-17

### 🎉 New Subsystems (GOAP-Lite Foundation)

#### 1. Smart Object Manager
- **Central Registry**: `USmartObjectManager` tracks all interactive objects by `FGameplayTag` (e.g., `Activity.Eat`, `Activity.Sleep`).
- **Reservation System**: Prevents multiple NPCs from using the same exclusive object (e.g., a single bed).
- **Spatial Queries**: Efficient `FindBestSmartObject` based on distance and availability.

#### 2. Time Manager
- **Day/Night Cycle**: `UTimeManager` handles game time advancement (`RealSecondsPerGameHour` configurable).
- **Event System**: Broadcasts `OnHourChanged` for schedule-based behaviors.
- **Schedules**: NPCs can now follow daily routines (Work, Sleep, Wander) defined in `DT_Professions`.

#### 3. Goal Component (Arbitration Layer)
- **Priority System**: Dynamically selects the current `Directive` based on context priority:
  1.  **Survival** (Highest): Thirst, Hunger, Health (Critical LOD)
  2.  **Social** (Medium): Interactions, Conversations
  3.  **Schedule** (Lowest): Default daily routine (Standard LOD)
- **LOD Integration**: Automatically lowers Context LOD when in "Schedule" mode to save performance.

### Fixed
- **Action Switching Logic**: Fixed `UUtilityActionBase::ShouldExit` default return value from `false` to `true`, allowing NPCs to correctly switch from Idle/Move actions to higher priority actions.
- **SmartObject Action Blocking**: Fixed a critical bug where `Action_SmartObject`'s duration check was misinterpreted as "disable selection" (`return 0.0f`), causing actions like Eat/Sleep to be permanently disabled after one use. Now correctly interprets `ShouldExit` as "allow switching".
- **Emotion Decay System**: Fixed LLM-set emotions not decaying by ensuring `CurrentEmotionScore` is initialized to 1.0f when set by LLM. Adjusted decay rate to -0.2 every 5s (approx 20s total duration).
- **Talk Action Exit**: Fixed `Test_TalkTo` not reducing `Loneliness`/`Boredom`, causing infinite loops. Now naturally exits when social needs are met.
- **Action Animation**: Restored missing `InteractionMontage` assignment in `UtilityAIComponent`, fixing Eat/Sleep animations not playing.
- **SmartObject Detection**: Increased `SensoryComponent` fallback scan radius from 30m to 150m to ensuring NPCs can find beds/food even after wandering far.
- **Physiological Priority**: Increased BaseReward for Eat/Sleep actions (2.0 -> 4.0) to prevent Emotion Penalties (e.g., Curious) from suppressing survival needs.

### ✨ Features
- **NPC Definition Component**: Unified "ID Card" component (`UNPCDefinitionComponent`) to manage PersonalityID, ProfessionID, etc.
- **Profession-Based Action Filtering**: Implemented `RequiredProfessionID` in `UtilityActionBase` to strictly limit actions to specific roles.

### 🔧 Configuration
- **New Tables**:
  - `DT_Professions`: Defines daily schedules for roles (Merchant, Scholar, etc.).
- **Updated Tables**:
  - `DT_UtilityActions`: Added `RequiredProfessionID` column.



## [0.5.1] - 2026-01-15

### 🔧 Utility AI Fixes & Enhancements

#### Emotion Evaluator Refactored
- **Algorithm Change**: Switched from "Waterfall Priority" to "Utility Scoring (Winner Takes All)"
  - New formula: `Score = Max(0, (Value - Threshold) * Weight)`
  - Highest urgency emotion now wins, preventing low-priority emotions from masking critical ones

#### Intention/Emotion Pipeline Fixed
- **Intention Transfer**: Fixed `UNPCMentalState::UpdateFromStruct()` to copy `Intention` from LLM
- **Emotion Parsing**: Fixed LLM emotion parsing to use fully qualified enum name (`EEmotionState::Scared`)
- **LLM Priority**: Controller now prioritizes LLM's explicit emotion over calculated values

#### Detailed Utility Logging
- **Score Breakdown**: Added comprehensive logging for action score calculation
  - Motivation Sum, Intention Bonus, Context Product, Emotion Multiplier
  - Final formula: `FINAL SCORE = Base * (Mot + Intent) * Ctx * Emo`
- **Abort Logging**: Clear error log when Context = 0 aborts calculation

#### Death Event Perception
- **FocusActor Clearing**: When an actor dies, the perception system now clears `FocusActor` if it was targeting the dead actor
- **Dead Actor Filter**: `HandleTargetPerceived` now ignores actors with `Dead` tag or in ragdoll state

#### LLM Prompt Improvements
- **Strict Emotion**: Added explicit rule limiting Emotion output to 7 valid values only

### 🐛 Bug Fixes
- Fixed `EmotionMatrixTable` not being read due to timing issues (added BeginPlay diagnostic)
- Fixed duplicate death event logs (cooldown filter applied)

---

## [0.5.0] - 2026-01-15

### 🏛️ Major Architecture Update: Hybrid V3

#### 1. Deficit Model & Variable Renaming
- **Refined Mental State Variables**: Transitioned to a unified "Deficit Model" where needs grow and reactions decay.
  - `Anger` → `Indignity` (Reactive: Decays over time)
  - `Curiosity` → `Boredom` (Need: Grows over time)
- **Metabolism Logic Update**:
  - `Hunger`, `Fatigue`, `Boredom`, `Loneliness`: **Grow over time** (Need based).
  - `Indignity`, `Perceived_Threat`: **Decay over time** (Reaction based).
  - Configurable rates added: `BoredomRate`, `LonelinessRate`.

#### 2. Emotion Matrix System
- **New Component**: `EmotionMatrixConfig` and `EmotionTypes`.
- **Logic**: Implemented 7 Core Emotions (`Neutral`, `Angry`, `Scared`, `Sad`, `Happy`, `Curious`, `Disgust`).
- **Matrix Multiplier**: `Score *= Matrix[Emotion][ActivityTag]`
  - Example: `Scared` emotion multiplies `Activity.Flee` score by 5.0x and `Activity.Combat` by 0.1x.
- **Data-Driven**: Configurable via `DT_EmotionMatrix` DataTable.

#### 3. Intention Override Mechanism
- **Strategic Control**: LLM's `Intention` output now serves as a "Veto" or "Super Vote".
- **Logic**: If `LLM.Intention` matches `Action.IntentionTag`, score is multiplied by **3.0x**.
  - Solves the "Cowardly Warrior" problem: Warrior can attack even if `Perceived_Threat` is high, if LLM explicitly intends to `Attack`.
  - Preserves Physical Context: 0 score (impossible actions) remains 0.

### 🔧 Technical Changes
- **UtilityActionBase**: Added `ActivityTag` (for Matrix) and `IntentionTag` (for Override).
- **UtilityAIController**: Added central `CurrentEmotion` calculation using `EmotionEvaluator`.
- **LLM Prompt**: Updated to output `Intention` string and use new variable names.
- **Cleaned Up**: Removed obsolete variables (`Trust`, `social_battery`, `Energy`) and unused MaslowWeights dynamic calculation.

### 📝 Documentation
- **Updated**: `docs/design/5_Stage_AI_Pipeline_Design.md` to reflect the final V3 architecture.

---

## [0.4.7] - 2026-01-13

### 🔧 Changed - Metabolism System Overhaul

#### Engine-Exclusive Variable Protection
- **Fixed LLM Overwriting Engine Values**: LLM responses and Interpolator no longer overwrite `Hunger` and `Fatigue` values managed by `MetabolismComponent`
  - Modified `UNPCMentalState::UpdateFromStruct()` to skip Engine-exclusive fields
  - Modified `MentalStateInterpolator::SetCurrentValue()` to skip Engine-exclusive fields
  - Modified `UtilityActionBase::GetConsiderationValue()` to read `Hunger`/`Fatigue` directly from State, not Interpolator

#### Variable Rename: Energy → Fatigue
- **Renamed `Energy` to `Fatigue`** for clearer semantics
  - `Fatigue = 0.0` → Not tired (just woke up)
  - `Fatigue = 1.0` → Very tired (needs sleep)
  - Updated all references across codebase

#### Metabolism Logic Fix
- **Fixed Hunger/Fatigue Direction**: Clarified that both represent "need level" (higher = more urgent)
  - Time passing → Increases Hunger/Fatigue (become hungry/tired)
  - Eating/Sleeping → Decreases Hunger/Fatigue (become satisfied/rested)
  - SmartObject `RestoreValue` is positive (e.g., 1.0 = restore 1.0 per second)

#### Action Scoring Fix
- **Removed Montage Playing Check**: Fixed issue where eating/sleeping animations blocked all action scoring
  - Previously: Any montage playing → All action scores = 0
  - Now: Eating/Sleeping animations don't block action evaluation

### 🐛 Bug Fixes
- **Fixed Action Duration Auto-Exit**: Actions with `ActionDuration` now correctly force score to 0 when expired
- **Fixed Action Switching**: NPCs now correctly switch from Eating/Sleeping to Idle when done
- **Fixed InertiaBonus Logic**: InertiaBonus no longer applies when base score is 0 (allows action exit when need is satisfied)
- **Fixed Action State Reset**: `bIsInteracting` now properly reset in `Exit()` to prevent stuck states
- **Fixed Static Variable Bug**: Changed `LastDebugTime` from `static` to member variable to prevent cross-instance interference
- **Fixed MoveToActor Failure**: Added error checking for `MoveToActor` return value with proper logging
- **Fixed SmartObject NavMesh Issue**: Increased acceptance radius from 50cm to 200cm to handle SmartObjects whose center is not on NavMesh
- **Fixed LLM Missing Emotion/Speech**: Updated System Prompt to include `Emotion`, `Speech`, and `Intention` fields

### 📝 Documentation
- **Added**: `docs/guides/Metabolism_Logic_Fix_Summary.md` - Complete summary of metabolism fixes

### 🎯 Impact
- **Before**: Hunger/Fatigue values were constantly reset by LLM, actions never switched properly
- **After**: Metabolism system works end-to-end: NPC becomes hungry → eats → becomes satisfied → idles → becomes hungry again

---

## [0.4.6] - 2026-01-13

### ✨ Features
- **Combat Death Perception**: `CombatEnemy` now broadcasts death events to nearby AI NPCs' `SensoryComponent`.
- **Sensory Filtering for Death**: `SensoryComponent::HandleDeath` now uses the same attention/filtering mechanism as Sight (`ShouldPerceiveTarget`, `ProcessEventFilter`) to prevent spam and respect importance rules.
- **Killer Tracking**: `CombatEnemy` now tracks `LastDamageCauser` to correctly identify the killer in death events.

## [0.4.5] - 2026-01-13

### 🐛 Bug Fixes
- **Robust LLM Retry Mechanism**: Fixed race conditions where LLM requests would fail if the Personality was not yet initialized (`PendingStimulus` fix).
- **Faction Initialization**: Fixed issue where `GetFaction()` returned Neutral due to race conditions or missing DataTable fields.
- **PendingStimulus Logic**: Correctly clearing `PendingStimulus` after successful retry to prevent infinite loops or silent failures.

### 📝 Documentation
- **Added**: `docs/troubleshooting/Faction_Not_Loading_Fix.md` - Guide to fixing Faction initialization issues.

## [0.4.4] - 2026-01-12

### 🔧 Changed - Faction System Simplification

#### Core Faction System
- **Simplified Faction Enum**: Reduced `EFactionType` to three core factions: `Neutral`, `Human`, and `Monster`
  - Removed complex faction types (Bandit, Wildlife, etc.)
  - Clearer semantic meaning and easier to configure
  - All existing tags (e.g., "Bandit") now map to `Monster` faction

#### Hostility Logic Refinement
- **Updated Hostility Rules**: Simplified faction-based hostility detection in `SensoryComponent`
  - If two actors are of different factions (and neither is Neutral), they are hostile
  - Neutral faction is never hostile to anyone
  - Same faction actors are never hostile to each other
- **Removed Tag-Based Hostility**: Replaced "Enemy" tag checks with faction-based system for cleaner architecture

#### Player Handling
- **Player Faction Assignment**: Players are now correctly assigned to `Human` faction
  - Ensures consistent behavior with other human NPCs
  - Monsters will be hostile to players automatically

### 📝 Documentation
- **Added**: `docs/updates/Faction_System_Update.md` - Complete faction system update summary with examples and migration guide

### 🎯 Impact
- **Before**: Complex faction system with multiple types, tag-based hostility checks
- **After**: Simple three-faction system with clear hostility rules based on faction membership
- **Benefit**: Easier to configure, more predictable behavior, reduced complexity

---

## [0.4.3] - 2026-01-12

### 🐛 Bug Fixes
- **Fixed Compilation Error**: Updated `MetabolismComponent.cpp` to correctly include `UtilityAI/UNPCMentalState.h` instead of the non-existent `UtilityAI/NPCMentalState.h`.

### ✨ Added
- **Universal Smart Object Action**: Implemented `UAction_SmartObject` to handle generic interactions (Eat, Sleep) driven by tags.
- **Sensory Scanning**: Added `FindBestSmartObject` to `SensoryComponent` to locate objects by `SocialTag`.
- **Configuration**: Added `SmartObjectTag` to `FUtilityActionConfig` for data-driven targeting.
- **New Inputs**: Added `HasFoodNearby` and `HasBedNearby` Utility inputs.


## [0.4.2] - 2026-01-09

### 🐛 Critical Bug Fixes

#### Faction System
- **Fixed Faction Loading**: Modified `PersonalityComponent` to correctly load `Faction` field from `PersonalityTable` into `PersonalityConfig` struct
- **Fixed GetActorFaction**: Updated `SensoryComponent::GetActorFaction()` to handle both Controller and Pawn as input, fixing the issue where Self faction was always "Neutral"
- **Enhanced Faction Detection**: Added automatic `SetFocus()` call when hostile faction is detected, ensuring Attack actions target the correct enemy

#### Utility AI Decision Making
- **Implemented Target Value System**: Modified `UtilityActionBase::GetConsiderationValue()` to use `MentalStateInterpolator::GetTargetValue()` instead of current interpolated values
  - Utility AI now responds immediately to LLM intentions without waiting for interpolation
  - Example: When LLM sets `Perceived_Threat=0.7`, Utility AI sees 0.7 immediately instead of waiting for interpolation from 0.0
- **Added MentalStateInterpolator::GetTargetValue()**: New public function to retrieve LLM's intended values for Utility AI decision making

#### Attack Action Targeting
- **Fixed Attack Target Selection**: Modified `TestAction_Attack` to only attack the current `FocusActor` instead of automatically searching for enemies or defaulting to Player
  - Removed automatic enemy search logic from `Enter()` and `Execute()`
  - Attack now relies on `SensoryComponent` to set the correct target via `SetFocus()`
  - Prevents NPCs from incorrectly attacking Player when they should attack each other

### 🔧 Technical Improvements

**SensoryComponent**
- Added `SetFocus()` call when hostile faction is detected (Magnitude 0.8)
- Ensures Attack actions receive the correct target from perception system
- Added debug logging for FocusActor changes

**UtilityActionBase**
- Added includes for `UtilityAIController`, `CognitionComponent`, and `MentalStateInterpolator`
- Modified `GetConsiderationValue()` to access Interpolator through Controller→CognitionComp→Interpolator
- Fallback to current MentalState values if Interpolator is unavailable

**MentalStateInterpolation**
- Added `GetTargetValue(const FString& VariableName)` public function
- Returns LLM's intended value before interpolation completes
- Returns 0.0 if target value not set

### 📝 Code Quality

**Improved Variable Naming**
- Renamed `Controller` to `Controller1` in `GetActorFaction()` to avoid shadowing warnings
- Better code clarity and compiler compliance

### 🎯 Behavior Changes

**Before:**
- Warrior sees Zombie → LLM returns `Perceived_Threat=0.7` → Utility AI sees `0.0` (not interpolated yet) → Flee action wins
- Attack action automatically searches for "Enemy" tag or Player → Both Warrior and Zombie attack Player

**After:**
- Warrior sees Zombie → LLM returns `Perceived_Threat=0.7` → Utility AI sees `0.7` (target value) → Attack action wins
- Attack action uses FocusActor set by SensoryComponent → Warrior attacks Zombie, Zombie attacks Warrior

### 🐛 Known Issues

None currently identified.

---

## [0.4.1] - 2026-01-08

### 🐛 Bug Fixes & Refinements

#### AI Perception System
- **Fixed Inter-NPC Perception**: Added `AIPerceptionStimuliSource` to NPC Blueprints (via documentation guide) and configured for Sight/Hearing, allowing NPCs to perceive each other.
- **Improved Perception Logging**: Updated `SensoryComponent` to log `PersonalityID` (e.g., "I saw Zombie") instead of generic instance names, providing clearer context for the LLM.

#### Utility AI Behavior
- **Fixed Attack Targeting**: Modified `TestAction_Attack` to prioritize actors with the "Enemy" tag. NPCs with specific personalities (like BraveWarrior) will now correctly target enemies (like Zombies) instead of defaulting to the Player.
- **Enhanced Logging**: Added `[UtilityAI|<PersonalityID>]` prefix to Utility AI logs for better debugging in multi-NPC scenarios.

#### Visual & Presentation
- **Fixed Emoji Display Initialization**: Added a delay to `EmotionDisplayComponent::BeginPlay` to ensure the Controller possesses the Pawn before creating widget components, solving the issue where emojis wouldn't appear.
- **Introduction of Speech Bubbles**: Integrated speech bubble display driven by LLM output, allowing NPCs to "speak" based on their current mental state and context.

### 📝 Documentation
- **Troubleshooting Guides**: Added guides for fixing Perception issues (`Why_NPCs_See_Player_But_Not_Each_Other.md`) and Summary of Fixes (`Fix_Summary_2026_01_08.md`).

## [0.4.0] - 2026-01-08

### 🎉 Major Features

#### OCEAN Personality + Maslow's Hierarchy System

**Added OCEAN Big Five Personality Model**
- Implemented 5 personality traits: Openness, Conscientiousness, Extraversion, Agreeableness, Neuroticism
- Created `PersonalityConfig.h` with OCEAN structure and preset templates
- Added DataTable support for personality configuration
- Preset personalities: BraveWarrior, CautiousGuard, FriendlyMerchant, ReclusiveScholar

**Implemented Maslow's Hierarchy of Needs**
- Replaced old emotion system with 10 scientifically-grounded Maslow variables
- 5 hierarchy levels: Physiological, Safety, Love/Belonging, Esteem, Self-Actualization
- Created `MaslowWeights.h` for calculated sensitivity weights
- Implemented `PsychologyModel` for OCEAN → Maslow transformation

**Established Jurisdiction Division**
- Engine-exclusive: Hunger, Energy (physical, time-based)
- LLM-exclusive: Trust, Anger, Social_Status, Curiosity (semantic, psychological)
- Hybrid (co-op): Perceived_Threat, Resource_Anxiety, Loneliness, Duty_Urgency
- Defined conflict resolution rules for hybrid variables

#### Utility AI Two-Phase Algorithm Refactoring

**Separated Motivation from Context**
- Motivation (additive): Multiple motivations can accumulate
- Context (multiplicative): Necessary conditions use multiplication
- Added `EConsiderationType` enum to distinguish types

**New Scoring Formula**
```
Score = BaseReward × (Σ Motivations) × (∏ Contexts)
      = ActionReward × (MentalState × Personality) × Contexts
```

**Simplified Configuration**
- Removed `ConsiderationType`, `Weight`, and `ResponseCurve` from `FUtilityConsideration`
- Weights now provided by PersonalityComponent
- Clearer semantics: "want to do" vs "can do"

#### BaseReward Semantic Refactoring

**Renamed BaseWeight to BaseReward**
- Better semantics: represents action's intrinsic value
- Updated all references in codebase
- Enhanced logging with formula breakdown

**Three-Dimensional Calculation**
- MentalState (LLM): "How hungry am I?"
- Personality (OCEAN): "How much do I love eating?"
- BaseReward (Config): "How filling is this meal?"

#### Semantic Mapping + Smooth Interpolation System

**Implemented SentimentMapper**
- LLM outputs semantic tags instead of numerical values
- 5-point Likert scale: None, Slight, Moderate, Strong, Extreme
- Natural language tags: Annoyed, Angry, Furious, Enraged
- Automatic tag-to-value conversion
- High fault tolerance and stability

**Implemented MentalStateInterpolator**
- Smooth transitions instead of instant jumps
- Configurable interpolation speeds per emotion
- Random perturbation for personalization
- Frame-by-frame smooth transitions: `0.1 → 0.18 → 0.28 → ... → 0.9`

**Configured Interpolation Speeds**
- Anger: Fast (5.0) - quick emotional response
- Trust: Slow (0.5) - trust builds gradually
- Hunger: Medium (2.0) - gradual increase

### ✨ New Components

**Created Files:**
- `Source/AINPC/Public/UtilityAI/PersonalityConfig.h`
- `Source/AINPC/Public/UtilityAI/MaslowWeights.h`
- `Source/AINPC/Public/UtilityAI/PsychologyModel.h`
- `Source/AINPC/Private/UtilityAI/PsychologyModel.cpp`
- `Source/AINPC/Components/PersonalityComponent.h`
- `Source/AINPC/Components/PersonalityComponent.cpp`
- `Source/AINPC/Public/UtilityAI/EngineManagedValues.h`
- `Source/AINPC/Public/UtilityAI/SentimentMapping.h`
- `Source/AINPC/Private/UtilityAI/SentimentMapping.cpp`
- `Source/AINPC/Public/UtilityAI/MentalStateInterpolation.h`
- `Source/AINPC/Private/UtilityAI/MentalStateInterpolation.cpp`

### 🔧 Technical Improvements

**PersonalityComponent Architecture Fix**
- Moved PersonalityComponent from Character to AIController
- All AI components now in same controller hierarchy
- Proper integration with UtilityAIComponent

**Updated MentalStateFields**
- 10 Maslow variables with jurisdiction annotations
- C++ compliant enum names (camelCase)
- Manual mapping for underscore field names

**LLM Prompt Optimization**
- Converted to strict JSON format
- Added jurisdiction-aware instructions
- Added semantic tag selection guidelines
- Included 3 detailed input-output examples

**LLMCommunicator Updates**
- Added SentimentMapper integration
- Updated JSON parsing to support both tags and numbers
- Backward compatible with old numerical format

**CognitionComponent Integration**
- Added SentimentMapper and Interpolator
- Implemented TickComponent for frame-by-frame updates
- Updated OnLLMReply to use Interpolator

### 🐛 Bug Fixes

**Fixed Enum Naming Compilation Errors**
- Removed underscores from `EUtilityInputType` enum values
- Added `GetVariableNameFromInputType()` for mapping
- Maintained underscore naming in struct fields for readability

**Updated Log Output**
- Now displays all 10 Maslow variables
- Hierarchical display by Maslow level
- Bilingual labels (Chinese and English)

### 📝 Documentation

**Design Documents:**
- `OCEAN_Maslow_System.md` - Complete system design
- `OCEAN_Maslow_Coefficients.md` - Transformation coefficients
- `Maslow_Variables_Jurisdiction.md` - Jurisdiction details
- `Jurisdiction_Summary.md` - Quick reference
- `LLM_Prompt_Optimization.md` - JSON prompt structure
- `UtilityAI_TwoPhase_Algorithm.md` - Algorithm explanation
- `BaseReward_Calculation_Formula.md` - Formula and examples
- `Semantic_Mapping_System.md` - Semantic mapping guide
- `Data_Flow_Summary.md` - Complete data flow
- `Complete_Data_Flow.md` - Integration examples

**Implementation Guides:**
- `PersonalityComponent_Integration_Guide.md`
- `Engine_Managed_Values_Implementation.md`
- `PersonalityTable_Configuration_Guide.md`
- `PersonalityComponent_Architecture_Fix.md`
- `UtilityAI_Simplification_Refactoring.md`
- `Compilation_Error_Fix_Summary.md`
- `Semantic_Mapping_Integration_Summary.md`
- `Testing_Guide.md`
- `Quick_Start_Guide.md`

**Summary Documents:**
- `OCEAN_Maslow_Implementation_Summary.md`
- `README_OCEAN_Maslow.md`

### 🔄 Migration Guide

**From Old Emotion System to Maslow**
- Old emotions (Fear, Confidence, SocialBattery) replaced
- Update references to new variable names
- Update LLM prompts to request Maslow variables

**From Component Configuration to DataTable**
- Create `DT_Personalities` DataTable
- Add personality rows with OCEAN values
- Set `PersonalityTable` and `PersonalityID` in component

**From BaseWeight to BaseReward**
- Rename `Weight` to `BaseReward` in DataTable
- Values remain same, only field name changes
- Update custom code referencing `BaseWeight`

### ⚠️ Breaking Changes

**Enum Value Changes**
- `EUtilityInputType` enum values changed from underscore to camelCase
- Example: `Perceived_Threat` → `PerceivedThreat`
- Requires recompilation and DataTable updates

**MentalState Field Changes**
- Removed: `Fear`, `Confidence`, `SocialBattery`
- Added: 10 Maslow variables
- Requires LLM prompt updates

**LLM Output Format**
- Now expects semantic tags instead of numbers
- Old numerical format still supported (backward compatible)
- Recommended to update prompts to use tags

### 📊 Performance Impact

**Compile Time**
- Minimal increase (~5-10 seconds) due to new files

**Runtime**
- PersonalityComponent weight calculation: One-time at BeginPlay
- Interpolation: ~0.01ms per NPC per frame
- Total impact: < 0.1ms per NPC
### 🔄 Refactor - Mental State Decay System (Asymmetric Transition)
- **Implemented Asymmetric State Transition** in `MetabolismComponent`:
  - **Reaction (Spike)**: `Target > Current` uses high speed (10.0f) for instant emotional response (0.1s).
  - **Recovery (Decay)**: `Target <= Current` ignores target and decays towards 0.0f using `DecayRate` for natural cooldown.
- **MentalStateInterpolator Optimization**:
  - Reduced to a simple **Target Intent Cache**.
  - Implemented **Intention Decay**: Target values fade towards zero (0.2f speed), ensuring LLM commands act as temporary "pulses".
- **Jurisdiction Rules**:
  - `Perceived_Threat`, `Indignity`, `Boredom` now allow LLM-driven "pulses".
  - `Hunger`, `Fatigue` remain strictly engine-controlled.
- **Impact**: NPCs now react instantly to new stimuli but calm down naturally without LLM intervention.