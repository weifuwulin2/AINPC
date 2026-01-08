# WBP_EmojiDisplay 配置指南 / Configuration Guide

## 概述 / Overview

`WBP_EmojiDisplay` 是显示在 NPC 头顶的 emoji Widget。它需要一个 **Image** widget 来显示 emoji 图片。

## 步骤 / Steps

### Step 1: 创建 Widget Blueprint

1. 在 Content Browser 中右键
2. 选择 **User Interface** → **Widget Blueprint**
3. 命名为 `WBP_EmojiDisplay`
4. 双击打开

### Step 2: 设置 Canvas Panel

默认会有一个 Canvas Panel 作为根节点。保持它。

### Step 3: 添加 Image Widget

1. 从左侧 **Palette** 面板拖拽一个 **Image** 到 Canvas Panel
2. **重要！** 在右侧 **Details** 面板中，将这个 Image 重命名为：
   ```
   EmojiImage
   ```
   ⚠️ **必须是这个名字！** C++ 代码会通过这个名字查找 widget。

### Step 4: 配置 Image 属性

选中 `EmojiImage`，在 Details 面板中设置：

#### Slot (Canvas Panel Slot)
```
Position X: 0
Position Y: 0
Size X: 128  (或你想要的大小)
Size Y: 128
Anchors: 中心点 (0.5, 0.5)
Alignment: (0.5, 0.5)
```

#### Appearance
```
Brush:
  - Image: (留空，会由 C++ 代码设置)
  - Image Size: (128, 128) 或你想要的大小
  - Tint: 白色 (1, 1, 1, 1)
  - Draw As: Image
```

#### Render Transform
```
Scale: (1, 1)  (如果想要更大可以调整)
```

### Step 5: 可选 - 添加背景或边框

如果你想要背景：

1. 添加一个 **Border** 或 **Image** 作为背景
2. 将 `EmojiImage` 放在背景上方（调整层级）
3. 设置背景的颜色、透明度等

### Step 6: 可选 - 添加动画

你可以添加动画让 emoji 更生动：

1. 点击底部的 **Animations** 标签
2. 点击 **+ Animation** 创建新动画
3. 命名为 `FadeIn` 或 `Bounce`
4. 添加 Track：
   - 选择 `EmojiImage`
   - 添加 **Render Opacity** track (淡入淡出)
   - 或添加 **Render Transform** → **Scale** track (弹跳效果)

示例动画：
```
FadeIn (0.3 秒):
  0.0s: Opacity = 0.0
  0.3s: Opacity = 1.0

Bounce (0.5 秒):
  0.0s: Scale = (0.5, 0.5)
  0.2s: Scale = (1.2, 1.2)
  0.5s: Scale = (1.0, 1.0)
```

### Step 7: 编译并保存

1. 点击右上角 **Compile**
2. 点击 **Save**

---

## 完整的 Widget 层级结构 / Complete Widget Hierarchy

```
Canvas Panel (Root)
└── EmojiImage (Image)  ← 必须是这个名字！
    - Size: 128x128
    - Anchors: Center
    - Alignment: Center
```

**可选的更复杂结构：**
```
Canvas Panel (Root)
├── Background (Border/Image) - 可选背景
└── EmojiImage (Image)  ← 必须是这个名字！
    - Size: 128x128
    - Anchors: Center
```

---

## 视觉示例 / Visual Example

### 简单版本（推荐）

```
┌─────────────────────┐
│                     │
│                     │
│        😊          │  ← EmojiImage (128x128)
│                     │
│                     │
└─────────────────────┘
Canvas Panel (200x200)
```

### 带背景版本

```
┌─────────────────────┐
│  ┌───────────────┐  │
│  │               │  │
│  │      😊      │  │  ← EmojiImage
│  │               │  │
│  └───────────────┘  │  ← Background (Border)
└─────────────────────┘
Canvas Panel
```

---

## 常见问题 / Troubleshooting

### Q: Emoji 不显示？
**A:** 检查：
1. ✅ Image widget 的名字是否是 `EmojiImage`（区分大小写）
2. ✅ `DA_EmojiConfig` 是否配置了纹理
3. ✅ `EmotionDisplayComponent` 的 `Emoji Config Asset` 是否设置
4. ✅ 查看日志是否有 `[EmotionDisplay]` 相关错误

### Q: Emoji 太小/太大？
**A:** 调整：
1. `EmojiImage` 的 Size (在 Slot 中)
2. 或者在 `EmotionDisplayComponent` 中调整 `Draw Size`
3. 或者使用 Render Transform 的 Scale

### Q: Emoji 位置不对？
**A:** 确保：
1. Anchors 设置为中心点 (0.5, 0.5)
2. Alignment 设置为 (0.5, 0.5)
3. Position 设置为 (0, 0)

### Q: 想要圆形背景？
**A:** 添加一个 Border：
1. 添加 Border widget
2. 设置 Brush → Tint 为你想要的颜色
3. 设置 Brush → Draw As → Rounded Box
4. 调整 Brush → Outline Settings → Corner Radius

---

## 高级配置 / Advanced Configuration

### 添加阴影效果

在 `EmojiImage` 的 Details 中：
```
Render Transform:
  - Translation: (2, 2)  // 轻微偏移

然后复制一个 Image 作为阴影：
  - Tint: 黑色 (0, 0, 0, 0.5)
  - 放在 EmojiImage 下方
```

### 添加发光效果

使用 Material：
1. 创建一个 Material (M_EmojiGlow)
2. 使用 Emissive 输出
3. 在 `EmojiImage` 的 Brush → Image 中使用这个 Material

### 添加粒子效果

在 Canvas Panel 中添加：
1. **Retainer Box** (用于性能优化)
2. 在里面放置粒子系统 Widget

---

## 性能优化 / Performance Optimization

### 使用 Retainer Box

如果有很多 NPC：
```
Canvas Panel
└── Retainer Box
    └── EmojiImage
```

Retainer Box 会缓存渲染结果，减少重绘。

### 设置合适的纹理大小

Emoji 纹理建议：
- **推荐大小**: 128x128 或 256x256
- **格式**: PNG (支持透明)
- **压缩**: UserInterface2D (在 Texture 设置中)

---

## 完整配置清单 / Complete Checklist

- [ ] 创建 `WBP_EmojiDisplay` Widget Blueprint
- [ ] 添加 Image widget，命名为 `EmojiImage`
- [ ] 设置 Size 为 128x128
- [ ] 设置 Anchors 为中心
- [ ] 设置 Alignment 为 (0.5, 0.5)
- [ ] (可选) 添加背景 Border
- [ ] (可选) 创建淡入/弹跳动画
- [ ] 编译并保存
- [ ] 在 `BP_UtilityAIController` 中设置 `Emoji Widget Class` = `WBP_EmojiDisplay`
- [ ] 测试：触发情绪变化，检查 emoji 是否显示

---

## 下一步 / Next Steps

配置完 `WBP_EmojiDisplay` 后：
1. 创建 `WBP_SpeechBubble` (类似流程，但用 TextBlock 命名为 `MessageText`)
2. 创建 `DA_EmojiConfig` DataAsset
3. 在 `BP_UtilityAIController` 中配置所有引用
4. 测试！

