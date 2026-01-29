# Timeline 配置指南 (Timeline Configuration Guide)

本文档说明如何在 `DT_NarrativeScenes.json` 中配置 **剧情演进时间轴 (Narrative Timeline)**。

---

## 📋 字段说明

### `FNarrativeTimelineEntry` 结构

每个时间轴节点包含以下字段：

| 字段名 | 类型 | 必填 | 描述 |
|-------|------|------|------|
| **TimeOffset** | `float` | ✅ | 自场景激活后的时间（秒）。如果有 `TriggerCondition`，则作为**最小等待时间**。 |
| **TriggerCondition** | `FGameplayTag` | ❌ | 可选事件条件。如果设置，节点仅在**时间到达且事件触发**时执行。 |
| **PlotUpdate** | `FString` | ✅ | 新的剧情描述，会注入到 NPC 的 LLM 上下文中。 |
| **DirectiveOverride** | `FGameplayTag` | ❌ | 可选指令覆盖。例如 `Directive.Combat` 强制所有成员进入战斗行为。 |
| **BarkID** | `FName` | ❌ | （未实现）队长台词 ID。 |

---

## 🎯 触发模式

### 1️⃣ 纯时间触发 (Time-Only Trigger)

**配置：** 设置 `TimeOffset`，不设置 `TriggerCondition`。

**行为：** 场景激活后，经过指定时间自动触发。

**示例：** 10 秒后 NPC 自动发起攻击

```json
{
  "TimeOffset": 10.0,
  "TriggerCondition": "",
  "PlotUpdate": "计划时间到！立刻行动！",
  "DirectiveOverride": "Directive.Combat"
}
```

---

### 2️⃣ 混合触发 (Hybrid Trigger)

**配置：** 同时设置 `TimeOffset` 和 `TriggerCondition`。

**行为：** 
1. 必须**至少等待** `TimeOffset` 时间
2. **且**收到指定的事件标签
3. 两个条件都满足时触发

**示例：** 至少等待 5 秒，且玩家被守卫发现时触发

```json
{
  "TimeOffset": 5.0,
  "TriggerCondition": "Event.PlayerDetected",
  "PlotUpdate": "有入侵者！抓住他！",
  "DirectiveOverride": "Directive.Combat"
}
```

> **⚠️ 注意：** 如果玩家在 5 秒内就被发现，节点不会立即触发，而是等到第 5 秒时才触发。

---

### 3️⃣ 纯事件触发 (Event-Only Trigger)

**配置：** 设置 `TimeOffset: 0.0` 和 `TriggerCondition`。

**行为：** 场景激活后，只要事件发生即触发（无时间限制）。

**示例：** NPC 受到伤害立即反击

```json
{
  "TimeOffset": 0.0,
  "TriggerCondition": "Event.NPCDamaged",
  "PlotUpdate": "他们攻击我们了！全力反击！",
  "DirectiveOverride": "Directive.Combat"
}
```

---

## 📝 完整配置示例

### 场景：矿场劫持 (Mine Heist)

**剧情演进：**
1. **T=0**: 劫匪潜入，警戒观察
2. **T=10s + 玩家靠近**: 劫匪发现玩家，语言威胁
3. **T=20s + 玩家不退后**: 劫匪开火
4. **任何时间 + 劫匪被攻击**: 立即进入战斗

```json
{
  "Scene_MineHeist": {
    "PlotOutline": "三名匪徒正在策划劫持矿场。他们需要等待合适时机。",
    "Cast": [
      {
        "NPCTemplateID": "Orc_Bandit",
        "RelativeTransform": {"Translation": [100, 0, 0]},
        "RoleOverride": "你是劫匪头目，负责指挥行动。"
      },
      {
        "NPCTemplateID": "Orc_Bandit",
        "RelativeTransform": {"Translation": [150, 100, 0]},
        "RoleOverride": "你是劫匪打手，听从头目指令。"
      }
    ],
    "CompletionTags": ["Event.AllBanditsDead", "Event.PlayerEscaped"],
    "Timeline": [
      {
        "TimeOffset": 0.0,
        "TriggerCondition": "",
        "PlotUpdate": "劫匪们潜入矿场，正在观察地形，等待时机。",
        "DirectiveOverride": ""
      },
      {
        "TimeOffset": 10.0,
        "TriggerCondition": "Event.PlayerDetected",
        "PlotUpdate": "劫匪发现了一个目击者！头目决定先警告对方离开。",
        "DirectiveOverride": "Directive.Social"
      },
      {
        "TimeOffset": 20.0,
        "TriggerCondition": "Event.PlayerStillNear",
        "PlotUpdate": "目击者不肯离开！必须动手了，杀人灭口！",
        "DirectiveOverride": "Directive.Combat"
      },
      {
        "TimeOffset": 0.0,
        "TriggerCondition": "Event.BanditAttacked",
        "PlotUpdate": "有人袭击了我们！立刻反击！",
        "DirectiveOverride": "Directive.Combat"
      }
    ]
  }
}
```

---

## 🎮 使用指南

### 1. 编辑 JSON 文件

打开 `Content/Data/DT_NarrativeScenes.json`，在场景定义中添加 `Timeline` 字段。

### 2. 使用 Gameplay Tags

确保 `TriggerCondition` 中的标签已在 `DefaultGameplayTags.ini` 中注册：

```ini
[/Script/GameplayTags.GameplayTagsList]
GameplayTagList=(Tag="Event.PlayerDetected",DevComment="玩家被 NPC 感知到")
GameplayTagList=(Tag="Event.BanditAttacked",DevComment="劫匪受到攻击")
```

### 3. 广播事件

在你的游戏代码中，通过 `NarrativeDirectorSubsystem` 广播事件：

```cpp
// 示例：玩家被守卫发现时
if (UNarrativeDirectorSubsystem* Director = GetWorld()->GetSubsystem<UNarrativeDirectorSubsystem>())
{
    FNarrativeEvent Event;
    Event.Tag = FGameplayTag::RequestGameplayTag("Event.PlayerDetected");
    Event.Tags.Add("Event.PlayerDetected");
    Director->RecordEvent(Event);
}
```

### 4. 测试场景

在编辑器中放置 `NarrativeSceneAnchor`，设置场景 ID 为 `Scene_MineHeist`，运行游戏观察时间轴演进。

---

## ⚙️ 高级技巧

### 链式演进

节点会**按顺序**执行，你可以设计渐进式剧情：

```json
"Timeline": [
  {"TimeOffset": 0, "PlotUpdate": "第一阶段：潜伏"},
  {"TimeOffset": 10, "PlotUpdate": "第二阶段：侦察"},
  {"TimeOffset": 20, "PlotUpdate": "第三阶段：行动"},
  {"TimeOffset": 30, "PlotUpdate": "第四阶段：撤退"}
]
```

### 条件分支

通过不同事件触发不同分支：

```json
"Timeline": [
  {
    "TimeOffset": 10,
    "TriggerCondition": "Event.PlayerFriendly",
    "PlotUpdate": "玩家表现友好，NPC 降低戒备。",
    "DirectiveOverride": "Directive.Idle"
  },
  {
    "TimeOffset": 10,
    "TriggerCondition": "Event.PlayerHostile",
    "PlotUpdate": "玩家表现敌对，NPC 进入战斗状态！",
    "DirectiveOverride": "Directive.Combat"
  }
]
```

### 动态难度

根据玩家进度调整敌人行为：

```json
{
  "TimeOffset": 60,
  "TriggerCondition": "Event.BossPhase2",
  "PlotUpdate": "Boss 进入狂暴模式！",
  "DirectiveOverride": "Directive.Combat"
}
```

---

## 🐛 常见问题

**Q: 为什么节点没有触发？**  
A: 检查以下几点：
- 场景是否通过 `ActivateScene` 激活？
- 时间是否到达 `TimeOffset`？
- 如果有 `TriggerCondition`，事件是否正确广播？

**Q: 可以跳过某个节点吗？**  
A: 不能直接跳过。Timeline 是顺序执行的，如果某节点有 `TriggerCondition` 且事件未触发，后续节点会被阻塞。

**Q: 如何重置 Timeline？**  
A: 调用 `EndScene` 结束场景，再重新 `ActivateScene`。

---

## 📚 参考资料

- **设计文档**: `docs/design/NarrativeEvolution_Design.md`
- **实现计划**: `artifacts/implementation_plan.md`
- **代码文件**: 
  - `NarrativeSquadSubsystem.h` (数据结构)
  - `NarrativeSquadSubsystem.cpp` (Timeline 逻辑)
