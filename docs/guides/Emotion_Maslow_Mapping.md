# Emotion-Maslow Mapping Guide / 情绪-马斯洛映射指南

## Overview / 概述

This document explains how **Emotions** (displayed as emoji) map to **Maslow's Hierarchy of Needs** (internal mental state).

本文档解释**情绪**（显示为 emoji）如何映射到**马斯洛需求层次**（内部心理状态）。

## Architecture / 架构

```
Maslow 需求层次 (Internal State)
    ↓ LLM 综合判断
Emotion 标签 (External Display)
    ↓ 查找 DataAsset
Emoji 图片 (Visual Representation)
```

## Emotion-Maslow Mapping / 情绪-马斯洛映射

### 生理需求层 (Physiological Needs) - 最高优先级

| Maslow Variable | Condition | Emotion | Emoji | Description |
|----------------|-----------|---------|-------|-------------|
| Energy | Low (< 0.3) | **Tired** | 😴 | Exhausted, weary, sleepy |
| Hunger | High (> 0.7) | **Hungry** | 😫 | Starving, desperate for food |

**中文:**
| 马斯洛变量 | 条件 | 情绪 | Emoji | 描述 |
|-----------|------|------|-------|------|
| Energy | 低 (< 0.3) | **Tired** | 😴 | 精疲力竭、疲惫、困倦 |
| Hunger | 高 (> 0.7) | **Hungry** | 😫 | 饥饿、急需食物 |

### 安全需求层 (Safety Needs)

| Maslow Variable | Condition | Emotion | Emoji | Description |
|----------------|-----------|---------|-------|-------------|
| Perceived_Threat | High (> 0.7) | **Scared** | 😨 | Terrified, afraid, panicked |
| Resource_Anxiety | High (> 0.7) | **Anxious** | 😰 | Worried, stressed, insecure |

**中文:**
| 马斯洛变量 | 条件 | 情绪 | Emoji | 描述 |
|-----------|------|------|-------|------|
| Perceived_Threat | 高 (> 0.7) | **Scared** | 😨 | 恐惧、害怕、惊慌 |
| Resource_Anxiety | 高 (> 0.7) | **Anxious** | 😰 | 焦虑、紧张、不安 |

### 社交需求层 (Love/Belonging Needs)

| Maslow Variable | Condition | Emotion | Emoji | Description |
|----------------|-----------|---------|-------|-------------|
| Loneliness | High (> 0.7) | **Sad** | 😢 | Depressed, isolated, lonely |
| Trust | Low (< 0.3) | **Suspicious** | 🤨 | Distrustful, wary, doubtful |
| Trust | High (> 0.7) + No threats | **Happy** | 😊 | Content, pleased, joyful |

**中文:**
| 马斯洛变量 | 条件 | 情绪 | Emoji | 描述 |
|-----------|------|------|-------|------|
| Loneliness | 高 (> 0.7) | **Sad** | 😢 | 沮丧、孤立、孤独 |
| Trust | 低 (< 0.3) | **Suspicious** | 🤨 | 不信任、警惕、怀疑 |
| Trust | 高 (> 0.7) + 无威胁 | **Happy** | 😊 | 满足、愉快、快乐 |

### 尊严需求层 (Esteem Needs)

| Maslow Variable | Condition | Emotion | Emoji | Description |
|----------------|-----------|---------|-------|-------------|
| Anger | High (> 0.7) | **Angry** | 😠 | Furious, enraged, hostile |
| Social_Status | High (> 0.7) | **Proud** | 😌 | Confident, satisfied, accomplished |

**中文:**
| 马斯洛变量 | 条件 | 情绪 | Emoji | 描述 |
|-----------|------|------|-------|------|
| Anger | 高 (> 0.7) | **Angry** | 😠 | 愤怒、暴怒、敌对 |
| Social_Status | 高 (> 0.7) | **Proud** | 😌 | 自信、满足、有成就感 |

### 自我实现需求层 (Self-Actualization Needs)

| Maslow Variable | Condition | Emotion | Emoji | Description |
|----------------|-----------|---------|-------|-------------|
| Curiosity | High (> 0.7) | **Curious** | 🤔 | Interested, inquisitive, wondering |
| Duty_Urgency | High (> 0.7) | **Determined** | 😤 | Focused, driven, purposeful |

**中文:**
| 马斯洛变量 | 条件 | 情绪 | Emoji | 描述 |
|-----------|------|------|-------|------|
| Curiosity | 高 (> 0.7) | **Curious** | 🤔 | 好奇、探究、疑惑 |
| Duty_Urgency | 高 (> 0.7) | **Determined** | 😤 | 专注、坚定、有目标 |

### 其他情绪 (Other Emotions)

| Situation | Emotion | Emoji | Description |
|-----------|---------|-------|-------------|
| Situation unclear | **Confused** | 😕 | Puzzled, uncertain, bewildered |
| Positive opportunity | **Excited** | 😃 | Enthusiastic, eager, thrilled |
| All needs satisfied | **Neutral** | 😐 | Calm, indifferent, composed |

**中文:**
| 情况 | 情绪 | Emoji | 描述 |
|------|------|-------|------|
| 情况不明 | **Confused** | 😕 | 困惑、不确定、迷茫 |
| 积极机会 | **Excited** | 😃 | 兴奋、热切、激动 |
| 所有需求满足 | **Neutral** | 😐 | 平静、淡然、镇定 |

## Selection Priority / 选择优先级

**Maslow's Hierarchy Priority (从高到低):**

1. **生理需求** (Physiological) - Tired, Hungry
2. **安全需求** (Safety) - Scared, Anxious
3. **社交需求** (Love/Belonging) - Sad, Suspicious, Happy
4. **尊严需求** (Esteem) - Angry, Proud
5. **自我实现** (Self-Actualization) - Curious, Determined

**规则 / Rule:**
选择**最紧迫的未满足需求**对应的情绪。
Choose the emotion corresponding to the **most urgent unsatisfied need**.

### Examples / 示例

**Example 1: 饥饿的僵尸 / Hungry Zombie**
```
Maslow State:
- Hunger: 0.9 (High)
- Perceived_Threat: 0.0 (None)
- Anger: 0.8 (High)

Priority: Physiological > Esteem
Result: Emotion = "Hungry" 😫
```

**Example 2: 受威胁的守卫 / Threatened Guard**
```
Maslow State:
- Hunger: 0.3 (Low)
- Perceived_Threat: 0.9 (High)
- Duty_Urgency: 0.8 (High)

Priority: Safety > Self-Actualization
Result: Emotion = "Scared" 😨
```

**Example 3: 满足的商人 / Satisfied Merchant**
```
Maslow State:
- Hunger: 0.1 (Low)
- Perceived_Threat: 0.0 (None)
- Trust: 0.8 (High)
- Social_Status: 0.7 (High)

Priority: All needs satisfied
Result: Emotion = "Happy" 😊
```

## Complete Emotion List / 完整情绪列表

### 需要在 DA_EmojiConfig 中配置的 Emotion:

```
1. Tired (😴) - Energy low
2. Hungry (😫) - Hunger high
3. Scared (😨) - Perceived_Threat high
4. Anxious (😰) - Resource_Anxiety high
5. Sad (😢) - Loneliness high
6. Suspicious (🤨) - Trust low
7. Happy (😊) - Trust high + no threats
8. Angry (😠) - Anger high
9. Proud (😌) - Social_Status high
10. Curious (🤔) - Curiosity high
11. Determined (😤) - Duty_Urgency high
12. Confused (😕) - Situation unclear
13. Excited (😃) - Positive opportunity
14. Neutral (😐) - Default/all satisfied
```

## Implementation Checklist / 实现清单

### 1. 创建 Emoji 图片资源

需要准备 14 个 emoji 图片:
- T_Emoji_Tired.png
- T_Emoji_Hungry.png
- T_Emoji_Scared.png
- T_Emoji_Anxious.png
- T_Emoji_Sad.png
- T_Emoji_Suspicious.png
- T_Emoji_Happy.png
- T_Emoji_Angry.png
- T_Emoji_Proud.png
- T_Emoji_Curious.png
- T_Emoji_Determined.png
- T_Emoji_Confused.png
- T_Emoji_Excited.png
- T_Emoji_Neutral.png

### 2. 配置 DA_EmojiConfig

在 DataAsset 中添加所有 14 个 emoji 配置。

### 3. 测试映射

测试不同的 Maslow 状态是否产生正确的 Emotion:

```
Test Case 1: Zombie sees player
- Expected: Perceived_Threat: High → Emotion: "Scared"

Test Case 2: Zombie is hungry
- Expected: Hunger: High → Emotion: "Hungry"

Test Case 3: Guard on duty
- Expected: Duty_Urgency: High → Emotion: "Determined"
```

## Summary / 总结

**现在的系统:**
- ✅ Maslow 变量 (10个) - 内部需求状态
- ✅ Emotion 标签 (14个) - 外部情绪表现
- ✅ 清晰的映射规则 - 基于马斯洛层次优先级
- ✅ LLM 智能选择 - 综合考虑所有因素

**优势:**
- ✅ 心理学上合理 - 基于马斯洛需求理论
- ✅ 优先级明确 - 生理需求 > 安全 > 社交 > 尊严 > 自我实现
- ✅ 易于理解 - 每个 Emotion 对应明确的 Maslow 变量
- ✅ 可扩展 - 可以添加更多 Emotion 或 Maslow 变量

