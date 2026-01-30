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
- [Future Plans & Roadmap](#-future-plans--roadmap)

---

## 🎯 Project Overview

**AINPC** is a **next-generation Game AI solution** that transforms static NPCs into living, breathing characters. Unlike traditional behavior trees (predictable) or raw LLM agents (slow/expensive), AINPC uses a **Hybrid Architecture** to deliver **millisecond-level gameplay response** with **LLM-powered reasoning**.

It is not just a chatbot; it is a **Society Simulation Engine**.

---

## 🌟 Product Highlights

### 🌍 A Living, Breathing Society
The world operates autonomously, even when the player isn't looking.
- **Factions & Politics**: Cultists hate Guards; Villagers fear Monsters. Relationships drive emergent conflicts.
- **Professions & Schedules**: Miners wake up at 6am to dig; Merchants open shops at 8am. They have lives, not just patrol routes.
- **Smart Ecology**: The world is semantic. Beds are for sleeping, Ore Veins are for mining. NPCs perceive and use the environment intelligently.

### 🎬 Situational Narrative System
Turn emergent chaos into structured storytelling.
- **The Director AI**: An intelligent subsystem (`NarrativeDirector`) orchestration dramatic moments behind the scenes.
- **Cinematic Enactment**: NPCs are "cast" into roles (e.g., *Victim*, *Rescuer*) for specific plot scenes (e.g., *Rescue Operation*).
- **Stage Presence**: During scenes, actors gain **Role Adherence**, ignoring hunger/distractions to focus on the performance (`Status.InScene`).

### 🧠 Deep Psychology (OCEAN + Maslow)
Characters are driven by internal needs, not scripts.
- **Maslow's Hierarchy**: NPCs eat when hungry, seek friends when lonely, and work when bored.
- **OCEAN Personality**: 
  - A *Conscientious* Guard never leaves his post.
  - A *Neurotic* Villager flees at the first sign of danger.
  - An *Extroverted* Merchant actively hails customers.
- **Memory & Trauma**: "I remember you hit me." Past events shape future trust and dialogue.

### 🛠️ Designer-First Workflow
Built for Game Designers, not just Programmers.
- **Project Settings Config**: Tweak global AI parameters (e.g., *Loneliness Threshold*) directly in Unreal Editor.
- **Data-Driven Design**: Mass-produce thousands of unique NPCs using **DataTables** (Excel/JSON).
  - Configure *Personalities, Professions, Factions, and Dialogues* without touching C++.

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

### 🌍 World Subsystems

| Subsystem Name | Responsibility | Key Features |
|:---------------|:---------------|:-------------|
| `NarrativeDirector` | Global story orchestration | Analyzes world state, triggers Plot Scenes |
| `NarrativeSquad` | Local scene management | Casts NPCs into roles, manages execution flow |
| `SmartObjectManager` | World interaction registry | Spatial queries (`FindNearbyBed`), Slot reservations |
| `TimeManager` | Living world clock | Broadcasts hourly events, drives NPC schedules |
| `FactionSubsystem` | Relationship management | Resolves hostility (Orc vs Human), Reputation tracking |


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

### v0.6.5 - Brain-Body Connection & Narrative Fixes (2026-01-31)

#### 🧠 Brain-Body Connection
- **Intention Override**: The Goal System (`Body`) now directly respects the Cognition Component (`Brain`).
- **Free Will**: If the LLM decides to "Celebrate" (Intention.Social), it immediately overrides the "Work" schedule, solving ludonarrative dissonance (e.g., freed slaves returning to mines).

#### 🎭 Narrative Logic Overhaul
- **Scene Completion**: Fixed event tag matching and added `PostSceneStimulus` to broadcast global context updates (e.g., "Tyrant is Dead").
- **Persisted Props**: Scenes can now leave behind permanent changes (debris, corpses) via `bKeepPropsOnEnd`.

### v0.6.4 - Narrative Evolution & Universal Targeting (2026-01-29)

#### 🎭 Narrative Evolution
- **Timeline System**: Implemented `FNarrativeTimelineEntry` for directed storytelling (Intro -> Climax -> Resolution).
- **Hybrid Triggers**: Scenes progress via Time Delays OR Gameplay Events (e.g., Player detection).
- **Event Broadcasting**: World interactions now drive the narrative forward.

#### 🎯 Universal Target Selection
- **Hybrid Targeting**: New Subsystem combines fast Rule-Based scoring (Combat) with slow, thoughtful LLM selection (Social/Narrative).
- **Async Cognition**: NPCs can now "think" about who to target ("Who killed my friend?") without blocking the game thread.
- **Smart Caching**: 5s cache ensures high performance in crowded scenes.

### v0.6.3 - P0 Architecture Stability & Settings (2026-01-29)

#### ⚡ P0 Refactors & Configuration
- **Externalized Settings**: Moved all AI tuning parameters (Utility weights, Needs thresholds) to `UAINPCSettings`.
  - Adjustable in **Project Settings -> Plugins -> AINPC** (No recompilation needed).
- **Centralized Helpers**: Created `AINPCHelpers` to unify component lookups across the actor hierarchy.
- **Narrative Stability**: Implemented `Status_InScene` context tag.
  - Suppresses social needs during cutscenes.
  - Boosts directive adherence (3.0x multiplier) to ensure actors stay in character.
- **Gameplay Tags**: Migrated legacy string-based tags to Native Gameplay Tags for type safety.

### v0.6.2 - Narrative Logic & UX Refinement (2026-01-28)

#### 🎭 Narrative & UX
- **Additive Narrative Roles**: Fixed prompt construction to **combine** base personality bio, specific scene roles, and plot context. NPCs now remember who they are while following the current story.
- **Targeted Player Chat**: Refactored chat logic to target the single closest NPC in front of the player (120° cone), improving interaction precision.
- **Animation Loop Fallback**: Added a manual replay system for montages, ensuring infinite looping for actions like "ComboAttack" even without "Default" section loops.
- **Initialization Warmup**: Added a 1.0s startup delay to Utility AI logic to prevent race conditions during squad spawning (e.g., "Leader" NPCs stealing slots before squad formation).

### v0.6.1 - Action Transition System (2026-01-26)

#### 🔄 Core Refactor
- **Unified Action Transitions**:
  - Replaced scattered boolean flags (`ShouldExit`, `bInConversation`) with a centralized **Priority System**.
  - **Commitment Time**: Prevents rapid action flickering by enforcing minimum execution time.
- **Data-Driven**: Action priorities (0-10) and exit rules are now fully configurable in `DT_UtilityActions`.

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

## 🗺️ Future Plans & Roadmap

### 🚧 Phase 5: Emergent Gameplay ("Soul Injection")
Ref: [Advanced_AI_Features.md](docs/design/Advanced_AI_Features.md)

- [ ] **Self-Evolution (Automated RL)**
  - **Nightly Reflection**: NPCs analyze daily logs during sleep.
  - **Dynamic Values**: Adjust `Indignity` or `Boredom` sensitivity based on daily outcomes (e.g., getting hurt reduces Trust).

- [ ] **Emergent Behavior (Bard & Stage)**
  - **Dynamic Stage**: SmartObject with conditional slots (Performer triggers Audience slots).
  - Crowds form naturally via Utility AI (Boredom -> Watch Performance), not scripts.

### 🚧 Phase 7: Human-in-the-Loop RLHF (Dialogue Tuning)
**Character Authoring Tool** - Refining personas through interaction.
- **Feedback Loop**: Chat with NPC → "Like/Edit" response → System updates dynamic prompt.
- **Distillation**: Meta-LLM extracts style consistency from feedback.

### 🚧 Phase 8: Agent-in-the-Loop (Automated "Director")
**Massive Scale Tuning** - Replacing the human with a Critic Agent.
- **The Director**: An LLM with a "Casting Brief" (e.g., "Grumpy Dwarf").
- **The Actor**: The NPC being tuned.
- **Automated Rehearsal**: Director critiques Actor's lines 100x/night until the persona is perfect.

### 🚧 Phase 9: Local Model Distillation (Edge AI)
**From Cloud to Consumer** - Compressing intelligence into a shippable format.
- **Dataset**: Use Phase 8's "Perfect Logs" as training data.
- **Fine-Tuning**: Train a small local model (Llama-3-8B/Mistral) to mimic the "Director-Approved" behavior.
- **Result**: Offline, zero-cost, high-fidelity NPCs running on the player's GPU.

### 🚧 Phase 10: Future Research (Utility AI Self-Calibration)
**Inverse Reinforcement Learning (IRL)** - The AI learns "How to be itself".
- **Observation**: System records behavior (e.g., "Did the Workaholic keep working when hungry?").
- **Calibration**: Logic automatically adjusts Utility Weights to match the desired Personality Profile.
- **Goal**: An AI that *behaves* like its personality, not just speaks like it.

### 🔮 Phase X: The "North Star" (Neural Utility Architecture)
**The Ultimate Goal** - A Neuro-Symbolic "Double Brain" System.
- **Concept**: Combine **Vector Math (System 1)** for speed with **Local LLM (System 2)** for reasoning.
- **Mechanism**: Hard-coded Utility Functions are replaced by **Semantic Vector Matching**.
- **Design Doc**: [Neural_Utility_Architecture.md](docs/design/Neural_Utility_Architecture.md)

### 📋 Technical Roadmap

See detailed design docs for implementation plans:
- [**Phase 4: Optimization & Agency**](docs/design/Phase4_AI_System_Enhancement_Plan.md)
- [**Action Transition System 2.0**](docs/design/Action_Transition_System_Plan.md)

- [ ] **Phase 4 Continuation**:
  - [x] P0: Log Channels & UI Fixes
  - [ ] P1: Memory Refactor (Decay & Resolution)
  - [ ] P2: Agency (Hidden Agendas)
  
- [ ] **Multi-NPC Coordination**
  - Group tactics for Combat.
  - Social clusters for chatting.



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
