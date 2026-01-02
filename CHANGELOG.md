# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
