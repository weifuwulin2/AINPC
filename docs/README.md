# 📚 Knowledge Base Index

Welcome to the AINPC Knowledge Base. This index serves as the central hub for all project documentation.

## 🧭 Navigation Categories

### 🌟 Getting Started
*   [**Framework Testing Guide**](guides/Framework_Testing_Guide.md) - A complete guide to testing the AINPC framework features.
*   [**Quick Start**](../README.md#🚀-快速开始--quick-start) - (Link to Main README)
*   [**LLM Configuration**](guides/LLM_Config_Guide.md) - How to set up API keys and endpoints.

### 🏗️ Architecture & Design
*   [**Data Flow Analysis**](analysis/DataFlow_Analysis.md) - **(Crucial)** Mermaid diagrams and detailed system data flow analysis.
*   [**Initialization Architecture**](design/Initialization_Architecture.md) - How NPCs load identities and state at spawn.
*   [**Social/Work LOD System**](design/Social_Work_LOD_Integration_Design.md) - How attention and distance affect AI behavior complexity.
*   [**Personality Pipeline**](design/Personality_OCEAN_Maslow_LLM_Pipeline.md) - Detailed flow from OCEAN traits to Maslow needs to LLM prompts.
*   [**Jurisdiction System**](design/Maslow_Variables_Jurisdiction.md) - Rules defining whether the Engine, Utility AI, or LLM controls specific state variables.
*   [**Utility AI Algorithm**](design/UtilityAI_TwoPhase_Algorithm.md) - Explanation of the specific scoring algorithm used.
*   [**Narrative Evolution**](design/NarrativeEvolution_Design.md) - Design for Story Timeline, Directors, and Scenes.
*   [**Unified Event Bus**](systems/UnifiedEventBus_Guide.md) - Guide to the new centralized event system.
*   [**Action Observation**](systems/ActionObservationSystem_Guide.md) - Guide to the NPC observation and attention budget system.
*   [**Universal Target Selection**](design/Universal_Target_Selection.md) - Design for Hybrid Rule-Based + LLM target acquisition.
*   [**Neural Utility Architecture**](design/Neural_Utility_Architecture.md) - **(North Star)** "System 1" (Vector) & "System 2" (LLM) hybrid architecture design.

### 🛠️ Configuration Guides
*   [**Single Point Config**](guides/Single_Point_Configuration_Guide.md) - How to add new mental states using the `MentalStateFields.h` macro system.
*   [**Utility Action Config**](guides/Utility_AI_Configuration_Guide.md) - How to create and tune Utility AI Actions.
*   [**Personality Tables**](guides/PersonalityTable_Configuration_Guide.md) - Configuring DataTable assets for different NPC personalities.
*   [**Social Profiles**](guides/PersonalityTable_Configuration_Guide.md) - (Updated) Configuring Role/Motivation/Values.

### Troubleshooting
*   [**Known Bugs List**](troubleshooting/Known_Bugs_List.md) - Active issues and investigation plans
*   [**Bug Fixes Patch**](guides/BugFixes_Patch.md) - Guide for applying recent fixes
*   [**Faction Not Loading**](troubleshooting/Faction_Not_Loading_Fix.md) - Troubleshooting Faction initialization issues.
*   [**Dreaming System Test**](guides/Dreaming_Test_Guide.md) - Validating the memory consolidation system.
*   [**Commit Log**](../GIT_COMMIT_MESSAGES.md) - History of git commit messages.
*   [**Changelog**](../CHANGELOG.md) - Version history.

### 🔄 System Updates
*   [**Refactoring Plan**](design/Action_TargetSelection_Refactoring_Plan.md) - **(New)** Detailed plan for Action Transition and Target Selection system refactoring.
*   [**Phase 4: AI System Enhancement Plan**](design/Phase4_AI_System_Enhancement_Plan.md) - **(New)** Comprehensive plan for Memory Refactor, Agency/Soul, Debug Protocol, and Prompt Optimization.
*   [**Phase 3: Player Interaction**](../CHANGELOG.md#v053---phase-3-player-interaction--modular-identity-2026-01-19) - Summary of the player dialogue and modular identity system.
*   [**Faction System Update**](updates/Faction_System_Update.md) - Simplification to three core factions (Human, Monster, Neutral) with improved hostility logic.
*   [**Metabolism System Fix**](guides/Metabolism_Logic_Fix_Summary.md) - Complete overhaul of Hunger/Fatigue system with Engine-exclusive protection.
*   [**Action Transition System Plan**](design/Action_Transition_System_Plan.md) - **(Implemented)** Unified, priority-based action transition system that replaced scattered logic.

### 📊 Reports
*   [**Executive Summary**](reports/EXECUTIVE_SUMMARY.md)
*   [**Final Progress Report**](reports/FINAL_PROGRESS_REPORT.md)
*   [**Concurrent Request Fix**](reports/Bug4_Fix_Report.md)

---
*Last Updated: 2026-02-02*
