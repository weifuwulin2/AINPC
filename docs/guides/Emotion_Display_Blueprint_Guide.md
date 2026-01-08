# Emotion Display Blueprint Guide / 情绪显示蓝图指南

## Overview / 概述

This guide shows how to display emoji and speech bubbles in **Blueprint** using the `Emotion` and `Speech` data from the LLM.

本指南展示如何在**蓝图**中使用 LLM 返回的 `Emotion` 和 `Speech` 数据显示 emoji 和对话泡泡。

## Why Blueprint? / 为什么用蓝图？

- ✅ **更灵活** - 可视化编辑 UI
- ✅ **更直观** - 实时预览效果
- ✅ **更容易调试** - 可以直接看到 Widget
- ✅ **更好的 Widget 支持** - UMG 在蓝图中更好用

## Data Flow / 数据流

```
LLM 返回 JSON
    ↓
LLMCommunicator 解析
    ↓
CognitionComponent 广播 OnMentalStateChanged 事件
    ↓
Blueprint 监听事件 ← 你在这里处理 UI
    ↓
显示 Emoji Widget + Speech Bubble Widget
```

## Setup Steps / 设置步骤

### Step 1: Create Emoji Widget / 创建 Emoji Widget

1. Create a new **Widget Blueprint** named `WBP_EmojiDisplay`
2. Add a **Text** widget
3. Set properties:
   - Font Size: 64
   - Justification: Center
   - Color: White

创建一个新的 **Widget Blueprint**，命名为 `WBP_EmojiDisplay`
添加一个 **Text** widget
设置属性

### Step 2: Create Speech Bubble Widget / 创建对话泡泡 Widget

1. Create a new **Widget Blueprint** named `WBP_SpeechBubble`
2. Add a **Border** or **Image** (for bubble background)
3. Add a **Text** widget inside
4. Set properties:
   - Font Size: 18-24
   - Wrap Text: Enabled
   - Padding: 10-20

创建一个新的 **Widget Blueprint**，命名为 `WBP_SpeechBubble`
添加一个 **Border** 或 **Image**（作为泡泡背景）
在里面添加一个 **Text** widget

### Step 3: Blueprint Event Graph / 蓝图事件图

Open your **AI Controller Blueprint** (e.g., `BP_UtilityAIController`):

打开你的 **AI 控制器蓝图**（例如 `BP_UtilityAIController`）:

#### 3.1 Add Variables / 添加变量

Add these variables:
- `EmojiWidget` (Type: `Widget Component`)
- `SpeechBubbleWidget` (Type: `Widget Component`)
- `CurrentEmotion` (Type: `String`)
- `CurrentSpeech` (Type: `String`)

添加这些变量

#### 3.2 BeginPlay Event / BeginPlay 事件

```
Event BeginPlay
    ↓
Add Widget Component (Class: Widget Component)
    - Name: "EmojiWidget"
    - Widget Class: WBP_EmojiDisplay
    - Widget Space: Screen
    - Draw Size: (100, 100)
    - Relative Location: (0, 0, 100)  // Above head
    - Visibility: Hidden
    ↓
Attach to Component (Root)
    ↓
Add Widget Component (Class: Widget Component)
    - Name: "SpeechBubbleWidget"
    - Widget Class: WBP_SpeechBubble
    - Widget Space: Screen
    - Draw Size: (300, 100)
    - Relative Location: (0, 0, 150)  // Higher than emoji
    - Visibility: Hidden
    ↓
Attach to Component (Root)
```

#### 3.3 Bind to OnMentalStateChanged Event / 绑定到 OnMentalStateChanged 事件

```
Event BeginPlay
    ↓
Get CognitionComp
    ↓
Bind Event to OnMentalStateChanged
    ↓
[Custom Event: OnEmotionChanged]
```

#### 3.4 OnEmotionChanged Custom Event / OnEmotionChanged 自定义事件

**Inputs:**
- `NewState` (Type: `FMentalState`)

**Logic:**

```
OnEmotionChanged (NewState)
    ↓
Branch: Is NewState.Emotion NOT Empty?
    ↓ True
    Get Emoji for Emotion (NewState.Emotion)
        ↓
    Set Text (EmojiWidget -> WBP_EmojiDisplay -> Text)
        ↓
    Set Visibility (EmojiWidget -> Visible)
        ↓
    Set Timer by Function Name
        - Function: "HideEmoji"
        - Time: 3.0
        - Looping: False
    
    ↓
Branch: Is NewState.Speech NOT Empty?
    ↓ True
    Set Text (SpeechBubbleWidget -> WBP_SpeechBubble -> Text)
        ↓
    Set Visibility (SpeechBubbleWidget -> Visible)
        ↓
    Set Timer by Function Name
        - Function: "HideSpeechBubble"
        - Time: 5.0
        - Looping: False
```

#### 3.5 Helper Functions / 辅助函数

**Function: GetEmojiForEmotion**

Inputs: `Emotion` (String)
Outputs: `EmojiText` (String)

```
Switch on String (Emotion)
    Case "Happy":    Return "😊"
    Case "Angry":    Return "😠"
    Case "Scared":   Return "😨"
    Case "Sad":      Return "😢"
    Case "Confused": Return "😕"
    Case "Excited":  Return "😃"
    Case "Neutral":  Return "😐"
    Case "Suspicious": Return "🤨"
    Case "Curious":  Return "🤔"
    Case "Tired":    Return "😴"
    Default:         Return "😐"
```

**Function: HideEmoji**

```
Set Visibility (EmojiWidget -> Hidden)
```

**Function: HideSpeechBubble**

```
Set Visibility (SpeechBubbleWidget -> Hidden)
```

## Blueprint Visual Example / 蓝图可视化示例

### BeginPlay Setup

```
┌─────────────────────────────────────────────┐
│ Event BeginPlay                             │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Add Widget Component                        │
│ - Component Class: Widget Component         │
│ - Return Value → EmojiWidget                │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Set Widget Class                            │
│ - Target: EmojiWidget                       │
│ - Widget Class: WBP_EmojiDisplay            │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Set Widget Space                            │
│ - Target: EmojiWidget                       │
│ - New Space: Screen                         │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Set Relative Location                       │
│ - Target: EmojiWidget                       │
│ - New Location: (0, 0, 100)                 │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Set Visibility                              │
│ - Target: EmojiWidget                       │
│ - New Visibility: Hidden                    │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Get CognitionComp                           │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Bind Event to OnMentalStateChanged          │
│ - Event: OnEmotionChanged                   │
└─────────────────────────────────────────────┘
```

### OnEmotionChanged Event

```
┌─────────────────────────────────────────────┐
│ OnEmotionChanged (NewState: FMentalState)   │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Branch                                      │
│ - Condition: NewState.Emotion != ""         │
└─────┬───────────────────────────────────────┘
      │ True
      ▼
┌─────────────────────────────────────────────┐
│ GetEmojiForEmotion (NewState.Emotion)       │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Get Widget (EmojiWidget)                    │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Cast to WBP_EmojiDisplay                    │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Set Text (Text Widget)                      │
│ - In Text: EmojiText                        │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Set Visibility (EmojiWidget → Visible)      │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│ Set Timer by Function Name                  │
│ - Function Name: "HideEmoji"                │
│ - Time: 3.0                                 │
└─────────────────────────────────────────────┘
```

## Simplified C++ Approach / 简化的 C++ 方案

Since you're using Blueprint for UI, you can **remove EmotionDisplayComponent** from C++.

既然你用蓝图处理 UI，可以**从 C++ 中移除 EmotionDisplayComponent**。

### What to Keep in C++ / C++ 中保留什么

✅ **Keep:**
- `FMentalState.Emotion` field
- `FMentalState.Speech` field
- LLM parsing for these fields
- `CognitionComponent.OnMentalStateChanged` event

❌ **Remove:**
- `EmotionDisplayComponent.h`
- `EmotionDisplayComponent.cpp`
- References in `UtilityAIController`

### Accessing Data in Blueprint / 在蓝图中访问数据

The `OnMentalStateChanged` event gives you a `FMentalState` struct with:
- `Emotion` (String) - e.g., "Angry", "Happy"
- `Speech` (String) - e.g., "Brains... must eat..."
- All other mental state values (floats)

`OnMentalStateChanged` 事件给你一个 `FMentalState` 结构体，包含:
- `Emotion` (字符串) - 例如 "Angry", "Happy"
- `Speech` (字符串) - 例如 "Brains... must eat..."
- 所有其他心理状态值（浮点数）

## Testing / 测试

1. Compile C++ code
2. Open AI Controller Blueprint
3. Implement the event graph as shown above
4. Play in Editor
5. Trigger an event (e.g., zombie sees player)
6. Check logs for:
   ```
   [Emotion] Angry
   [Speech] "Brains... must eat..."
   ```
7. You should see emoji and speech bubble appear

编译 C++ 代码
打开 AI 控制器蓝图
按照上面的方式实现事件图
在编辑器中播放
触发事件（例如僵尸看到玩家）
检查日志
你应该看到 emoji 和对话泡泡出现

## Troubleshooting / 故障排除

### LLM Returns Empty Speech / LLM 返回空对话

Check logs for `[Speech]`. If empty, the LLM might not be generating content.

**Solution:** The improved prompt should prevent this. If it still happens, check:
- LLM is receiving the role description
- Situation description is clear
- LLM API is working correctly

检查日志中的 `[Speech]`。如果为空，LLM 可能没有生成内容。

**解决方案:** 改进的提示词应该能防止这个问题。如果仍然发生，检查:
- LLM 是否接收到角色描述
- 情景描述是否清晰
- LLM API 是否正常工作

### Widget Not Showing / Widget 不显示

1. Check Widget Component is created in BeginPlay
2. Check Widget Class is set
3. Check Visibility is set to Visible
4. Check Widget is attached to character
5. Check height offset (should be above character's head)

检查 Widget 组件是否在 BeginPlay 中创建
检查 Widget 类是否设置
检查可见性是否设置为 Visible
检查 Widget 是否附加到角色
检查高度偏移（应该在角色头顶上方）

### Emoji Shows Wrong Character / Emoji 显示错误字符

1. Check font supports emoji (use Segoe UI Emoji or similar)
2. Check `GetEmojiForEmotion` function returns correct emoji
3. Check `NewState.Emotion` value in logs

检查字体是否支持 emoji
检查 `GetEmojiForEmotion` 函数返回正确的 emoji
检查日志中的 `NewState.Emotion` 值

## Summary / 总结

**C++ 负责:**
- ✅ 数据结构 (`Emotion`, `Speech`)
- ✅ LLM 解析
- ✅ 事件广播

**Blueprint 负责:**
- ✅ Widget 创建
- ✅ UI 显示
- ✅ 动画和效果
- ✅ 定时隐藏

这样分工更清晰，也更灵活！

