# 🧪 GOAP-Lite Testing Guide

This guide explains how to test the new **Day/Night Cycle**, **Smart Object Manager**, and **Goal Component** systems.

## 1. Testing Time Manager (Day/Night Layout)

The `TimeManager` subsystem runs automatically.

### 🔍 Verification Steps
1.  **Start PIE** (Play in Editor).
2.  Open **Output Log** (`Window -> Output Log`).
3.  Filter log by `TimeManager`.
4.  **Wait**: You should see logs like `[TimeManager] Hour Changed: 9:00` approximately every `RealSecondsPerGameHour` (Default: 60s).
5.  **Debug Speed**: To test faster, find the `TimeManager` blueprint or config and set `RealSecondsPerGameHour` to **5.0** (5 seconds = 1 hour).
6.  **Verify Events**: Ensure objects bound to `OnHourChanged` (like StreetLights or NPCs) react when the hour changes.

## 2. Testing Smart Object Manager

Smart Objects are interactive actors (e.g., Bed, Chair) registered with the system.

### 🔍 Verification Steps
1.  **Place Smart Object**: Drag a `BP_SmartObject_Bed` (or similar) into your test level.
2.  **Verify Tags**: Ensure it has the correct GameplayTag (e.g., `Activity.Sleep`).
3.  **Spawn NPC**: Place an NPC with a `PersonalityID` that has a need (e.g., "Tired").
4.  **Observe**:
    - The NPC should query `SmartObjectManager`.
    - The Manager should return the Bed.
    - The NPC should move to and use the Bed.
5.  **Test Reservation**:
    - Place **two** NPCs and only **one** Bed.
    - One NPC should use the Bed.
    - The other NPC should **wait** or find another action (Reservation system functioning).

## 3. Testing Goal Component (Arbitration)

The `GoalComponent` decides "What is most important right now?".

### 🔍 Verification Steps
1.  **Open Output Log**: Filter by `GoalComponent`.
2.  **Baseline (Schedule)**:
    - Ensure NPC stats are high (Not Hungry, Not Tired).
    - Log should show: `SetDirective: Directive.Schedule` (or specific task like `Directive.Work`).
3.  **Trigger Survival**:
    - Use Console Command (if realized) or Cheat to set NPC Hunger to 1.0 (Critical).
    - Or simply wait for Hunger to increase naturally.
    - Log should change to: `SetDirective: Directive.Survival`.
    - The NPC should immediately prioritize finding food.
4.  **Trigger Social**:
    - Reset Hunger.
    - Bring another friendly NPC nearby.
    - Log should change to: `SetDirective: Directive.Social` (if configured).

## 4. Testing Profession Schedules

NPCs should change behavior based on time of day.

### 🔍 Verification Steps
1.  **Configure NPC**: Set `ProfessionID` to `Merchant` (or a known profession).
2.  **Check Data Table**: Open `DT_Professions` and note the schedule (e.g., 08:00 Work, 18:00 Rest).
3.  **Run Time**:
    - **@ 08:00**: NPC should go to Work location (or start working).
    - **@ 18:00**: NPC should stop working and go to Rest.
4.  **Modify Time**: You can manually advance time using `TimeManager->AdvanceTime(3600)` in Blueprint to skip hours.

---

### ❓ Troubleshooting

- **NPC freezing?** Check `GoalComponent` logs to see if it's stuck in "Idle" directive.
- **Smart Object ignored?** Verify the object is strictly within the NavMesh bounds.
- **Logs not showing?** Ensure `VisualLogger` is active or check `Output Log` filters.
