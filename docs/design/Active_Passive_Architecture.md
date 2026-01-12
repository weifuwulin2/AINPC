# Active vs. Passive AI Modes: A Neuro-Symbolic Arbitration Mechanism
# 主动与被动模式：一种神经符号仲裁机制

**Author:** [Your Name]
**Date:** 2026-01-11
**Status:** Architecture Draft

---

## 1. Abstract / 摘要

Current Game AI architectures often struggle with the "Handover Problem" between LLM-driven planning (High Agency) and Utility-driven survival (Low Agency). This document proposes a dynamic **Arbitration Mechanism** that switches the agent between two distinct cognitive modes: **Active (Deliberative)** and **Passive (Reactive)**.

目前的游戏 AI 架构常在 LLM 驱动的规划（高代理权）与 Utility 驱动的生存（低代理权）之间的“切换问题”上挣扎。本文档提出了一种动态**仲裁机制**，使智能体能够在两种截然不同的认知模式之间切换：**主动（慎思型）**和**被动（反应型）**。

---

## 2. The Duality of Agency / 代理权的二元性

### 2.1 Active Mode (Deliberative) / 主动模式
*   **Trigger**: Stable environment, clear long-term goals, or LLM `Intention` signal is strong.
*   **Mechanism**: The Utility AI serves the LLM. 
    *   Actions matching the LLM's `Intention` receive a **Dominant Weighting** (e.g., +2.0 Score).
    *   The agent is "Goal-Oriented". (e.g., "I must go talk to the King", ignoring minor hunger).
*   **User Experience**: The NPC feels distinct, opinionated, and driven by personality.

### 2.2 Passive Mode (Reactive) / 被动模式
*   **Trigger**: Immediate threat (`Perceived_Threat` > 0.8), critical physiological needs (`Hunger` > 0.9), or combat.
*   **Mechanism**: The Utility AI overrides the LLM. 
    *   The LLM's `Intention` is treated as a weak suggestion (or ignored).
    *   Survival instincts take precedence.
*   **User Experience**: The NPC feels intelligent, responsive, and realistic.

---

## 3. The Arbitration Logic / 仲裁逻辑

The transition is governed by a `Cognitive Load` arbitration curve:

```cpp
EAIExecutionMode Arbiter(const FAttributeState& State, const FIntention& Intention)
{
    // 1. Critical Override (Survival Reflex)
    // 1. 紧急覆盖（生存反射）
    if (State.Perceived_Threat > 0.7f || State.Hunger > 0.9f)
    {
        return EAIExecutionMode::Passive;
    }

    // 2. Intention Strength (Goal Clarity)
    // 2. 意图强度（目标清晰度）
    if (!Intention.IsEmpty() && Intention.Confidence > 0.6f)
    {
        return EAIExecutionMode::Active;
    }

    // 3. Default Drift
    // 3. 默认漂移
    return EAIExecutionMode::Passive;
}
```

---

## 4. Implementation Strategy / 实现策略

### Phase 1: Explicit Mode Tracking (Current Step)
*   Add `EAIExecutionMode` to `UtilityAIComponent`.
*   Log the current mode clearly in debug output.
*   Vizualize the mode switch.

### Phase 2: Dynamic Weighting
*   **Active**: `TotalScore = UtilityScore * 0.5 + IntentionScore * 2.0`
*   **Passive**: `TotalScore = UtilityScore * 1.0 + IntentionScore * 0.2`

---

## 5. Academic Value / 学术价值

This architecture goes beyond simple "State Machines". It represents a **Variable Autonomy System**.
*   **Research Question**: How does the *ratio* of Active/Passive time affect player perception of intelligence?
*   **Hypothesis**: Players perceive "Soul" during Active moments, but perceive "Intelligence" during Passive moments.

---
