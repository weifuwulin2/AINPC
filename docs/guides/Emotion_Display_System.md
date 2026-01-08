# Emotion Display System Guide / 情绪显示系统指南

## Overview / 概述

The Emotion Display System shows emoji and speech bubbles above NPCs based on their emotional state returned by the LLM.

情绪显示系统根据 LLM 返回的情绪状态在 NPC 头顶显示 emoji 和对话泡泡。

## Architecture / 架构

```
LLM 返回 JSON:
{
  "Hunger": "None",
  "Anger": "Furious",
  ...
  "Emotion": "Angry"  ← 新增字段
}
    ↓
LLMCommunicator 解析 Emotion 字段
    ↓
CognitionComponent.OnMentalStateChanged 广播事件
    ↓
EmotionDisplayComponent 接收事件
    ↓
显示对应的 Emoji Widget (😠)
```

## Setup / 设置

### Step 1: Create Emoji Widget Blueprint / 创建 Emoji Widget 蓝图

1. In Content Browser, create a new **Widget Blueprint**
2. Name it `WBP_EmojiDisplay`
3. Add a **TextBlock** widget named `EmojiText`
4. Set the TextBlock properties:
   - **Font Size**: 48-72 (large enough to see)
   - **Justification**: Center
   - **Alignment**: Center
5. Optional: Add a background image or border

在内容浏览器中，创建一个新的 **Widget Blueprint**
命名为 `WBP_EmojiDisplay`
添加一个名为 `EmojiText` 的 **TextBlock** widget
设置 TextBlock 属性:
- **字体大小**: 48-72 (足够大以便看清)
- **对齐**: 居中
- **对齐方式**: 居中
可选: 添加背景图片或边框

### Step 2: Create Speech Bubble Widget Blueprint / 创建对话泡泡 Widget 蓝图

1. Create another **Widget Blueprint**
2. Name it `WBP_SpeechBubble`
3. Add a **TextBlock** widget named `MessageText`
4. Add a background image (speech bubble shape)
5. Set the TextBlock properties:
   - **Font Size**: 16-24
   - **Justification**: Center
   - **Wrap Text**: Enabled

创建另一个 **Widget Blueprint**
命名为 `WBP_SpeechBubble`
添加一个名为 `MessageText` 的 **TextBlock** widget
添加背景图片(对话泡泡形状)
设置 TextBlock 属性

### Step 3: Add EmotionDisplayComponent to AI Controller / 将 EmotionDisplayComponent 添加到 AI 控制器

1. Open your AI Controller Blueprint (e.g., `BP_UtilityAIController`)
2. Add **EmotionDisplayComponent** component
3. Set the component properties:
   - **Emoji Widget Class**: `WBP_EmojiDisplay`
   - **Speech Bubble Widget Class**: `WBP_SpeechBubble`
   - **Emoji Display Duration**: 3.0 seconds (adjust as needed)
   - **Speech Bubble Display Duration**: 5.0 seconds
   - **Height Offset**: 100.0 cm (adjust based on character height)

打开你的 AI 控制器蓝图
添加 **EmotionDisplayComponent** 组件
设置组件属性

### Step 4: Compile and Test / 编译并测试

1. Compile C++ code
2. Compile Blueprints
3. Play in Editor (PIE)
4. Trigger an event that changes the NPC's emotion (e.g., see the player)
5. You should see an emoji appear above the NPC's head

编译 C++ 代码
编译蓝图
在编辑器中播放
触发改变 NPC 情绪的事件
你应该看到 emoji 出现在 NPC 头顶

## Emotion Tags / 情绪标签

The LLM can return the following emotion tags:

LLM 可以返回以下情绪标签:

| Emotion Tag | Emoji | Description |
|-------------|-------|-------------|
| Happy       | 😊    | Content, pleased, joyful |
| Angry       | 😠    | Furious, enraged, hostile |
| Scared      | 😨    | Terrified, afraid, panicked |
| Sad         | 😢    | Depressed, sorrowful, disappointed |
| Confused    | 😕    | Puzzled, uncertain, bewildered |
| Excited     | 😃    | Enthusiastic, eager, thrilled |
| Neutral     | 😐    | Calm, indifferent, composed |
| Suspicious  | 🤨    | Distrustful, wary, doubtful |
| Curious     | 🤔    | Interested, inquisitive, wondering |
| Tired       | 😴    | Exhausted, weary, sleepy |

## Example LLM Output / LLM 输出示例

### Zombie Sees Player / 僵尸看到玩家

**Input:**
```
Current Situation: I saw Player
```

**LLM Output:**
```json
{
  "Hunger": "Starving",
  "Energy": "None",
  "Perceived_Threat": "Extreme",
  "Resource_Anxiety": "None",
  "Loneliness": "None",
  "Trust": "None",
  "Anger": "Furious",
  "Social_Status": "None",
  "Duty_Urgency": "None",
  "Curiosity": "None",
  "Emotion": "Angry"
}
```

**Result:** 😠 emoji appears above zombie's head

**结果:** 😠 emoji 出现在僵尸头顶

### Friendly NPC Sees Player / 友好 NPC 看到玩家

**Input:**
```
Current Situation: I saw Player
```

**LLM Output:**
```json
{
  "Hunger": "None",
  "Energy": "None",
  "Perceived_Threat": "None",
  "Resource_Anxiety": "None",
  "Loneliness": "Slight",
  "Trust": "Trusting",
  "Anger": "None",
  "Social_Status": "Moderate",
  "Duty_Urgency": "None",
  "Curiosity": "Moderate",
  "Emotion": "Happy"
}
```

**Result:** 😊 emoji appears above NPC's head

**结果:** 😊 emoji 出现在 NPC 头顶

## Advanced Usage / 高级用法

### Manually Show Speech Bubble / 手动显示对话泡泡

You can manually trigger speech bubbles from Blueprint or C++:

你可以从蓝图或 C++ 手动触发对话泡泡:

**Blueprint:**
```
Get EmotionDisplayComponent
  ↓
Show Speech Bubble (Message: "Hello, traveler!")
```

**C++:**
```cpp
UEmotionDisplayComponent* EmotionComp = AIController->FindComponentByClass<UEmotionDisplayComponent>();
if (EmotionComp)
{
    EmotionComp->ShowSpeechBubble(TEXT("Hello, traveler!"));
}
```

### Customize Emoji Mapping / 自定义 Emoji 映射

To add new emotions or change emoji mappings, edit `EmotionDisplayComponent.cpp`:

要添加新情绪或更改 emoji 映射，编辑 `EmotionDisplayComponent.cpp`:

```cpp
FString UEmotionDisplayComponent::GetEmojiForEmotion(const FString& Emotion) const
{
    if (Emotion == TEXT("Hungry"))
    {
        return TEXT("🍔"); // Custom emoji for hunger
    }
    // ... add more mappings
}
```

Then update the LLM prompt in `CognitionComponent.cpp` to include the new emotion tag.

然后在 `CognitionComponent.cpp` 中更新 LLM 提示词以包含新的情绪标签。

### Widget Customization / Widget 自定义

You can customize the appearance of widgets:

你可以自定义 widget 的外观:

1. **Emoji Widget**:
   - Add animations (fade in/out, bounce)
   - Add particle effects
   - Change font, color, size
   
2. **Speech Bubble Widget**:
   - Add tail/pointer to bubble
   - Add character portrait
   - Add typing animation
   - Support rich text formatting

## Troubleshooting / 故障排除

### Emoji not showing / Emoji 不显示

1. **Check Widget is created**:
   - Look for log: `[EmotionDisplay] Emoji Widget Component created`
   - If missing, check `EmojiWidgetClass` is set
   
   检查 Widget 是否创建
   - 查找日志: `[EmotionDisplay] Emoji Widget Component created`
   - 如果缺失，检查 `EmojiWidgetClass` 是否设置

2. **Check emotion is received**:
   - Look for log: `[Emotion] Angry` (or other emotion)
   - If missing, check LLM is returning `Emotion` field
   
   检查是否接收到情绪
   - 查找日志: `[Emotion] Angry`
   - 如果缺失，检查 LLM 是否返回 `Emotion` 字段

3. **Check Widget visibility**:
   - Widget might be hidden behind character
   - Adjust `HeightOffset` property
   - Check `SetWidgetSpace` is set to `Screen`
   
   检查 Widget 可见性
   - Widget 可能被角色遮挡
   - 调整 `HeightOffset` 属性
   - 检查 `SetWidgetSpace` 是否设置为 `Screen`

### Emoji shows wrong symbol / Emoji 显示错误符号

1. **Check TextBlock name**:
   - Widget must have a TextBlock named `EmojiText`
   - Case-sensitive!
   
   检查 TextBlock 名称
   - Widget 必须有一个名为 `EmojiText` 的 TextBlock
   - 区分大小写!

2. **Check font supports emoji**:
   - Some fonts don't support emoji characters
   - Use a font that supports Unicode emoji (e.g., Segoe UI Emoji, Noto Color Emoji)
   
   检查字体是否支持 emoji
   - 某些字体不支持 emoji 字符
   - 使用支持 Unicode emoji 的字体

### Widget position is wrong / Widget 位置错误

1. **Adjust HeightOffset**:
   - Default is 100cm, adjust based on character height
   - Taller characters need higher offset
   
   调整 HeightOffset
   - 默认是 100cm，根据角色高度调整
   - 更高的角色需要更高的偏移

2. **Check attachment**:
   - Widget should be attached to character's root component
   - Check logs for attachment errors
   
   检查附加
   - Widget 应该附加到角色的根组件
   - 检查日志中的附加错误

## Related Files / 相关文件

- `EmotionDisplayComponent.h` - Component header
- `EmotionDisplayComponent.cpp` - Component implementation
- `LLMCommunicator.h` - Added `Emotion` field to `FMentalState`
- `LLMCommunicator.cpp` - Parse `Emotion` from LLM JSON
- `CognitionComponent.cpp` - Updated LLM prompt to request `Emotion` field
- `WBP_EmojiDisplay` - Emoji widget blueprint (create this)
- `WBP_SpeechBubble` - Speech bubble widget blueprint (create this)

## Future Enhancements / 未来增强

- **Animated Emoji**: Use animated textures or flipbooks
- **3D Emoji**: Use 3D meshes instead of 2D widgets
- **Voice Synthesis**: Generate voice based on emotion
- **Emotion History**: Show emotion changes over time
- **Multi-Emotion**: Show multiple emotions simultaneously
- **Context-Aware Messages**: Generate speech bubble content based on situation

