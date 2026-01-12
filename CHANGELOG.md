# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

---

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
