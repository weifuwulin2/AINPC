# Action Transition & Target Selection System Refactoring Plan

## Overview

This document outlines the refactoring plan for the two core systems identified as having architectural issues:
1. **Action Transition System** (`UtilityAIComponent` + `UtilityActionBase`)
2. **Target Selection Subsystem** (`TargetSelectionSubsystem`)

---

## Phase 1: Immediate Fixes (P0 - Completed)

### ✅ 1.1 Fix Cache Key Design (DONE - 2026-01-31)

**Problem**: `FTargetCacheKey::Timestamp` was ignored in `operator==` but used in `IsCacheValid`, causing non-deterministic cache behavior.

**Solution**: Separated `FTargetCacheKey` (lookup key) from `FTargetCacheEntry` (cached data with timestamp).

**Files Changed**:
- `Source/AINPC/Public/Subsystems/TargetSelectionSubsystem.h`
- `Source/AINPC/Private/Subsystems/TargetSelectionSubsystem.cpp`

---

## Phase 2: Single Responsibility Refactoring (P0)

### 2.1 Extract Score Modifiers from CalculateScore

**Current State**: `UtilityActionBase::CalculateScore` is a 516-line monolithic function with 8+ responsibilities.

**Target State**: Break down into composable `UScoreModifier` classes.

#### Step 2.1.1: Create FActionEvaluationContext

```cpp
// New file: Source/AINPC/Public/UtilityAI/ActionEvaluationContext.h

USTRUCT(BlueprintType)
struct FActionEvaluationContext
{
    GENERATED_BODY()
    
    // Required Components (cached once per evaluation cycle)
    UPROPERTY() UNPCMentalState* MentalState = nullptr;
    UPROPERTY() UGoalComponent* GoalComp = nullptr;
    UPROPERTY() UPersonalityComponent* PersonalityComp = nullptr;
    UPROPERTY() UCognitionComponent* CognitionComp = nullptr;
    UPROPERTY() UFactionReputationComponent* FactionComp = nullptr;
    
    // Runtime Data
    UPROPERTY() AAIController* Controller = nullptr;
    UPROPERTY() APawn* Pawn = nullptr;
    UPROPERTY() float CurrentTime = 0.0f;
    
    // Intermediate Calculation State
    float MotivationSum = 0.0f;
    float ContextProduct = 1.0f;
    float RawScore = 0.0f;
    
    // Debug
    bool bLogDebug = false;
};
```

#### Step 2.1.2: Create Score Modifier Base Class

```cpp
// New file: Source/AINPC/Public/UtilityAI/ScoreModifier.h

UCLASS(Abstract, Blueprintable)
class AINPC_API UScoreModifier : public UObject
{
    GENERATED_BODY()

public:
    // Apply this modifier to the score
    // Returns the modified score
    UFUNCTION(BlueprintNativeEvent)
    float Apply(float BaseScore, UPARAM(ref) FActionEvaluationContext& Context, const UUtilityActionBase* Action);
    
    // Priority determines evaluation order (lower = earlier)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 Priority = 100;
    
    // Display name for debugging
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FString ModifierName;
};
```

#### Step 2.1.3: Create Specific Modifiers

| Modifier Class | Responsibility | Priority |
|----------------|---------------|----------|
| `UCooldownModifier` | Cooldown check (returns 0 if in cooldown) | 10 |
| `UConsiderationModifier` | Process Motivation/Context considerations | 20 |
| `UIntentionModifier` | LLM Intention bonus | 30 |
| `UScheduleModifier` | Schedule Activity bonus | 40 |
| `UPAMModifier` | Personality Action Modifier | 50 |
| `UEmotionMatrixModifier` | Emotion-based multiplier | 60 |
| `UDirectiveModifier` | Goal Directive filtering | 70 |

#### Step 2.1.4: Refactor CalculateScore

```cpp
float UUtilityActionBase::CalculateScore(UNPCMentalState* MentalState, AAIController* Controller, bool bLogDebug)
{
    // 1. Build Context (one-time component lookup)
    FActionEvaluationContext Context = BuildEvaluationContext(Controller, MentalState, bLogDebug);
    if (!Context.Controller) return 0.0f;
    
    // 2. Apply Modifiers in Priority Order
    float Score = BaseReward;
    for (UScoreModifier* Modifier : GetActiveModifiers())
    {
        Score = Modifier->Apply(Score, Context, this);
        if (Score <= 0.0f) 
        {
            // Early exit optimization
            if (bLogDebug) UE_LOG(LogTemp, Log, TEXT("  [%s] Aborted by %s"), *ActionName, *Modifier->ModifierName);
            return 0.0f;
        }
    }
    
    return Score;
}
```

---

### 2.2 Unify Target Management

**Current State**: Dual sources of truth - `Action::TargetActor` and `Controller->GetFocusActor()`.

**Target State**: Single source of truth via `TargetSelectionSubsystem`.

#### Step 2.2.1: Remove TargetActor from Action Classes

- Remove `TargetActor` member from `Action_Attack`, `Action_TalkTo`, etc.
- All target queries go through `TargetSelectionSubsystem::SelectTarget()`
- `Controller->SetFocus()` remains the "execution-time" target

#### Step 2.2.2: Add Target Invalidation Events

```cpp
// In TargetSelectionSubsystem.h
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTargetInvalidated, AAIController*, Controller, AActor*, OldTarget);

UPROPERTY(BlueprintAssignable)
FOnTargetInvalidated OnTargetInvalidated;

// Called when cached target becomes invalid (died, left range, etc.)
```

---

## Phase 3: Transition System Refactoring (P1)

### 3.1 Data-Driven Transition Rules

**Current State**: `CanTransition` has 5 hardcoded rules with magic numbers.

**Target State**: Rules defined in DataTable with configurable priorities and conditions.

#### Step 3.1.1: Create Transition Rule DataTable

```cpp
USTRUCT(BlueprintType)
struct FTransitionRule : public FTableRowBase
{
    GENERATED_BODY()
    
    // Rule Name (for debugging)
    UPROPERTY(EditAnywhere) FString RuleName;
    
    // Priority (lower = checked first)
    UPROPERTY(EditAnywhere) int32 Priority = 100;
    
    // Condition Type
    UPROPERTY(EditAnywhere) ETransitionCondition Condition;
    
    // Threshold Value (interpretation depends on Condition)
    UPROPERTY(EditAnywhere) float Threshold = 0.0f;
    
    // Result: true = allow transition, false = block
    UPROPERTY(EditAnywhere) bool bAllowTransition = true;
};
```

#### Step 3.1.2: Refactor CanTransition

```cpp
bool UUtilityAIComponent::CanTransition(UUtilityActionBase* Current, UUtilityActionBase* Candidate, float CandidateScore, float CurrentScore)
{
    if (!Candidate) return false;
    if (!Current) return true;
    
    // Load rules from DataTable (cached)
    for (const FTransitionRule& Rule : CachedTransitionRules)
    {
        if (EvaluateRule(Rule, Current, Candidate, CandidateScore, CurrentScore))
        {
            return Rule.bAllowTransition;
        }
    }
    
    // Default: allow if no rule matched
    return true;
}
```

---

## Phase 4: Selection Strategy Pattern (P2)

### 4.1 Abstract Selection Strategy

```cpp
// New file: Source/AINPC/Public/UtilityAI/TargetSelectionStrategy.h

UCLASS(Abstract)
class AINPC_API UTargetSelectionStrategy : public UObject
{
    GENERATED_BODY()

public:
    virtual AActor* SelectTarget(
        const TArray<AActor*>& Candidates,
        APawn* MyPawn,
        ETargetSelectionContext Context,
        const FActionEvaluationContext& EvalContext
    ) PURE_VIRTUAL(UTargetSelectionStrategy::SelectTarget, return nullptr;);
};

UCLASS()
class AINPC_API URuleBasedSelectionStrategy : public UTargetSelectionStrategy
{
    GENERATED_BODY()
public:
    virtual AActor* SelectTarget(...) override;
};

UCLASS()
class AINPC_API ULLMSelectionStrategy : public UTargetSelectionStrategy
{
    GENERATED_BODY()
public:
    virtual AActor* SelectTarget(...) override;
    
    // Async callback for LLM response
    void OnLLMResponse(const FString& SelectedName);
    
private:
    // Pending request state
    TWeakObjectPtr<AActor> PendingResult;
    bool bWaitingForLLM = false;
};
```

---

## Implementation Timeline

| Phase | Task | Estimated Effort | Dependencies |
|-------|------|-----------------|--------------|
| **1** | Cache Key Fix | ✅ Done | None |
| **2.1** | FActionEvaluationContext | 2 hours | None |
| **2.1** | ScoreModifier Base + 3 Modifiers | 4 hours | 2.1.1 |
| **2.1** | Remaining Modifiers | 3 hours | 2.1.2 |
| **2.1** | Refactor CalculateScore | 2 hours | 2.1.3 |
| **2.2** | Unify Target Management | 3 hours | None |
| **3.1** | Transition Rule DataTable | 2 hours | None |
| **3.1** | Refactor CanTransition | 2 hours | 3.1.1 |
| **4.1** | Selection Strategy Pattern | 4 hours | Phase 1 |

**Total Estimated Effort**: ~22 hours (spread across multiple sprints)

---

## Risks & Mitigations

| Risk | Mitigation |
|------|-----------|
| Breaking existing behavior | Create comprehensive test scenarios before refactoring |
| Performance regression | Profile before/after; modifiers add indirection but cache context |
| DataTable complexity | Keep default rules in code; DataTable is for customization |

---

## Success Criteria

1. **`CalculateScore` reduced to < 100 lines**
2. **No dual-source target management** (all via Subsystem)
3. **`CanTransition` rules externalized** to DataTable
4. **Zero cache-related bugs** in TargetSelectionSubsystem
5. **Easier debugging**: Each modifier logs its own contribution

---

## Next Steps

1. **Verify P0 Fix** - Compile and test cache behavior
2. **Start Phase 2.1.1** - Create `FActionEvaluationContext`
3. **Incremental Migration** - Extract one modifier at a time, test after each

