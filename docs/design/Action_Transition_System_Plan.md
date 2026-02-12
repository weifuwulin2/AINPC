# Action Transition System

## Overview

The Transition System controls when and how NPCs switch between actions. It uses a **Priority-Scaled Inertia** model: a single formula where priority difference modulates the inertia threshold, replacing the old multi-rule hard-gate approach.

## Priority Levels

```cpp
enum class EActionPriority : uint8
{
    None = 0,
    Idle = 10,       // Wander, Patrol — always interruptible
    Ambient = 20,    // Looking around — low commitment
    Work = 30,       // Mining, Farming, Merchant Stand
    Social = 35,     // Talk, Trade interaction
    Needs = 40,      // Eat, Sleep — high inertia
    Threat = 50,     // Attack, Flee — urgent
    Critical = 60    // Death, Stun — uninterruptible
};
```

## Transition Rules

All switching decisions go through `CanTransition()`. Three rules, evaluated in order:

### Rule 0: Dead Action Yield

```
IF CurrentScore <= 0.1 → allow transition (current action is effectively invalid)
```

### Rule 1: Commitment Lock

```
IF Current action is within CommitmentTime:
    IF Candidate is Threat+ → allow (emergency override)
    ELSE → deny
```

CommitmentTime is configured per action in DataTable (e.g., Merchant Stand = 20s, Talk = 5s). This provides short-term stability after an action starts.

### Rule 2: Priority-Scaled Inertia (Core Formula)

This is the key innovation. Instead of treating priority as a hard gate ("higher always wins, lower always blocked"), priority **scales the inertia threshold**:

```
PriorityDiff  = Candidate.Priority - Current.Priority
PriorityScale = Clamp(1.0 - PriorityDiff / 20.0, 0.0, 2.0)
Threshold     = CurrentScore × (1 + InertiaBonus × PriorityScale)

IF CandidateScore > Threshold → allow
ELSE → deny
```

**How it works:**

| Direction | PriorityDiff | PriorityScale | Effect |
|-----------|-------------|---------------|--------|
| Much higher (e.g., Threat→Work, diff=+20) | +20 | 0.0 | Zero inertia — just need to outscore |
| Somewhat higher (e.g., Talk→Work, diff=+5) | +5 | 0.75 | Reduced inertia — advantage but not free |
| Same priority | 0 | 1.0 | Full inertia — maximum stability |
| Somewhat lower (e.g., Work→Talk, diff=-5) | -5 | 1.25 | Amplified inertia — uphill battle |
| Much lower (e.g., Idle→Needs, diff=-20) | -20 | 2.0 | Double inertia — nearly impossible |

**Concrete examples (InertiaBonus = 1.0):**

| Scenario | CurrentScore | Threshold | Candidate needs |
|----------|-------------|-----------|-----------------|
| Attack(50) → MerchantStand(30) | 0.5 | 0.5 × (1+1×0.0) = 0.5 | > 0.5 |
| Eat(40) → MerchantStand(30) | 0.5 | 0.5 × (1+1×0.5) = 0.75 | > 0.75 |
| Talk(35) → MerchantStand(30) | 0.5 | 0.5 × (1+1×0.75) = 0.875 | > 0.875 |
| Sleep(40) → Eat(40) | 0.5 | 0.5 × (1+1×1.0) = 1.0 | > 1.0 |
| MerchantStand(30) → Talk(35) | 0.5 | 0.5 × (1+1×1.25) = 1.125 | > 1.125 |
| Patrol(10) → Eat(40) | 0.5 | 0.5 × (1+1×2.0) = 1.5 | > 1.5 |

### Rule 3: Exit Conditions (Data-Driven)

After passing inertia, the system also checks if the current action's configured ExitConditions are met (e.g., Hunger < 0.2 for Eat action). This is a supplementary signal, not a gatekeeper.

## Design Rationale

### Why not hard priority gates?

The old system had "higher priority always wins" (Rule 5) and "lower priority never interrupts" (Rule 1 with escape clauses). This caused:

1. **Ping-ponging**: Talk(Social=35) freely interrupted MerchantStand(Work=30) via priority, then MerchantStand took back via escape clause when Talk's score dropped. Inertia (Rule 3) only applied to same-priority pairs, so it never fired.
2. **Hardcoded thresholds**: The escape clause used `bCurrentIsWeak (score < 0.4)` and `bCandidateIsOverwhelming (score > 2×current)` — magic numbers unrelated to InertiaBonus.
3. **InertiaBonus was decorative**: Configured to 1.0 (100%) in DataTable but only applied when priorities matched exactly, which rarely happened between the actions that actually oscillated.

### Why Priority-Scaled Inertia?

- **Single formula**: One equation replaces 5 separate rules
- **Priority is meaningful**: Higher priority = lower switching cost, not a binary gate
- **InertiaBonus actually works**: The DataTable value directly controls ALL transitions
- **Tunable**: Change `/20.0` divisor to adjust how much priority matters vs inertia
- **Predictable**: Given two scores and two priorities, the outcome is deterministic and loggable

### The `/20.0` Divisor

This value means a priority difference of 20 fully cancels inertia. In the current priority scheme:

- Threat(50) vs Work(30) = diff 20 → zero inertia (instant combat response)
- Needs(40) vs Work(30) = diff 10 → half inertia
- Social(35) vs Work(30) = diff 5 → 75% inertia (prevents Talk/Work ping-pong)
- Same priority = diff 0 → full inertia

Adjusting this value:
- **Smaller** (e.g., `/10.0`): Priority matters more, smaller gaps cancel inertia
- **Larger** (e.g., `/40.0`): Priority matters less, only huge gaps reduce inertia

## Configuration

### DataTable Columns (DT_NPC_Actions)

| Column | Type | Description |
|--------|------|-------------|
| Priority | EActionPriority | Action priority level |
| InertiaBonus | float | Inertia multiplier (0.0 = no inertia, 1.0 = need 2× score at same priority) |
| CommitmentTime | float | Seconds of guaranteed execution after Enter() |
| ExitConditions | TArray | Data-driven conditions for voluntary exit |
| CooldownTime | float | Seconds of lockout after Exit() |

### Current Values

| Action | Priority | InertiaBonus | CommitmentTime |
|--------|----------|-------------|----------------|
| Attack | Threat(50) | 1.0 | 0s |
| Flee | Threat(50) | 1.0 | 0s |
| Idle | Idle(10) | 1.0 | 0s |
| Talk | Social(35) | 1.0 | 5s |
| Eat | Needs(40) | 1.0 | 5s |
| Sleep | Needs(40) | 1.0 | 10s |
| Mining | Work(30) | 2.0 | 10s |
| Farming | Work(30) | 2.0 | 10s |
| Guard Patrol | Work(30) | 2.0 | 10s |
| Merchant Stand | Work(30) | 2.0 | 20s |
| Scavenging | Work(30) | 2.0 | 5s |
| Praying | Work(30) | 2.0 | 30s |
| Scholarly Study | Work(30) | 2.0 | 20s |

## Debug Output

```
[Transition] ⚠️ Current Action Sleep Score is low (0.05) - Forcing Yield to Patrol
[Transition] Denied: Talk blocked by MerchantStand Commitment (12.0s left)
[Transition] ⚡ Emergency: Attack(Threat+) breaks MerchantStand Commitment!
[Transition] Denied: Talk(P:35, 0.70) <= MerchantStand(P:30, 0.50 * (1+2.0*0.75)=1.25)
[Transition] Allowed: Attack(P:50, 0.80) > MerchantStand(P:30, threshold=0.50, scale=0.00)
```

## Changelog

| Date | Change |
|------|--------|
| 2026-02-05 | Original system: Priority Gate + Same-Priority Inertia + Escape Clauses |
| 2026-02-12 | Replaced with Priority-Scaled Inertia: single formula, priority modulates threshold |
