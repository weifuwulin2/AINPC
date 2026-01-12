# Context LOD (Level of Detail) System Design

**Version:** 1.0  
**Date:** 2026-01-09  
**Status:** Designing / Implemented (Partially)

## 1. 核心理念：认知呼吸 (Cognitive Breathing)

该系统的核心理念是将**图形渲染中的 LOD (Level of Detail)** 概念引入到 **LLM 认知系统**中。
NPC 的大脑不应该总是全速运转。就像生物呼吸一样，认知应该随着环境压力的变化进行**紧张（吸气/Focus）**与**松弛（呼气/Reflect）**的自动切换。

*   **在高压下 (Combat/Danger)**：大脑屏蔽背景噪音，专注于生存（"Run or Fight"）。
*   **在低压下 (Social/Idle)**：大脑恢复由于从容，能够回忆过去，思考哲学（"Who am I?"）。

---

## 2. LOD 级别定义 (LOD Levels)

我们定义了三个认知精度级别：

### LOD 0: Critical (危急/生存)
*   **适用场景**：战斗、受伤、被恐吓、极端饥饿。
*   **Token 消耗**：极低 (~200 tokens)。
*   **Prompt 特征**：
    *   **包含**：核心职业 (Identity)、当前威胁 (Perception)、身体状态 (Health)。
    *   **排除**：背景故事、社会地位、世界观、复杂动机。
    *   **LLM 指令**："ACT FAST. SURVIVE. Concise tactical intentions."
*   **行为表现**：反应极快，冷酷，动物性本能。

### LOD 1: Standard (标准/任务)
*   **适用场景**：日常探索、交易、工作、普通闲聊。
*   **Token 消耗**：中等 (~500 tokens)。
*   **Prompt 特征**：
    *   **包含**：完整姓名、社会地位、核心动机 (Prime Motivation)、近期记忆 (Recent Memories)。
    *   **排除**：深层背景故事 (Deep Backstory)、复杂的历史传说。
    *   **LLM 指令**："Follow routine. Achieve goals. Stay in character."
*   **行为表现**：正常社会化行为，符合人设。

### LOD 2: Deep (深度/反思)
*   **适用场景**：深度交谈、独处反思、做梦、规划长期目标。
*   **Token 消耗**：高 (~1000+ tokens)。
*   **Prompt 特征**：
    *   **包含**：**一切**。完整背景故事、所有记忆、世界观历史、详细的人际关系网。
    *   **排除**：无。
    *   **LLM 指令**："Reflect deeply. Roleplay complex emotions. Plan for the future."
*   **行为表现**：充满"人性"，有深度，能建立情感连接。

---

## 3. 运作机制 (Mechanism)

### 3.1 自动判定 (`DetermineLOD`)

系统不再依赖不稳定的字符串匹配，而是利用 `SensoryComponent` 产生的结构化数据 `FSemanticEvent`，通过 **GameplayTags** 进行精确判断：

```cpp
ECognitiveLOD DetermineLOD(const FSemanticEvent& Event) {
    // 1. Critical: Check Gameplay Tags
    if (Event.Verb.MatchesTag("Event.Damage") || 
        Event.Verb.MatchesTag("Event.Combat") ||
        Event.Verb.MatchesTag("Perception.Threat.High"))
    {
        return ECognitiveLOD::Critical;
    }
        
    // 2. Deep: Check Tags & Content
    if (Event.Verb.MatchesTag("Social.Chat.Deep") || 
        Event.Verb.MatchesTag("Activity.Reflect"))
    {
        return ECognitiveLOD::Deep;
    }
        
    // 3. State Check (Idle/Dreaming always allows Deep)
    if (GetOwnerState() == EState::Idle && Energy > 0.8) 
        return ECognitiveLOD::Deep;

    return ECognitiveLOD::Standard; // Default
}
```

> **注意**：这要求 `CognitionComponent::ProcessStimulus` 的接口需要升级，从只接收 `FString` 变为接收 `FSemanticEvent` (或其封装).

### 3.2 动态 Prompt 组装 (`AssemblePrompt`)
根据判定出的 LOD，系统会物理裁剪 Prompt 的内容：

| 内容模块 | Critical (LOD 0) | Standard (LOD 1) | Deep (LOD 2) |
| :--- | :--- | :--- | :--- |
| **Identity** | Role Only | Name, Role, Wealth, Status | Full Profile |
| **Motivation** | ❌ | ✅ Prime Motivation | ✅ Full Values |
| **Backstory** | ❌ | ❌ | ✅ Full History |
| **Memory** | 0 items | 3 items | 6+ items |
| **Guidelines**| ❌ (Unless tactical) | ✅ | ✅ |

---

## 4. 与架构的化学反应 (Integration)

### 4.1 对 Utility AI 的影响
*   **Critical Mode**：LLM 输出简单的 Intention（如 "Flee"）。Utility AI 负责纯战术执行（找掩体、计算路径）。
*   **Deep Mode**：LLM 输出复杂的 Intention（如 "Confess Love"）。Utility AI 将其映射为 "Talk" 动作，并附带相应的情绪参数。

### 4.2 对 Interpolator (情绪插值) 的影响
*   **Critical Mode**：建议将情绪插值速度 (`InterpSpeed`) 调高，模拟肾上腺素飙升。
*   **Deep Mode**：建议将情绪插值速度调低，模拟情绪的缓慢酝酿。

---

## 5. 数据结构参考

```cpp
struct FCharacterProfile {
    FString Name;
    FString Role;
    FString PrimeMotivation;
    FString WealthLevel;
    FString SocialStatus;
    FString Backstory; // Only used in Deep LOD
};
```

此设计旨在解决"背景故事越写越长，导致 LLM 在战斗时反应迟钝或产生幻觉"的问题，确保 NPC 在任何情境下都能表现得体且高效。
