# World Major Events 系统设计

## 1. 概述

### 1.1 设计背景

WorldDirector 现有两套数据驱动系统:

| 系统 | DataTable 类型 | 职责 |
|:---|:---|:---|
| **WorldTimeline** | `FWorldTimelineEvent` | 触发叙事场景（预编排 Scene 或 LLM 生成） |
| **MajorEvents** ✨新 | `FWorldMajorEventRow` | 定义大规模世界事件（兽潮、魔王入侵等） |

两者的核心区别:

- **Timeline** = 叙事层。产出 NarrativeSceneSquad，NPC 在场景锚点演出对话/剧情。
- **MajorEvents** = 系统层。只广播 Delegate，不直接产出场景。外部系统（战斗AI、Spawner、UI 等）订阅 Delegate 来实现具体效果。

### 1.2 为什么要分离?

```
WorldDirector 只负责: "Day 3 Hour 8 开始兽潮，持续 2 天"
外部系统各自负责:
  - CombatSpawner: 增加野兽刷新频率
  - UI:            显示全屏警告
  - NPC UtilityAI: 提高 Flee/Combat 优先级
  - LLM Context:   自动注入世界状态，影响场景生成
```

WorldDirector 不需要知道"兽潮具体怎么实现"，它只是一个**事件总线 + 计时器**。

---

## 2. 数据结构

### 2.1 核心类型 (`WorldDirectorTypes.h`)

```
┌──────────────────────────────────────────┐
│  EWorldMajorEventType (enum)             │
│  BeastTide | DemonInvasion | FactionWar  │
│  NaturalDisaster | Festival | Plague     │
│  Custom                                  │
└──────────────────────────────────────────┘
          ↓ 用于
┌──────────────────────────────────────────────────┐
│  FWorldMajorEventRow : FTableRowBase (DataTable)  │
│                                                    │
│  TriggerDay        int32       触发日              │
│  TriggerHour       float       触发时 (0-24)      │
│  EventType         enum        事件类型            │
│  EventName         FString     事件名              │
│  EventDescription  FString     描述 (→LLM 上下文)  │
│  DurationDays      float       持续天数 (0=瞬时)   │
│  TensionBoost      float       张力加成 (0-0.5)    │
│  ForcedStartAction enum        强制 DramaticAction │
│  bHasStarted       bool        运行时标记 (Transient) │
└──────────────────────────────────────────────────┘
          ↓ 触发后生成
┌──────────────────────────────────────────────────┐
│  FActiveWorldEvent (runtime tracking)              │
│                                                    │
│  RowName           FName       DataTable 行名      │
│  EventType         enum        事件类型            │
│  EventName/Desc    FString     名称/描述           │
│  StartDay/Hour     int32/float 开始时间            │
│  EndDay/EndHour    int32/float 预计算结束时间      │
│  TensionBoost      float       张力加成            │
│  EventTag          FGameplayTag 对应的 GameplayTag │
└──────────────────────────────────────────────────┘
```

### 2.2 GameplayTags

| C++ 名 | Tag 字符串 |
|:---|:---|
| `AINPCTags::WorldEvent_BeastTide` | `WorldEvent.BeastTide` |
| `AINPCTags::WorldEvent_DemonInvasion` | `WorldEvent.DemonInvasion` |
| `AINPCTags::WorldEvent_FactionWar` | `WorldEvent.FactionWar` |
| `AINPCTags::WorldEvent_NaturalDisaster` | `WorldEvent.NaturalDisaster` |
| `AINPCTags::WorldEvent_Festival` | `WorldEvent.Festival` |
| `AINPCTags::WorldEvent_Plague` | `WorldEvent.Plague` |
| `AINPCTags::WorldEvent_Custom` | `WorldEvent.Custom` |

### 2.3 Delegates

```cpp
// 事件开始时广播
FOnWorldMajorEventStarted(EWorldMajorEventType EventType, const FActiveWorldEvent& EventData)

// 事件结束时广播
FOnWorldMajorEventEnded(EWorldMajorEventType EventType, const FActiveWorldEvent& EventData)
```

两个都是 `DYNAMIC_MULTICAST`，Blueprint 和 C++ 均可订阅。

---

## 3. 数据流 (Data Flow)

### 3.1 完整流程图

```
         ┌─────────────────────────────────────────────────┐
         │              DataTable: DT_MajorWorldEvents      │
         │  (FWorldMajorEventRow rows, 设计师在 Editor 填写) │
         └─────────────────────┬───────────────────────────┘
                               │ (MajorEventsTable 引用)
                               ▼
┌──────────────────────────────────────────────────────────────────┐
│                    WorldDirectorSubsystem                        │
│                                                                  │
│  PeriodicEvaluation() [每 120s 调用一次]                          │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │ ① CheckMajorEvents()  ← 最先执行，不受 Cooldown 限制        │ │
│  │   遍历 MajorEventsTable                                     │ │
│  │   if (Day >= TriggerDay && Hour >= TriggerHour && !Started) │ │
│  │     → bHasStarted = true                                    │ │
│  │     → 构建 FActiveWorldEvent (预算 EndDay/EndHour)           │ │
│  │     → ActiveMajorEvents.Add(Active)                         │ │
│  │     → RecordEvent("[WorldDirector] Major Event Started: ...")│ │
│  │     → Broadcast OnWorldMajorEventStarted                    │ │
│  │     → Broadcast OnDramaticBeat(ForcedStartAction)           │ │
│  ├─────────────────────────────────────────────────────────────┤ │
│  │ ② TickActiveEvents()  ← 紧接执行                            │ │
│  │   反向遍历 ActiveMajorEvents                                │ │
│  │   if (Day > EndDay || (Day == EndDay && Hour >= EndHour))   │ │
│  │     → RecordEvent("[WorldDirector] Major Event Ended: ...")  │ │
│  │     → Broadcast OnWorldMajorEventEnded                      │ │
│  │     → RemoveAt(i)                                           │ │
│  ├─────────────────────────────────────────────────────────────┤ │
│  │ ③ Cooldown/Capacity 检查  ← 只影响叙事场景生成               │ │
│  │ ④ CheckWorldTimeline (FWorldTimelineEvent)                  │ │
│  │ ⑤ EvaluateTension()                                         │ │
│  │   → OverallTension = weighted sum + Σ TensionBoost          │ │
│  │ ⑥ SelectDramaticAction() → RequestLLMSceneGeneration()      │ │
│  │   → UserPrompt 追加 [ACTIVE WORLD EVENTS] section           │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────┬──────────────────────┬───────────────────┬────────────┘
          │                      │                   │
          ▼                      ▼                   ▼
  OnWorldMajorEventStarted  OnWorldMajorEventEnded  LLM Prompt
  (Delegate 广播)            (Delegate 广播)         (自动注入事件描述)
          │                      │                   │
          ▼                      ▼                   ▼
  ┌───────────────┐  ┌───────────────┐  ┌────────────────────────┐
  │ 订阅者 (外部)  │  │ 订阅者 (外部)  │  │ LLM 场景生成            │
  │ - Spawner     │  │ - Spawner     │  │ 生成的剧情会考虑         │
  │ - UI Alert    │  │ - UI Clear    │  │ "兽潮正在进行" 等上下文   │
  │ - NPC AI      │  │ - NPC AI      │  │ → 更符合当前世界状态      │
  │ - Blueprint   │  │ - Blueprint   │  │                          │
  └───────────────┘  └───────────────┘  └────────────────────────┘
```

### 3.2 Tension 影响链

```
基础 OverallTension (0.0 - 1.0)
  = 0.25 × DeathPressure
  + 0.20 × FactionVolatility
  + 0.15 × PlayerActivity
  + 0.10 × PopulationStress
  + 0.15 × TimelinePressure
  + 0.15 × Monotony

  + Σ ActiveMajorEvents[i].TensionBoost   ← 兽潮 +0.2, 节日 +0.05 等

  = Clamp(result, 0.0, 1.0)
```

效果: 活跃的大规模事件推高 OverallTension → SelectDramaticAction 偏向 ESCALATE → LLM 生成更激烈的场景。

### 3.3 LLM Prompt 注入格式

当有活跃事件时，`RequestLLMSceneGeneration()` 的 UserPrompt 末尾自动追加:

```
[ACTIVE WORLD EVENTS]:
- [Day 1-Day 3] Beast Tide: Waves of beasts emerge from the Dark Forest...
- [Day 2-Day 2] Festival: Harvest celebration in the town square...
```

### 3.4 NarrativeHistory 记录

事件开始和结束都通过 `RecordEvent()` 写入 NarrativeHistory:

```
[WorldDirector] Major Event Started: Beast Tide - Waves of beasts emerge...  [Tags: WorldDirector.Event, WorldEvent.BeastTide]
[WorldDirector] Major Event Ended: Beast Tide                                 [Tags: WorldDirector.Event, WorldEvent.BeastTide]
```

其他系统通过 `GetWorldStateDescription()` 可间接看到这些历史记录。

---

## 4. 文件清单

| 文件 | 修改类型 | 新增内容 |
|:---|:---|:---|
| `Public/Subsystems/WorldDirectorTypes.h` | 新增类型 | `EWorldMajorEventType`, `FWorldMajorEventRow`, `FActiveWorldEvent` |
| `Public/Subsystems/WorldDirectorSubsystem.h` | 新增声明 | Delegates, `MajorEventsTable`, `ActiveMajorEvents`, 5 个新方法 |
| `Private/Subsystems/WorldDirectorSubsystem.cpp` | 新增实现 + 修改现有 | 4 个新函数 + 修改 3 个现有函数 |
| `Public/Social/SocialGameplayTags.h` | 新增声明 | 7 个 `WorldEvent_*` Tags |
| `Private/Social/SocialGameplayTags.cpp` | 新增定义 | 7 个 `WorldEvent.*` Tag 定义 |

---

## 5. 测试指南

### 5.1 前置条件

- 编译通过
- 有可用的 `TimeManager` (GameInstanceSubsystem)，能获取 `GetCurrentDay()` / `GetCurrentHour()`

### 5.2 测试步骤

#### Step 1: 创建 DataTable

1. Content Browser → 右键 → Miscellaneous → **DataTable**
2. Row Structure 选择 **FWorldMajorEventRow**
3. 命名为 `DT_MajorWorldEvents`
4. 添加测试行:

| RowName | TriggerDay | TriggerHour | EventType | EventName | EventDescription | DurationDays | TensionBoost | ForcedStartAction |
|:---|:---|:---|:---|:---|:---|:---|:---|:---|
| `BeastTide_Day1` | 1 | 8.0 | BeastTide | Beast Tide | Waves of beasts emerge from the Dark Forest, threatening nearby settlements. | 2.0 | 0.2 | ESCALATE |
| `Festival_Day2` | 2 | 12.0 | Festival | Harvest Festival | The town celebrates the autumn harvest with feasting and games. | 0.5 | 0.05 | RELIEVE |
| `Instant_Test` | 1 | 0.0 | Custom | Test Instant | Instantaneous test event. | 0.0 | 0.0 | DISRUPT |

#### Step 2: 关联 DataTable

在 WorldDirectorSubsystem 的 **Details 面板** 或 **DefaultEngine.ini** 中:
- 将 `MajorEventsTable` 设置为刚创建的 `DT_MajorWorldEvents`

> **注意**: WorldDirectorSubsystem 是 WorldSubsystem，没有 Details 面板。需要在 C++ 或 Blueprint 中设置。推荐在 GameMode 的 BeginPlay 中:

```cpp
// 在 GameMode BeginPlay 中
UWorldDirectorSubsystem* WD = GetWorld()->GetSubsystem<UWorldDirectorSubsystem>();
if (WD)
{
    WD->MajorEventsTable = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/DataTables/DT_MajorWorldEvents.DT_MajorWorldEvents"));
}
```

或 Blueprint 中:
```
Get World Director Subsystem → Set Major Events Table → DT_MajorWorldEvents
```

#### Step 3: 绑定 Delegate（Blueprint 测试）

1. 在任意 Blueprint（如 GameMode BP）中:
   - 获取 `WorldDirectorSubsystem`
   - Bind Event → `On World Major Event Started`
   - 在回调中 → **Print String**: `"EVENT STARTED: {EventName} Type={EventType}"`
   - 同理绑定 `On World Major Event Ended`

2. 在回调中可以检查 `EventData` 结构体的所有字段

#### Step 4: 触发测试

**方法 A: 等待自然触发**
- 确保 `TimeManager` 正常推进 Day/Hour
- 等到 Day 1 Hour 8 → BeastTide 自动触发

**方法 B: ForceEvaluation (推荐)**
- Console 命令或 Blueprint 调用:
```
WorldDirectorSubsystem → ForceEvaluation()
```
- 这会立即执行 `CheckMajorEvents()` + `TickActiveEvents()`
- 如果当前时间已过 TriggerDay/Hour，事件会立即触发

**方法 C: 修改 TimeManager 快进**
- 如果 TimeManager 有 `SetCurrentDay()` / `SetCurrentHour()` API:
```
TimeManager → SetCurrentDay(1)
TimeManager → SetCurrentHour(8.0)
WorldDirectorSubsystem → ForceEvaluation()
```

#### Step 5: 验证清单

| 验证项 | 预期结果 | 检查方式 |
|:---|:---|:---|
| 事件触发 | Day/Hour 到达时 bHasStarted=true | Output Log: `"Major Event STARTED: 'Beast Tide'"` |
| Delegate 广播 | Blueprint 回调被调用 | Print String 输出 |
| ActiveMajorEvents 填充 | 数组长度 > 0 | `GetActiveWorldEvents().Num()` 或 Debug Watch |
| TensionBoost 生效 | OverallTension 增加 | Output Log: `"Tension: ... => Overall=X.XX"` |
| LLM Prompt 包含事件 | UserPrompt 有 `[ACTIVE WORLD EVENTS]` | Output Log: `"=== LLM Scene Request ==="` |
| 事件过期移除 | EndDay/EndHour 到达后从数组移除 | Output Log: `"Major Event ENDED: 'Beast Tide'"` |
| 历史记录写入 | NarrativeHistory 中有记录 | `GetWorldStateDescription()` 包含 Major Event |
| 瞬时事件 (Duration=0) | 同周期内触发并立即结束 | `Instant_Test` 在下一次 Tick 被移除 |
| 重复触发保护 | 同一行不会触发两次 | `bHasStarted` 标记为 true 后跳过 |

#### Step 6: 日志关键字搜索

在 Output Log 中搜索:
```
"Major Event STARTED"    → 事件触发
"Major Event ENDED"      → 事件过期
"Tension:"               → 查看 TensionBoost 是否生效
"[ACTIVE WORLD EVENTS]"  → LLM prompt 注入
```

### 5.3 C++ 订阅示例

```cpp
// 在任意 Actor 的 BeginPlay 中
void AMySpawner::BeginPlay()
{
    Super::BeginPlay();

    if (UWorldDirectorSubsystem* WD = GetWorld()->GetSubsystem<UWorldDirectorSubsystem>())
    {
        WD->OnWorldMajorEventStarted.AddDynamic(this, &AMySpawner::OnMajorEventStarted);
        WD->OnWorldMajorEventEnded.AddDynamic(this, &AMySpawner::OnMajorEventEnded);
    }
}

void AMySpawner::OnMajorEventStarted(EWorldMajorEventType EventType, const FActiveWorldEvent& EventData)
{
    if (EventType == EWorldMajorEventType::BeastTide)
    {
        // 增加野兽刷新频率
        SpawnRateMultiplier = 3.0f;
    }
}

void AMySpawner::OnMajorEventEnded(EWorldMajorEventType EventType, const FActiveWorldEvent& EventData)
{
    if (EventType == EWorldMajorEventType::BeastTide)
    {
        SpawnRateMultiplier = 1.0f;
    }
}
```

### 5.4 Blueprint 订阅示例

```
Event BeginPlay
  → Get Subsystem (WorldDirectorSubsystem)
  → Bind Event to On World Major Event Started
      → Switch on EWorldMajorEventType
          → BeastTide: Set Spawn Rate = 3.0
          → Festival: Play Celebration FX
```

---

## 6. 设计决策记录

| 决策 | 理由 |
|:---|:---|
| 独立于 WorldTimelineTable | 职责分离: Timeline = 叙事场景, MajorEvents = 系统事件 |
| 只广播 Delegate | WorldDirector 不应知道"怎么实现兽潮"，保持单一职责 |
| CheckMajorEvents 在 Cooldown 之前 | 大事件必须准时触发，不能被场景生成冷却阻塞 |
| 预计算 EndDay/EndHour | 避免每帧做加法，简化过期判断 |
| TensionBoost 累加 | 多个事件同时活跃时张力叠加，Clamp 保证不超过 1.0 |
| bHasStarted 是 Transient | 运行时标记，重启游戏后 DataTable 自动重置，事件可重新触发 |
| RecordEvent 写入历史 | NarrativeHistory 是唯一事实源，其他系统通过它获取世界状态 |

---

## 7. 扩展路径

1. **事件链**: MajorEvent A 结束后自动触发 MajorEvent B（当前需设计师在 DataTable 手动安排 Day/Hour）
2. **条件触发**: 添加 `TriggerCondition` 字段（如"Faction X hostility > 0.8 时才触发"）
3. **区域限定**: 添加 `AffectedArea` 或 `LocationTag` 限制事件影响范围
4. **玩家介入**: 添加 `bCanBePreventedByPlayer` 标志，配合任务系统
