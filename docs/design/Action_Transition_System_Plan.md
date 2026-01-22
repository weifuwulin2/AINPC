# Implementation Plan: Unified Action Transition System

## Goal
Replace scattered action switching logic (Cooldowns, ShouldExit, hardcoded Emergency checks) with a centralized, priority-based **Transition System**. This makes behavior changes predictable and maintainable.

## Problem Analysis

Currently, action switching logic is distributed across **6 locations**:

| # | File | Logic | Issue |
|---|------|-------|-------|
| 1 | `UtilityActionBase.cpp:88-111` | Cooldown check | Special `bIsRunning` handling |
| 2 | `UtilityAIComponent.cpp:292-316` | ShouldExit + Emergency | Hardcoded `Threat > 0.5f` |
| 3 | `Action_SmartObject.cpp:234-314` | ShouldExit override | Hardcoded `Hunger/Fatigue > 0.8`, 80 lines |
| 4 | `Action_SmartObject.cpp:254-272` | Duration lock | `bIsInteracting` + `ActionDuration` |
| 5 | `UtilityAIComponent.cpp:207-217` | Inertia bonus | `Score *= (1 + InertiaBonus * 0.2)` |
| 6 | `Action_TalkTo.cpp:26-28, 161-163` | Dialogue lock | `bInConversation` flag |

This leads to "Spaghetti Transitions" where:
- An action stops for unclear reasons (cooldown self-interruption)
- Emergency thresholds are duplicated and inconsistent
- Adding new actions requires knowing all 6 places to modify

---

## Proposed Architecture

### 1. Action Priority Levels

```cpp
UENUM(BlueprintType)
enum class EActionPriority : uint8
{
    None = 0,
    Idle = 10,       // Can always be interrupted (Wander)
    Ambient = 20,    // Random acts (Looking around)
    Work = 30,       // Job tasks (Mining, Crafting)
    Social = 35,     // Dialogue/Interaction (NEW!)
    Needs = 40,      // Eating/Sleeping - High Inertia
    Threat = 50,     // Combat/Fleeing - Hard to interrupt
    Critical = 60    // Death/Stun - Uninterruptible
};
```

### 2. Transition Rules ("The Constitution")

Centralize ALL switching logic into one function:

```cpp
bool UUtilityAIComponent::CanTransition(UUtilityActionBase* Current, UUtilityActionBase* Candidate, float CandidateScore)
{
    // Rule 1: Priority Gate
    if (Candidate->Priority < Current->Priority) return false;
    
    // Rule 2: Commitment Lock (replaces Duration check)
    if (Current->IsCommitted())
    {
        // Only Threat+ can break Commitment
        if (Candidate->Priority < EActionPriority::Threat) return false;
    }
    
    // Rule 3: Same-Priority Inertia
    if (Candidate->Priority == Current->Priority)
    {
        float CurrentScore = Current->GetLastScore();
        return CandidateScore > CurrentScore * (1.0f + Current->InertiaBonus);
    }
    
    // Rule 4: Higher Priority always wins
    return true;
}
```

### 3. Data-Driven Exit Conditions

Replace hardcoded `ShouldExit()` overrides with configurable conditions:

```cpp
USTRUCT(BlueprintType)
struct FActionExitCondition
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere)
    EUtilityInputType Variable;  // e.g., Hunger
    
    UPROPERTY(EditAnywhere)
    EComparisonOperator Operator; // LessThan, GreaterThan
    
    UPROPERTY(EditAnywhere)
    float Threshold;  // e.g., 0.2

    // NEW: Only check this condition after ActionDuration has passed?
    // Useful for "Eat until full, but at least chew for 5 seconds"
    UPROPERTY(EditAnywhere)
    bool bWaitForDuration = false;
};

// In FUtilityActionConfig:
UPROPERTY(EditAnywhere, Category = "Transition")
TArray<FActionExitCondition> ExitConditions;
```

**Example Config** (DataTable row for `Action_Eat`):
```
ExitConditions: [ { Hunger, LessThan, 0.2 } ]
```

### 4. Commitment vs Cooldown (Distinct Concepts)

| Concept | Meaning | When Applied |
|---------|---------|--------------|
| **Commitment** | "I promise to do this for X seconds" | After `Enter()`, blocks lower-priority interrupts |
| **Cooldown** | "I am forbidden from doing this again for Y seconds" | After `Exit()`, returns Score=0 |

Both remain in `FUtilityActionConfig`:
```cpp
float CommitmentTime = 0.0f;  // NEW: Post-Enter lock duration
float CooldownTime = 0.0f;    // EXISTING: Post-Exit lockout
```

### 5. Unified Action Lifecycle Management

Currently, lifecycle logic (Duration, Loop, Exit checks) is only implemented in `Action_SmartObject`. This should be centralized in `UUtilityActionBase`:

```cpp
// In UUtilityActionBase:
UPROPERTY(Transient)
float ActionStartTime = 0.0f;

UPROPERTY(Transient)
bool bShouldExit = false;

void Execute_Implementation(AAIController* Controller) override
{
    // 1. Duration check (unified)
    if (ActionDuration > 0.0f)
    {
        float Elapsed = GetWorld()->GetTimeSeconds() - ActionStartTime;
        if (Elapsed >= ActionDuration)
        {
            bShouldExit = true;
            return;
        }
    }
    
    // 2. Exit conditions check (data-driven)
    if (CheckExitConditions(Controller))
    {
        bShouldExit = true;
        return;
    }
    
    // 3. Subclass-specific logic
    Execute_Subclass(Controller);
}

void Enter_Implementation(AAIController* Controller) override
{
    ActionStartTime = GetWorld()->GetTimeSeconds();
    bShouldExit = false;
    
    // Animation handling (if configured)
    if (InteractionMontage)
    {
        PlayMontage(Controller, InteractionMontage, bLoopAnimation);
    }
    
    Enter_Subclass(Controller);
}
```

**Benefits**:
- Duration, Animation, ExitConditions handled automatically
- No more 80-line `ShouldExit()` overrides

### 6. Future Considerations (GAS Inspiration)

As the system grows, we can adopt more concepts from Unreal's **Gameplay Ability System (GAS)**:

1.  **Action Tasks**: Replace internal state logic with asynchronous tasks.
    ```cpp
    // Future: UAbilityTask_MoveTo, UAbilityTask_PlayMontage
    auto* Task = CreateTask<UAbilityTask_MoveTo>(Target);
    Task->OnArrived.AddDynamic(this, &ThisClass::OnArrived);
    ```
2.  **Execution Costs**: Define resource costs (Stamina, Mana) in config.
3.  **Blocking Tags**: Explicitly block actions based on tags (e.g., "Status.Stunned" blocks "Action.Move").
4.  **Instancing Policy**: Support non-instanced actions for memory optimization (flyweight pattern).

---

## Implementation Steps

### Step 1: Define Enums & Structs
- [ ] Add `EActionPriority` enum to `UtilityActionBase.h`
- [ ] Add `EComparisonOperator` enum (LessThan, GreaterThan, Equals)
- [ ] Add `FActionExitCondition` struct
- [ ] Add `Priority`, `CommitmentTime`, `ExitConditions` to `FUtilityActionConfig`
- [ ] Add runtime fields to `UUtilityActionBase`: `Priority`, `CommitmentStartTime`, `LastScore`

### Step 2: Implement CanTransition
- [ ] Create `bool CanTransition(Current, Candidate, Score)` in `UtilityAIComponent`
- [ ] Create `bool IsCommitted()` helper in `UUtilityActionBase`
- [ ] Create `bool CheckExitConditions()` data-driven evaluator
- [ ] Migrate Inertia logic from `EvaluateAndDecide()` into `CanTransition()`
- [ ] Remove hardcoded Emergency checks (replaced by Priority gate)

### Step 3: Migrate Existing Code
- [ ] Remove `ShouldExit()` override from `Action_SmartObject` (use ExitConditions)
- [ ] Remove `bInConversation` from `Action_TalkTo` (use Social Priority)
- [ ] Remove `bIsInteracting` Duration checks (use CommitmentTime)
- [ ] Update `EvaluateAndDecide()` to call `CanTransition()` before switching

### Step 4: Update DataTable
- [ ] Add Priority column to `DT_UtilityActions`
- [ ] Configure CommitmentTime for SmartObject actions
- [ ] Configure ExitConditions for Eat/Sleep/Work actions

### Step 5 (Optional): Pending Intention System

A lightweight mechanism to remember suppressed actions during dialogue or commitment locks:

```cpp
// In UUtilityAIComponent:
UPROPERTY()
UUtilityActionBase* PendingAction = nullptr;

UPROPERTY()
float PendingScore = 0.0f;

// When CanTransition fails due to Priority (not Commitment):
void OnTransitionDenied(UUtilityActionBase* Candidate, float Score)
{
    // Only store if candidate had higher priority than pending
    if (!PendingAction || Score > PendingScore)
    {
        PendingAction = Candidate;
        PendingScore = Score;
    }
}

// When current action exits naturally:
void OnActionExit()
{
    if (PendingAction)
    {
        // Check if pending intention is still relevant (score decay)
        float CurrentBestScore = EvaluateBestAction();
        if (PendingScore > CurrentBestScore * 0.8f)
        {
            SwitchTo(PendingAction);
            PendingAction = nullptr;
            return;
        }
        PendingAction = nullptr;  // Stale, discard
    }
    
    EvaluateAndDecide();  // Normal re-evaluation
}
```

**Use Case**: Player talks to NPC → NPC wants to Attack (denied by Social priority) → Player leaves → NPC remembers Attack intention and executes it instead of re-evaluating to Wander.

- [ ] Add `PendingAction` and `PendingScore` to `UUtilityAIComponent`
- [ ] Call `OnTransitionDenied()` when `CanTransition()` returns false
- [ ] Call `OnActionExit()` in `Exit_Implementation()` flow
- [ ] Add decay/expiry for pending intentions (optional: 10s timeout)

---

## Expected Outcome

| Before | After |
|--------|-------|
| 6 scattered locations | 1 `CanTransition()` function |
| Hardcoded thresholds | Data-driven `ExitConditions` |
| "Why did it switch?" → Debug 6 files | "Why?" → Check Priority + single log |
| `bInConversation`, `bIsInteracting` flags | `Priority::Social`, `IsCommitted()` |

**Debug Output Example**:
```
[Transition] Denied: Mining(Work:30) blocked by Sleep(Needs:40)
[Transition] Allowed: Attack(Threat:50) breaks Sleep(Needs:40) via Priority
[Transition] Denied: Wander(Idle:10) blocked by Mining Commitment (8s left)
```
