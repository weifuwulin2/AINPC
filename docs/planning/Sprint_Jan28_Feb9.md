# 🏃‍♂️ 13-Day Sprint: "The Supercell Pitch"
**Timeline**: January 28 - February 9, 2026

## 📋 Overview
**Objective**: Build a portfolio-ready demo showcasing "Emergent Gameplay" and "Hybrid AI Architecture" for the Supercell AI Lab application.
**Theme**: "Small Village, Big Chaos."

---

## Week 1: Foundation & Polish (Jan 28 - Feb 2)

### 📅 Day 1 - Tuesday, January 28
**Phase**: Foundation - Action System Stability
- [ ] **Morning**: Review and document current Action Interruption System
  - Audit all `ShouldExit()` implementations across actions
  - Verify priority-based transition logic in `UtilityAIComponent`
- [ ] **Afternoon**: Test Zombie attack interruption
  - Spawn Zombie in patrol mode
  - Verify it interrupts patrol when `Perceived_Threat > 0.5`
  - Log and fix any "stuck in patrol" bugs
- [ ] **EOD Deliverable**: ✅ Zombies reliably interrupt patrol to attack

### 📅 Day 2 - Wednesday, January 29
**Phase**: Foundation - Basic NPC循环验证
- [ ] **Morning**: Create `Map_Test_AI` clean test environment
  - Single NPC spawn point
  - 1x Bed (SmartObject)
  - 1x Food Source (SmartObject)
  - Simple patrol path
- [ ] **Afternoon**: Single NPC lifecycle test
  - Spawn 1 NPC with default stats
  - Monitor: Hungry → Eat → Tired → Sleep → Patrol cycle
  - Fix any jittering or stuck states
- [ ] **EOD Deliverable**: ✅ Single NPC completes full lifecycle without errors

### 📅 Day 3 - Thursday, January 30
**Phase**: Polish - SmartObject Visual Configuration
- [ ] **Morning**: Implement `SmartObjectComponent::OnConstruction()` visualization
  - Add `DrawDebugSphere` for each slot in `SlotOffsets`
  - Add `DrawDebugArrow` showing interaction direction
  - Add slot index labels
- [ ] **Afternoon**: Test in Editor
  - Place multiple SmartObjects (Bed, Bench, Campfire)
  - Verify slots are visible without PIE
  - Adjust offsets visually
- [ ] **EOD Deliverable**: ✅ SmartObject slots visible and editable in Editor viewport

### 📅 Day 4 - Friday, January 31
**Phase**: Polish - Faction-Aware Combat Validation
- [ ] **Morning**: Audit all combat actions
  - Review `Action_Attack.cpp` target selection logic
  - Review `Action_ChargedAttack.cpp` hostility checks
  - Ensure all use `SensoryComponent::AreActorsHostile()`
- [ ] **Afternoon**: Multi-faction combat test
  - Spawn: 1 Orc, 1 Elf, 1 Human, 1 Zombie
  - Verify correct hostility matrix:
    - Orc attacks Elf ✓
    - Human attacks Zombie ✓
    - Human does NOT attack Human ✓
- [ ] **EOD Deliverable**: ✅ Faction-based combat working correctly

### 📅 Day 5 - Saturday, February 1
**Phase**: Chaos - Sabotage System Foundation
- [ ] **Morning**: Create `Action_Sabotage.cpp`
  - Implement basic action structure
  - Add target finding logic (find nearest functional SmartObject)
  - Add scoring based on `Neuroticism` and `Hunger`
- [ ] **Afternoon**: Implement sabotage behavior
  - Play "Tinker" animation
  - Apply `Broken` tag to target SmartObject
  - Update SmartObject state to "Broken"
- [ ] **EOD Deliverable**: ✅ NPC can sabotage a SmartObject

### 📅 Day 6 - Sunday, February 2
**Phase**: Chaos - Breakable Objects + Work Animations
- [ ] **Morning**: Create `BP_BreakableObject` Blueprint
  - Add `Working` / `Broken` state machine
  - Implement state-based availability logic
  - Add visual feedback (particles, material change)
- [ ] **Afternoon**: Work Action Animation Configuration
  - Add `WorkAnimationMontage` field to `FUtilityActionConfig` struct
  - Update `DT_UtilityActions` with animation references:
    - Mining → `AM_Mine`
    - Farming → `AM_Farm`
    - Crafting → `AM_Craft`
- [ ] **EOD Deliverable**: ✅ Breakable objects functional + Work animations configurable

---

## Week 2: Project Peacemaker (Feb 3 - Feb 9)

### 📅 Day 7 - Monday, February 3
**Phase**: Concept - The "Why" Engine
- [ ] **Data**: Add `ResourceScarcity` tags to Faction Memory (e.g., "We are Angry because [No Food]").
- [ ] **UI**: Implementing the "Visual Debugger" to show *why* an NPC is hostile (e.g., Bubble over head: "Hungry: 0.9 -> Hating Human").
- [ ] **EOD Deliverable**: ✅ See a Zombie attack not because "It's a Zombie", but because "Hungry".

### 📅 Day 8 - Tuesday, February 4
**Phase**: Mechanic - The Bridge (Gift System)
- [ ] **Action**: Create `Action_GiveItem`.
- [ ] **Logic**: Player giving Food to Hungry Orc -> Reduces Hostility -> Triggers "Grateful" memory.
- [ ] **Effect**: Orc stops attacking Player, but still attacks Elves.
- [ ] **EOD Deliverable**: ✅ Bribe a hostile NPC with food to survive.

### 📅 Day 9 - Wednesday, February 5
**Phase**: Mechanic - The Summit (Diplomacy)
- [ ] **Goal**: `Goal_AttendSummit`.
    - Context: Triggered by Player using `Action_CallMeeting`.
    - Behavior: Leaders walk to a central table.
- [ ] **Tension System**:
    - If Player has high rep with BOTH -> Summit starts.
    - If Saboteur interrupts -> War resumes.
- [ ] **EOD Deliverable**: ✅ See Orc and Elf leaders sitting at the same table (unless interrupted).

### 📅 Day 10 - Thursday, February 6
**Phase**: Demo Prep - The "Broken" World
- [ ] **Full Day**: Design `Map_Demo_Village`
  - Layout: 20m x 20m square village
  - Assets:
    - 3x Beds (1 initially broken)
    - 1x Food storage
    - 1x Water well (sabotage target)
    - Patrol waypoints
  - Lighting: Warm afternoon, dramatic shadows
- [ ] **EOD Deliverable**: ✅ Fully lit and decorated demo map

### 📅 Day 11 - Friday, February 7
**Phase**: Demo Prep - NPC Cast Setup
- [ ] **Morning**: Configure NPCs
  - **Mayor** (BP_NPC_Mayor):
    - Personality: Ambitious + Cautious
    - Profession: Guard
    - Faction: Humans
  - **Saboteur** (BP_NPC_Saboteur):
    - Personality: Cunning + Grumpy
    - High Neuroticism (0.9)
    - Faction: Humans (creates internal conflict!)
  - **3x Villagers**:
    - Mixed personalities (Friendly, Lazy, Paranoid)
- [ ] **Afternoon**: Test scenario
  - Let NPCs run for 10 minutes
  - Verify emergent behaviors occur
  - Adjust Director timing if needed
- [ ] **EOD Deliverable**: ✅ 5 NPCs with distinct personalities interacting

### 📅 Day 12 - Saturday, February 8
**Phase**: Recording - Capture Key Scenes
- [ ] **Morning**: Scene capture preparation
  - Enable Visual Debugger overlays
  - Set up camera angles (Top-down, Close-up, Code view)
  - Disable unnecessary UI elements
- [ ] **Afternoon**: Record 3 key scenes
  - **Scene 1** (2 min): Saboteur breaks the well (Debugger ON)
  - **Scene 2** (3 min): Cascade reactions (Villagers panic, Mayor tries to repair)
  - **Scene 3** (1 min): Code montage
    - Show `CognitionComponent.cpp` prompt construction
    - Show `DT_Factions.json` race descriptions
    - Show `VectorDatabase.cpp` neural scoring
- [ ] **EOD Deliverable**: ✅ 3 raw video clips captured

### 📅 Day 13 - Sunday, February 9 ⏰ DEADLINE
**Phase**: Final Polish & Submission
- [ ] **Morning (2 hours)**: Video editing
  - Combine 3 scenes with transitions
  - Add text overlays explaining architecture
  - Export as 1080p MP4 (< 5 minutes total)
- [ ] **Midday (2 hours)**: Documentation finalization
  - Update `README.md`:
    - Add "Demo Video" link
    - Add "Architecture Highlights" section
    - Add "How to Run" instructions
  - Polish `CHANGELOG.md` with demo features
- [ ] **Afternoon (2 hours)**: Upload & Submit
  - Upload video to YouTube (Unlisted)
  - Verify all GitHub links work
  - Draft Supercell application email:
    - Subject: "AI Lab Application - Emergent NPC Demo"
    - Body: Brief intro + Video link + GitHub repo
  - **Final Review Checklist**:
    - [ ] Video plays correctly ✓
    - [ ] README has working links ✓
    - [ ] Code compiles (test one last time) ✓
    - [ ] Email drafted and reviewed ✓
- [ ] **5:00 PM**: 🚀 **SUBMIT APPLICATION**

---

## 🎯 Success Metrics

By February 9th, you should have:
1. ✅ **Functional Demo**: 5 NPCs exhibiting emergent conflict without scripting
2. ✅ **Visual Proof**: Video showing "Thought Bubbles" explaining AI decisions in real-time
3. ✅ **Technical Innovation**: Neural Utility system integrated and logged
4. ✅ **Polished Presentation**: Professional video (< 5 min) + clean GitHub repo
5. ✅ **Submitted Application**: Email sent to Supercell AI Lab with all materials

---

## ⚠️ Risk Mitigation

**If you fall behind:**
- **Cut Day 9 (Neural Utility)**: Keep traditional utility scoring only
- **Simplify Demo Map**: Use existing test map instead of building new village
- **Reduce NPC Count**: 3 NPCs instead of 5 is still compelling

**Daily Progress Check:**
- End each day with a **working build** (no broken code overnight)
- If a task takes > 6 hours, move to "nice to have" and proceed with core feature

**Backup Plan for Video:**
- If recording fails, use screenshots + voiceover explaining architecture
- GitHub README with GIFs can substitute for YouTube video

Good luck! 🚀
