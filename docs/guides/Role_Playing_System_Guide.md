# Role-Playing System Configuration Guide
# 角色扮演系统配置指南

This document explains how to configure the Role-Playing System (RPS) for NPCs. The RPS allows you to inject specific personas and behavioral rules into the LLM, enabling unique and consistent character behaviors.
本文档解释了如何配置 NPC 的角色扮演系统 (RPS)。RPS 允许您将特定的人设和行为准则注入到 LLM 中，从而实现独特且一致的角色行为。

## 1. Overview / 概述

The Role-Playing System works by modifying the "System Prompt" sent to the LLM. It consists of two main components:
角色扮演系统通过修改发送给 LLM 的 "System Prompt" 来工作。它由两个主要组件组成：

*   **Role Description (角色描述)**: Defines *WHO* the NPC is.
    *   定义 NPC *是* 谁。
*   **Behavioral Guidelines (行为准则)**: Defines *HOW* the NPC should act or think.
    *   定义 NPC 应该 *如何* 行动或思考。

These fields are stored in the `FPersonalityConfig` struct and can be configured via C++ Presets or DataTables.
这些字段存储在 `FPersonalityConfig`结构体中，可以通过 C++ 预设或数据表进行配置。

---

## 2. Configuration Fields / 配置字段

### Role Description (角色描述)

*   **Goal**: To immerse the LLM in a specific character.
*   **目标**: 让 LLM 沉浸在特定的角色中。
*   **Best Practice**: Use vivid, first-person or second-person descriptions. Mention their background, motivation, and key personality traits.
*   **最佳实践**: 使用生动的、第一人称或第二人称的描述。提及他们的背景、动机和关键性格特征。

**Example (Zombie):**
> "You are a mindless zombie infected by a virus. You have an insatiable hunger for living flesh. You feel no pain and no fear."

**Example (Guard):**
> "You are a vigilant royal guard. Your duty is to protect the castle gate. You are suspicious of strangers but polite to nobles."

### Behavioral Guidelines (行为准则)

*   **Goal**: To force specific logic mappings onto the LLM, especially for converting vague concepts (like "Hunger") into concrete Utility AI emotions (like "Anger").
*   **目标**: 强制 LLM 执行特定的逻辑映射，特别是将模糊的概念（如“饥饿”）转化为具体的 Utility AI 情绪（如“愤怒”）。
*   **Best Practice**: Use bullet points. Explicitly link narrative states to emotional outputs (`Anger`, `Fear`, `Trust`, etc.).
*   **最佳实践**: 使用要点符号。明确地将叙事状态链接到情绪输出。

**Example (Zombie Logic):**
> 1. Attack any human immediately.
> 2. Your insatiable hunger drives you into a 'Furious' rage (**Extreme Anger**).
> 3. You are mindless, so your 'Perceived_Threat' is always 'None'.
> 4. Your 'Trust' is always 'None'.

---

## 3. How to Configure / 如何配置

### Direct C++ Configuration (Fast Testing)

In `PersonalityConfig.h`, you can define static presets:

在 `PersonalityConfig.h` 中，您可以定义静态预设：

```cpp
static FPersonalityConfig Zombie()
{
    FPersonalityConfig Config;
    // ... Set OCEAN traits ...
    
    Config.RoleDescription = TEXT("You are a mindless zombie...");
    Config.BehavioralGuidelines = TEXT("1. Attack any human...");
    
    return Config;
}
```

### DataTable Configuration (Production) (Planned)

In your Personality DataTable (e.g., `DT_Personalities`), ensure you have columns for `RoleDescription` and `BehavioralGuidelines`.
在您的个性数据表（例如 `DT_Personalities`）中，确保有 `RoleDescription` 和 `BehavioralGuidelines` 列。

---

## 4. Jurisdiction & Architecture / 管辖权与架构

Important: The LLM acts as a "Cognitive Layer". It outputs **Emotional Target Values**. It does **NOT** directly control the Pawn's movement or modify Engine-Managed values like actual Health or Hunger.
重要提示：LLM 充当“认知层”。它输出 **情绪目标值**。它 **不** 直接控制 Pawn 的移动或修改引擎管理的值（如实际生命值或饥饿度）。

*   **Engine Exclusive (引擎独占)**: `Hunger`, `Energy`, `Health`. (The LLM cannot change these, even if it says "I am hungry" in the prompt. The code explicitly filters these out).
*   **LLM Influenced (LLM 影响)**: `Anger`, `Perceived_Threat`, `Trust`, `Curiosity`. (The LLM's output directly drives these Utility AI inputs).

### Example Flow: The Zombie Attack
### 示例流程：僵尸攻击

1.  **Input**: NPC sees a Player.
2.  **Prompt**: Includes "You are a zombie... Hunger drives Anger."
3.  **LLM Output**:
    *   `Anger`: "Extreme" (Because of the guideline).
    *   `Perceived_Threat`: "None" (Because zombies aren't afraid).
4.  **Utility AI**:
    *   `Anger` scores high -> **Attack Action** gets a high score.
    *   `Fear` scores low -> **Flee Action** gets a low score.
5.  **Result**: The Zombie attacks the player fearlessly.
