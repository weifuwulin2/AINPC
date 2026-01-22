# AINPC - LLM-Driven Utility AI System for Unreal Engine 5

<div align="center">

**UE5-Based LLM-Utility AI Hybrid Architecture**  
*Stanford Agent Logic (Brain) + Utility AI (Body)*

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.3+-blue.svg)](https://www.unrealengine.com/)
[![License](https://img.shields.io/badge/License-Limited-red.svg)](LICENSE)
[![Documentation](https://img.shields.io/badge/knowledge-base-blue.svg)](docs/README.md)

</div>

---

## 📖 Table of Contents

- [Project Introduction](#-project-introduction)
- [Core Features](#-core-features)
- [System Architecture](#-system-architecture)
- [Quick Start](#-quick-start)
- [Project Structure](#-project-structure)
- [Documentation](#-documentation)
- [Latest Updates](#-latest-updates)
- [Roadmap](#-roadmap)

---

## 🎯 Project Introduction

**AINPC** is an innovative AI NPC system designed to solve the problems of slow response and rigid actions in traditional LLM Agents. Through a **dual-layer decoupled design**, it achieves a perfect combination of intelligent decision-making and real-time response:

- **🧠 Cognitive Layer**: Uses LLM to process complex unstructured data (dialogue, environmental perception) and outputs structured emotional parameters.
- **💪 Action Layer**: Uses a Utility AI system to calculate optimal actions in real-time based on emotional parameters, ensuring **millisecond-level response**.

### Core Advantages

| Traditional LLM Agent | AINPC Hybrid Architecture |
|:----------------------|:--------------------------|
| ❌ Response Latency 1-3s | ✅ Action Response < 50ms |
| ❌ Rigid, Disjointed Actions | ✅ Fluid Behavior Transitions |
| ❌ Hard to Debug & Optimize | ✅ Visual Parameter Debugging |
| ❌ Decision requires LLM every time | ✅ LLM only handles cognitive updates |

---

## ✨ Core Features

### 🔧 Single-Point Configuration

**Automated Configuration System** - Defined once via macros, effective globally:

```cpp
// MentalStateFields.h - The only place to modify
#define MENTAL_STATE_FIELDS(FIELD) \
    FIELD(Anger, 0.0f, "Anger", "Affects aggression") \
    FIELD(Fear, 0.0f, "Fear", "Affects fleeing behavior") \
    FIELD(Confidence, 0.5f, "Confidence", "Affects confrontation strategy")
```

**Auto-generated**:
- ✅ `FMentalState` struct fields
- ✅ `UNPCMentalState` class properties (with full UPROPERTY metadata)
- ✅ Constructor initialization code
- ✅ `ResetState()` function
- ✅ `UpdateFromStruct()` / `ToStruct()` conversion functions
- ✅ LLM Prompt field list
- ✅ Utility AI `GetConsiderationValue()` switch cases

**Maintenance Cost**: Adding new attributes only requires modifying **2 files** (previously 8).

### 🧩 Modular Component Architecture

```
UtilityAIController (Controller)
├── SensoryComponent      # Sensory Translation (Physical Signal → Text)
├── CognitionComponent    # Cognitive Processing (Memory + LLM)
├── UtilityAIComponent    # Decision Execution (Utility Scoring)
└── MentalState           # Shared Emotional State
```

### 🌙 Dreaming System

Periodically organizes memories to extract long-term insights:
- Triggers automatically every 5 minutes (Configurable)
- Sends short-term memories to LLM for summarization
- Extracts key information into long-term memory
- Prevents memory queue overflow

### 🔄 Concurrent Request Management

Uses `TMap` to manage multiple concurrent LLM requests:
- ✅ Dreaming and perception events can trigger simultaneously
- ✅ Independent callback handling for each request
- ✅ Request ID tracking for easy debugging

---

## 🏗️ System Architecture

### Data Flow Diagram

```
┌─────────────────┐
│  UE5 Perception │  (Visual/Auditory)
└────────┬────────┘
         │ Physical Signals
         ▼
┌─────────────────┐
│ SensoryComponent│  Translate to Text
└────────┬────────┘
         │ "Saw Player with Gun"
         ▼
┌─────────────────┐
│CognitionComponent│ Store in Memory Stream
└────────┬────────┘
         │ Accumulate to Threshold
         ▼
┌─────────────────┐
│ LLM Communicator│  Send Prompt
└────────┬────────┘
         │ JSON Response
         ▼
┌─────────────────┐
│  MentalState    │  Update Emotional Params
│ (Anger: 0.8)    │
└────────┬────────┘
         │ Every 0.2s
         ▼
┌─────────────────┐
│UtilityAIComponent│ Calculate Action Score
└────────┬────────┘
         │ Select Highest Score
         ▼
┌─────────────────┐
│  Action Execute │  Play Animation/Move
└─────────────────┘
```

### Core Classes

| Class Name | Responsibility | Key Methods |
|:-----------|:---------------|:------------|
| `UtilityAIController` | Main controller, coordinates components | `BeginPlay()`, `ReceiveSpeech()` |
| `SensoryComponent` | Translates physical signals to text | `OnPerceptionUpdated()` |
| `CognitionComponent` | Manages memory and LLM communication | `AddMemory()`, `TriggerDreaming()` |
| `UtilityAIComponent` | Utility AI decision loop | `EvaluateActions()`, `ExecuteBestAction()` |
| `UNPCMentalState` | Emotional state data container | `UpdateFromStruct()`, `ToStruct()` |
| `UtilityActionBase` | Action base class | `CalculateScore()`, `Execute()` |

---

## 🚀 Quick Start

### Requirements

- **Unreal Engine**: 5.3 or higher
- **Compiler**: Visual Studio 2022 (Windows) / Xcode (Mac)
- **LLM API**: OpenAI / Ollama / Custom Endpoint

### Installation Steps

1. **Clone Repository**
   ```bash
   git clone https://github.com/yourusername/AINPC.git
   cd AINPC
   ```

2. **Generate Project Files**
   ```bash
   # Windows
   Right-click AINPC.uproject → Generate Visual Studio project files
   
   # Mac/Linux
   ./GenerateProjectFiles.sh
   ```

3. **Configure LLM API**
   
   Edit `Config/DefaultGame.ini`:
   ```ini
   [/Script/AINPC.LLMCommunicator]
   APIEndpoint=http://localhost:11434/api/generate
   ModelName=llama2
   APIKey=your_api_key_here
   ```

4. **Compile and Run**
   - Open `AINPC.sln`
   - Set configuration to `Development Editor`
   - Compile project
   - Launch Editor

### Quick Test

1. Open `Content/Maps/TestLevel`
2. Place a `BP_AICharacter` in the scene
3. Run Game (PIE)
4. Observe autonomous NPC behavior

Detailed Testing Guide: [Framework_Testing_Guide.md](docs/guides/Framework_Testing_Guide.md)

---

## 📁 Project Structure

```
AINPC/
├── Source/AINPC/
│   ├── Controller/
│   │   └── UtilityAIController.h/cpp      # AI Main Controller
│   ├── Components/
│   │   ├── SensoryComponent.h/cpp         # Sensory Translation
│   │   ├── CognitionComponent.h/cpp       # Cognitive Processing
│   │   └── UtilityAIComponent.h/cpp       # Decision Execution
│   ├── UI/
│   │   └── PlayerDialogueWidget.h/cpp     # Dialogue UI Base
│   ├── LLM/
│   │   └── LLMCommunicator.h/cpp          # LLM Communication
│   ├── UtilityAI/
│   │   ├── MentalStateFields.h            # ⭐ Single-Point Config
│   │   └── UNPCMentalState.h/cpp          # Emotional State
│   └── Base/
│       └── UtilityActionBase.h/cpp        # Action Base Class
├── Content/
│   ├── Blueprints/
│   │   ├── AI/                            # AI Blueprints
│   │   └── Actions/                       # Action Blueprints
│   ├── Widgets/                           # UI Widgets
│   └── Maps/                              # Test Maps
├── docs/
│   ├── guides/                            # 📖 User Guides
│   ├── reports/                           # 📝 Progress Reports
│   └── analysis/                          # 📊 Analysis Docs
├── README.md                              # This Document
└── CHANGELOG.md                           # Change Log
```

---

## 📚 Documentation

We maintain a detailed documentation library. Please visit **[📚 Knowledge Base Index](docs/README.md)** for the full list.

### Core Doc Entry Points
*   [**System Architecture Analysis**](docs/analysis/DataFlow_Analysis.md)
*   [**Bug Fix Guide**](docs/guides/BugFixes_Patch.md)
*   [**API Configuration Guide**](docs/guides/LLM_Config_Guide.md)
*   [**Git Commit Log**](docs/GIT_COMMIT_MESSAGES.md)

---

## 🎉 Latest Updates

### v0.6.0 - Phase 4: Gameplay Depth & "Living World" (2026-01-22)

#### 🧬 Smart Object Interaction Refinement
- **Interaction Offset System**: 
  - Supports 3D interaction points independent of object mesh (e.g., digging at a vein below ground).
  - NPCs navigate to the offset point but face the object dynamically.
  - Fixes "ghost interactions" where NPCs interacted with objects through walls or floors.

#### 💀 Death Event Perception
- **"I See Dead People"**:
  - NPCs now correctly perceive death events as specific semantic inputs.
  - **Memory Resolution**: Seeing a dead enemy automatically resolves "Hostile" memories, calming the NPC down.
  - **Logic Filter**: Death events bypass standard sensory filters to ensure immediate reaction.

#### 🧠 Architecture Hardening
- **Cross-Instance fix**: Fixed static variables causing multiple NPCs to share state.
- **Amygdala Hijack**: Immediate threat response system that skips LLM latency for `DANGER` events.
- **Request Throttling**: Prevents LLM spam during combat high-frequency events.

---

### v0.5.4 - Phase 4 P0: Debug & Logic Hardening (2026-01-20)

#### 🔧 Debug & Stability
- **✅ Dedicated Log Channels**
  - Implemented `LogAINPCBrain`, `LogAINPCMemory`, `LogAINPCSocial`, `LogAINPCUtility`.
  - Drastically reduced console noise and improved debugging efficiency.
- **✅ Nameplate Initialization Fix**
  - Robust initialization ensuring Nameplate UI always finds the correct `NPCDefinition`.
- **✅ UI Collision Fix**
  - Disabled collision on all EmotionDisplay widgets to prevent NPC navigation blocking.

### v0.5.3 - Phase 3: Player Interaction & Modular Identity (2026-01-19)

#### 🎉 New Features
- **✅ Action_TalkTo (Autonomous Dialogue)**
  - NPCs now initiate conversations when lonely using LLM-generated speech.
- **✅ Player Dialogue UI**
  - Real-time chat widget allowing players to type messages to nearby NPCs.
  - Automatic focus and input mode switching (Game/UI).
- **✅ Enhanced Perception**
  - Speech events now have high priority (Magnitude 0.8) and trigger visual speech bubbles.
- **✅ Modular Identity Refactor**
  - **Modular Prompting**: Name + Past Event + Social Profile (Role/Motivation).
  - **Prompt LOD**: Automatic context window optimization based on distance.

---

### v0.5.2 - GOAP-Lite Subsystems (2026-01-17)

#### 🎉 New Subsystems

- **✅ Smart Object Manager**
  - Central registry for `Activity.Eat`, `Activity.Sleep` tagged objects.
  - Reservation system to prevent NPC overcrowding.

- **✅ Time Manager**
  - Controls Day/Night cycle and broadcasts hourly events.
  - Drives NPC daily schedules (Work, Rest, Wander).

- **✅ Goal Component**
  - **GOAP-Lite Layer**: Arbitrates between `Survival` (Critical), `Social`, and `Schedule` directives.
  - **LOD System**: Automatically adjusts logic update frequency based on context.

- **✅ NPC Definition (Unified Identity)**
  - `UNPCDefinitionComponent` acts as the "Passport" containing PersonalityID and ProfessionID.

---

### v0.5.0 - Hybrid V3 Architecture: Deficit Model & Intention Override (2026-01-15)
		
#### 🎉 Major Improvements
		
##### 1. Deficit Model Unified
- **Variable Rename**: `Anger` → `Indignity`, `Curiosity` → `Boredom`.
- **Logic**: 
  - **Needs (Growth)**: Hunger, Fatigue, Boredom, Loneliness.
  - **Reactions (Decay)**: Indignity, Perceived_Threat.
		
##### 2. Emotion Matrix System
- **Filters**: 7 emotions have different multipliers for different behaviors (e.g., Scared -> Flee x5.0).
- **Data-Driven**: Fully controlled by `DT_EmotionMatrix` data table.
		
##### 3. Intention Override
- **Strategic Layer**: LLM's `Intention` output has final veto power.
- **Rules**: If LLM Intention matches Action Tag, Score **x3.0**.
- **Benefit**: Even if personality is timid (Fear), if LLM decides to fight (Intention: Attack), AI will execute attack.
		
---

### v0.4.0 - OCEAN + Maslow + Semantic Mapping System (2026-01-08)

#### 🎉 Major Improvements

##### 1. OCEAN Personality + Maslow's Hierarchy of Needs

- **✅ OCEAN Big Five Personality Model**
  - 5 Personality Traits: Openness, Conscientiousness, Extraversion, Agreeableness, Neuroticism
  - Load personality config from DataTable
  - Preset templates: BraveWarrior, CautiousGuard, FriendlyMerchant, ReclusiveScholar

- **✅ Maslow's Hierarchy of Needs**
  - 10 scientific need variables replacing old emotion system
  - 5 Levels: Physiological, Safety, Social, Esteem, Self-Actualization
  - Dynamic Weight Calculation: OCEAN → Maslow Transformation Matrix

- **✅ Jurisdiction Division**
  - Engine Exclusive: Hunger, Energy (Physical, Time-driven)
  - LLM Exclusive: Trust, Anger, Social_Status, Curiosity (Semantic, Psychological)
  - Hybrid Management: Perceived_Threat, Resource_Anxiety, Loneliness, Duty_Urgency

##### 2. Utility AI Two-Phase Algorithm Refactor

- **✅ Motivation (Additive)**
  - Multiple motivations can stack
  - Formula: `Σ(MentalState × Personality)`

- **✅ Context (Multiplicative)**
  - Necessary conditions use multiplication (Any 0 = Impossible)
  - Formula: `∏(Contexts)`

- **✅ New Scoring Formula**
  ```
  Score = BaseReward × (Σ Motivations) × (∏ Contexts)
        = Action Reward × (Mental State × Personality Weight) × Conditions
  ```

##### 3. BaseReward Semantic Refactor

- **✅ Clearer Naming**
  - `BaseWeight` → `BaseReward`
  - Represents intrinsic value of action ("Is this food filling?")

- **✅ Three-Dimensional Calculation**
  - MentalState (LLM): "How hungry am I?"
  - Personality (OCEAN): "How much do I love eating?"
  - BaseReward (Config): "Is this food filling?"

##### 4. Semantic Mapping + Smooth Interpolation System

- **✅ LLM Outputs Semantic Tags**
  - No longer outputs raw numbers (0.0-1.0)
  - Uses natural language tags: "Furious", "Trusting", "Starving"
  - 5-point scale: None, Slight, Moderate, Strong, Extreme

- **✅ SentimentMapper**
  - Automatically converts tags to values
  - Supports natural language tags (Annoyed, Angry, Furious, Enraged)
  - High fault tolerance and stability

- **✅ Smooth Interpolation System**
  - Emotion changes no longer jump abruptly
  - Configurable interpolation speed (Anger fast 5.0, Trust slow 0.5)
  - Random perturbation adds individuality
  - Smooth transition per frame: `0.1 → 0.18 → 0.28 → ... → 0.9`

#### 🔧 Technical Improvements

- **PersonalityComponent Architecture Fix**
  - Moved from Character to AIController
  - Managed uniformly with other AI components

- **LLM Prompt Optimization**
  - JSON formatting
  - Jurisdiction rules explanation
  - Tag selection instructions

- **Enum Naming Fix**
  - C++ compatible CamelCase
  - Manual mapping to underscore field names

#### 📊 Performance Impact

- PersonalityComponent Weight Calculation: One-time (BeginPlay)
- Interpolation Calculation: ~0.01ms per NPC per frame
- Total Impact: < 0.1ms per NPC

#### 📝 New Documentation

**Design Docs:**
- `OCEAN_Maslow_System.md` - Complete System Design
- `UtilityAI_TwoPhase_Algorithm.md` - Two-Phase Algorithm
- `BaseReward_Calculation_Formula.md` - Scoring Formula Details
- `Semantic_Mapping_System.md` - Semantic Mapping System
- `Maslow_Variables_Jurisdiction.md` - Jurisdiction Division
- `Personality_OCEAN_Maslow_LLM_Pipeline.md` - **⭐ Personality → OCEAN → Maslow → LLM Full Data Pipeline**

**Implementation Guides:**
- `PersonalityComponent_Integration_Guide.md` - Personality Component Integration
- `PersonalityTable_Configuration_Guide.md` - DataTable Config
- `Semantic_Mapping_Integration_Summary.md` - Semantic Mapping Integration
- `Testing_Guide.md` - Comprehensive Testing Guide
- `Quick_Start_Guide.md` - 5-Minute Quick Start

---

### v0.3.0 - Single-Point Configuration System (2026-01-03)

#### 🎉 Major Improvements

- **✅ Implemented Macro-Driven Single-Point Config**
  - All emotion attributes defined centrally in `MentalStateFields.h`
  - Automatically generates 7 different code contexts
  - Maintenance cost reduced by **75%**

- **✅ Optimized Data Structure**
  - Added `UpdateFromStruct()` / `ToStruct()` conversion functions
  - Automatic range clamping (0.0-1.0)
  - Removed manual field initialization

- **✅ Fixed Concurrent Request Conflicts**
  - Uses `TMap` to manage multiple LLM requests
  - Supports simultaneous Dreaming and Perception events
  - Added Request ID tracking

- **✅ Enabled Dreaming System**
  - Automatically organizes memories every 5 minutes
  - LLM extracts long-term insights
  - Prevents memory overflow

See full log: [CHANGELOG.md](CHANGELOG.md)

---

## 🗺️ Roadmap

### ✅ Completed

- [x] Basic Utility AI Framework
- [x] LLM Communication Module
- [x] Single-Point Configuration System
- [x] Dreaming Memory Organization
- [x] Concurrent Request Management
- [x] **Phase 3: Player Interaction & Modular Identity**
- [x] Comprehensive Documentation System

### 🚧 In Progress

- [ ] **Phase 4: AI System Enhancement (Memory, Agency, Debug)**
  - [x] P0: Log Channels & UI Fixes
  - [ ] P1: Memory Refactor (Decay & Resolution)
  - [ ] P2: Agency (Gossip & Hidden Agenda)

### 📋 Planned

- [ ] Multi-NPC Social Interaction
- [ ] More Emotional Visualization UI



## 📄 License

This project is licensed under a **Limited License** - Unauthorized commercial use or distribution is strictly prohibited. See [LICENSE](LICENSE) file for details.

---

## 📧 Contact

- **Project Homepage**: [GitHub Repository](https://github.com/weifuwulin2/AINPC)
- **Issues**: [Issues](https://github.com/weifuwulin2/AINPC/issues)
- **Documentation**: [Documentation](docs/README.md)

---

<div align="center">

**⭐ If this project helps you, please give it a Star!**

Made with ❤️ using Unreal Engine 5

</div>
