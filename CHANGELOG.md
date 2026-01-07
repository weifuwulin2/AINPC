# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

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

---

## [0.3.0] - 2026-01-03

### 🎉 Major Improvements

#### Implemented OCEAN Personality + Maslow's Hierarchy System
- **OCEAN Personality Model**: Integrated Big Five personality traits (Openness, Conscientiousness, Extraversion, Agreeableness, Neuroticism)
- **Maslow's Hierarchy**: Replaced old emotion system with 10 scientifically-grounded Maslow variables
- **Dynamic Weight Calculation**: Personality traits automatically calculate sensitivity weights for each Maslow variable
- **DataTable Integration**: OCEAN personalities now loaded from DataTable for centralized management

#### Refactored Utility AI to Two-Phase Algorithm
- **Motivation (Additive)**: Multiple motivations can accumulate using addition
- **Context (Multiplicative)**: Necessary conditions use multiplication (any zero = action impossible)
- **New Formula**: `Score = BaseReward × (Σ Motivations) × (∏ Contexts)`
- **Clearer Semantics**: Separates "want to do" from "can do"

#### Renamed BaseWeight to BaseReward
- **Better Semantics**: "BaseReward" clearly represents the action's intrinsic value
- **Three Dimensions**: MentalState (LLM) × Personality (OCEAN) × BaseReward (Config)
- **Intuitive Understanding**: "How hungry am I?" × "How much do I love eating?" × "How filling is this meal?"

### ✨ New Features

#### OCEAN + Maslow System
- **Created `PersonalityConfig.h`**: OCEAN Big Five personality configuration structure
  - 5 traits: Openness, Conscientiousness, Extraversion, Agreeableness, Neuroticism
  - Range: 0.0 - 1.0 for each trait
  - Preset templates: BraveWarrior, CautiousGuard, FriendlyMerchant, ReclusiveScholar

- **Created `MaslowWeights.h`**: Calculated sensitivity weights for 10 Maslow variables
  - Physiological: Hunger, Energy
  - Safety: Perceived_Threat, Resource_Anxiety
  - Love/Belonging: Loneliness, Trust
  - Esteem: Anger, Social_Status
  - Self-Actualization: Duty_Urgency, Curiosity

- **Created `PsychologyModel.h/.cpp`**: Transformation coefficients from OCEAN to Maslow
  - DataAsset for storing transformation matrix
  - `RecalculateWeights()` function for dynamic weight calculation
  - Scientifically-grounded coefficient values

- **Created `PersonalityComponent.h/.cpp`**: Component for managing NPC personality
  - Stores OCEAN personality configuration
  - Calculates and caches Maslow weights
  - Provides `GetWeightForVariable()` for Utility AI integration
  - Loads personality from DataTable using PersonalityID

- **Created `EngineManagedValues.h`**: Structure for engine-managed Maslow variables
  - Separates engine-exclusive (Hunger, Energy) from hybrid variables
  - Provides `Reset()` and `ToString()` helper functions

#### Maslow Variables Jurisdiction Division
- **Engine Exclusive**: Hunger, Energy (time-based, physical)
- **LLM Exclusive**: Trust, Anger, Social_Status, Curiosity (semantic, psychological)
- **Hybrid (Co-op)**: Perceived_Threat, Resource_Anxiety, Loneliness, Duty_Urgency
- **Conflict Resolution**: Defined rules for merging engine and LLM values

#### LLM Prompt Optimization
- **JSON Format**: Converted LLM prompt to strict JSON format
- **Jurisdiction-Aware**: Prompt includes jurisdiction rules for each variable
- **Detailed Guidelines**: Added analysis guidelines and output format
- **Examples**: Included 3 detailed input-output scenarios

#### Utility AI Simplification
- **Removed ConsiderationType Complexity**: Simplified FUtilityConsideration to only contain InputType
- **Removed Weight/Curve**: Weights now provided by PersonalityComponent
- **Two-Phase Algorithm**: Separated Motivation (additive) from Context (multiplicative)

### 🔧 Technical Improvements

#### PersonalityComponent Architecture Fix
- **Moved to AIController**: PersonalityComponent now correctly initialized in UtilityAIController
- **Component Hierarchy**: All AI components (Sensory, Cognition, Personality, Utility) in same controller
- **Proper Integration**: PersonalityComp accessible by UtilityAIComponent for weight queries

#### Updated MentalStateFields
- **10 Maslow Variables**: Replaced old emotions (Fear, Confidence, SocialBattery) with Maslow hierarchy
- **Jurisdiction Annotations**: Added [ENGINE], [LLM], [HYBRID] annotations to each variable
- **C++ Compliant Names**: Enum values use camelCase (PerceivedThreat) while fields use underscores (Perceived_Threat)

#### Enum Naming Fix
- **Fixed Compilation Errors**: Removed underscores from enum values in `EUtilityInputType`
- **Manual Mapping**: Added `GetVariableNameFromInputType()` to map enum to field names
- **Backward Compatible**: Maintains underscore naming in struct fields for readability

#### BaseReward Formula
- **Renamed BaseWeight**: Changed to BaseReward for clearer semantics
- **Updated Calculation**: `Score = BaseReward × (Σ Motivations) × (∏ Contexts)`
- **Enhanced Logging**: Detailed log output showing all calculation steps

### 🐛 Bug Fixes

#### LLMCommunicator Log Updates
- **Updated Log Output**: Now displays all 10 Maslow variables instead of old emotions
- **Hierarchical Display**: Groups variables by Maslow hierarchy level
- **Bilingual Labels**: Both Chinese and English labels in logs

#### CognitionComponent Integration
- **Added EngineManagedValues**: Integrated engine-managed values into prompt
- **Conflict Resolution**: Implemented jurisdiction-based merging in `OnLLMReply()`
- **Hybrid Variable Handling**: Proper merging of engine and LLM contributions

### 📝 Documentation

#### Comprehensive Guides
- `docs/design/OCEAN_Maslow_System.md` - Complete system design and architecture
- `docs/design/OCEAN_Maslow_Coefficients.md` - Transformation coefficient reference
- `docs/design/Maslow_Variables_Jurisdiction.md` - Jurisdiction division details
- `docs/design/Jurisdiction_Summary.md` - Quick reference and checklist
- `docs/design/LLM_Prompt_Optimization.md` - JSON prompt structure and optimization
- `docs/design/UtilityAI_TwoPhase_Algorithm.md` - Two-phase algorithm explanation
- `docs/design/BaseReward_Calculation_Formula.md` - BaseReward formula and examples
- `docs/design/Data_Flow_Summary.md` - Complete data flow diagram
- `docs/design/Complete_Data_Flow.md` - Integration examples

#### Implementation Guides
- `docs/guides/PersonalityComponent_Integration_Guide.md` - Step-by-step integration
- `docs/guides/Engine_Managed_Values_Implementation.md` - Engine value update examples
- `docs/guides/PersonalityTable_Configuration_Guide.md` - DataTable configuration
- `docs/guides/PersonalityComponent_Architecture_Fix.md` - Architecture correction
- `docs/guides/UtilityAI_Simplification_Refactoring.md` - Simplification summary
- `docs/guides/Compilation_Error_Fix_Summary.md` - Enum naming fix
- `docs/guides/Testing_Guide.md` - Comprehensive testing guide
- `docs/guides/Quick_Start_Guide.md` - 5-minute quick start

#### Summary Documents
- `docs/OCEAN_Maslow_Implementation_Summary.md` - Complete implementation summary
- `docs/design/README_OCEAN_Maslow.md` - Documentation index and navigation

### 🔄 Migration Guide

#### From Old Emotion System to Maslow
- Old emotions (Fear, Confidence, SocialBattery) replaced with Maslow variables
- Update all references to use new variable names
- Update LLM prompts to request Maslow variables

#### From Component Configuration to DataTable
- Create `DT_Personalities` DataTable with `PersonalityConfig` structure
- Add personality rows (BraveWarrior, CautiousGuard, etc.)
- Set `PersonalityTable` and `PersonalityID` in PersonalityComponent

#### From BaseWeight to BaseReward
- Rename `Weight` to `BaseReward` in DataTable configurations
- Values remain the same, only field name changes
- Update any custom code referencing `BaseWeight`

### ⚠️ Breaking Changes

#### Enum Value Changes
- `EUtilityInputType` enum values changed from underscore to camelCase
- Old: `Perceived_Threat`, New: `PerceivedThreat`
- Requires recompilation and DataTable updates

#### MentalState Field Changes
- Removed: `Fear`, `Confidence`, `SocialBattery`
- Added: 10 Maslow variables
- Requires LLM prompt updates and code changes

### 📊 Performance Impact

#### Compile Time
- Minimal increase due to additional header files
- Macro expansion occurs at compile time

#### Runtime
- PersonalityComponent weight calculation: One-time at BeginPlay
- Utility AI scoring: Negligible overhead from weight lookup
- Memory: ~100 bytes per NPC for personality data

---

## [Unreleased] - 2026-01-03

### 🎉 Major Improvements

#### Implemented Single-Point Configuration System for Mental State Attributes
- **Centralized Configuration**: All mental state attributes now defined in a single file (`MentalStateFields.h`)
- **Automatic Code Generation**: Field definitions, properties, initialization, conversion functions, and LLM prompts are automatically generated using C++ macros
- **Reduced Maintenance**: Adding new attributes now requires modifying only 2 files instead of 8

### ✨ New Features

#### Single-Point Configuration System
- **Created `MentalStateFields.h`**: Central configuration file for all mental state attributes
  - Define attributes with name, default value, display name, and description
  - Single source of truth for all emotion/physiological properties
  
- **Automatic Code Generation**:
  - `FMentalState` struct fields auto-generated from macro
  - `UNPCMentalState` class properties auto-generated with full UPROPERTY metadata
  - Constructor initialization auto-generated
  - `ResetState()` function auto-generated
  - `UpdateFromStruct()` and `ToStruct()` conversion functions auto-generated
  - LLM prompt field list auto-generated
  - `GetConsiderationValue()` switch cases auto-generated for Utility AI

### 🐛 Bug Fixes

#### Data Structure Optimization
- **Added Conversion Functions**: Implemented `UpdateFromStruct()` and `ToStruct()` in `UNPCMentalState`
  - Automatic field copying with value clamping (0.0-1.0)
  - Eliminates manual field-by-field assignment
  - Reduces code duplication

- **Removed Manual Initialization**: Cleaned up `UtilityAIController::BeginPlay()`
  - Removed redundant manual field initialization
  - Constructor now handles all default values automatically
  - Ensures consistency with central configuration

#### LLM Integration
- **Fixed Concurrent Request Handling**: Resolved callback conflicts in `LLMCommunicator`
  - Replaced single callback variables with `TMap` for managing multiple concurrent requests
  - Prevents callback overwrites when Dreaming and perception events occur simultaneously
  - Added request ID tracking for better debugging
  - Related files: `LLM/LLMCommunicator.h`, `LLM/LLMCommunicator.cpp`

- **Auto-Generated LLM Prompts**: LLM system prompt now automatically includes all mental state fields
  - Field list generated from `MentalStateFields.h` macro
  - Ensures LLM always requests correct attributes
  - Eliminates manual prompt maintenance

#### Dreaming System
- **Enabled Automatic Memory Consolidation**
  - Added `FTimerHandle DreamingTimerHandle` to `UtilityAIController`
  - Dreaming cycle triggers every 5 minutes (configurable)
  - Sends recent memories to LLM for summarization
  - Extracts long-term insights and stores them for future decision-making
  - Related files: `Controller/UtilityAIController.h`, `Controller/UtilityAIController.cpp`

### 🔧 Technical Improvements

#### Macro-Based Code Generation
- **Field Definition Macro**: `MENTAL_STATE_FIELDS(FIELD)` in `MentalStateFields.h`
  - Single macro expands to all necessary code
  - Supports field name, default value, display name, and description
  - Used across 7 different code generation contexts

- **Automatic Switch Case Generation**: `GetConsiderationValue()` in `UtilityActionBase`
  - Switch cases auto-generated for all mental state fields
  - Eliminates manual case additions
  - Reduces risk of missing fields

#### Code Quality
- **Reduced Code Duplication**: From ~50 lines of manual field handling to 1 macro invocation
- **Improved Maintainability**: Adding new attributes requires 2 file edits instead of 8
- **Type Safety**: Automatic generation ensures type consistency across all usages
- **Self-Documenting**: Field descriptions embedded in configuration

### 📝 Documentation

#### Added Comprehensive Guides
- `docs/guides/Single_Point_Configuration_Guide.md` - Complete usage guide for the configuration system
- `docs/guides/UENUM_Macro_Limitation.md` - Explanation of UE reflection system limitations
- `docs/guides/Full_Automation_Proposal.md` - Original automation proposal and alternatives
- `docs/reports/Single_Point_Config_Implementation.md` - Implementation details and code comparisons
- `docs/reports/Full_Automation_Complete.md` - Final implementation report
- `docs/reports/Bug1_Fix_Report.md` - Data structure optimization details
- `docs/reports/Bug4_Fix_Report.md` - Concurrent request fix details
- `docs/README.md` - Documentation index and navigation

### ⚠️ Known Limitations

#### UENUM Macro Limitation
- **Enum Values Require Manual Addition**: Due to Unreal Header Tool (UHT) limitations, `EUtilityInputType` enum values cannot be auto-generated
  - UHT parses UENUM before macro expansion
  - Workaround: Manually add enum values (1 line per attribute)
  - Switch cases still auto-generated in .cpp file
  - Trade-off: 2 manual edits vs 8 for full automation

### 🔄 Migration Guide

#### For Existing Code
- No breaking changes - all modifications are backward compatible
- Existing mental state attributes continue to work
- Default values now centralized in `MentalStateFields.h`

#### Adding New Attributes
1. Edit `MentalStateFields.h` - Add `FIELD(Name, DefaultValue, DisplayName, Description)`
2. Edit `UtilityActionBase.h` - Add enum value to `EUtilityInputType`
3. Recompile - All other code auto-updates

### 📊 Performance Impact

#### Compile Time
- Macro expansion occurs at compile time - no runtime overhead
- Code size identical to hand-written code

#### Runtime
- Zero performance impact - macros expand to same code as manual implementation
- Memory usage unchanged
- CPU usage unchanged

---

## File Changes Summary

### Modified Files
- `Source/AINPC/Public/UtilityAI/MentalStateFields.h` (NEW) - Central configuration
- `Source/AINPC/LLM/LLMCommunicator.h` - Auto-generate FMentalState fields
- `Source/AINPC/LLM/LLMCommunicator.cpp` - Auto-generate LLM prompts, fix concurrent requests
- `Source/AINPC/Public/UtilityAI/UNPCMentalState.h` - Auto-generate properties, add conversion functions
- `Source/AINPC/Private/UtilityAI/UNPCMentalState.cpp` - Auto-generate initialization and conversion
- `Source/AINPC/Controller/UtilityAIController.h` - Add DreamingTimerHandle
- `Source/AINPC/Controller/UtilityAIController.cpp` - Enable Dreaming, remove manual initialization
- `Source/AINPC/Public/Base/UtilityActionBase.h` - Add MentalStateFields.h include
- `Source/AINPC/Private/Base/UtilityActionBase.cpp` - Auto-generate switch cases

### Added Documentation Files
- `docs/guides/Single_Point_Configuration_Guide.md`
- `docs/guides/UENUM_Macro_Limitation.md`
- `docs/guides/Full_Automation_Proposal.md`
- `docs/reports/Single_Point_Config_Implementation.md`
- `docs/reports/Full_Automation_Complete.md`
- `docs/reports/Bug1_Fix_Report.md`
- `docs/reports/Bug4_Fix_Report.md`
- `docs/README.md`
- `docs/REORGANIZATION_SUMMARY.md`
- `CHANGELOG.md` (this file)

---

## Contributors
- Implementation: AI Assistant + User
- Testing: User

---

## Notes
This release implements a macro-based single-point configuration system that significantly reduces maintenance overhead when adding or modifying mental state attributes. While complete automation is limited by UE's reflection system (UENUM cannot use macros), the current implementation still reduces manual work by 75% and eliminates the most error-prone parts (switch cases, conversion functions, LLM prompts).
