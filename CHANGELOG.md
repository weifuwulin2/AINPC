# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
