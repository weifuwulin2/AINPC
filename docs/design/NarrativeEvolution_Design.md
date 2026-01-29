# 剧情演进系统设计文档 (Narrative Evolution System)

## 1. 概述 (Overview)
本功能旨在引入 **剧情时间轴 (Scene Timeline)**，使 **Narrative Scenes (小队剧情)** 能够随时间推移而演进。
目前，剧情场景只有一个静态的 `PlotOutline`（剧情大纲）。本功能将允许 `PlotOutline` 和 `SquadDirective`（小队指令）在场景持续期间动态改变（例如：从“计划中” -> “行动中” -> “撤退”）。这将使 NPC 的行为和 LLM 的对话内容更加符合当前剧情的发展阶段。

## 2. 架构 (Architecture)

### 2.1 核心组件 (Core Components)
*   **NarrativeSquadSubsystem (导演)**
    *   **角色**: 管理活动中小队的生命周期。
    *   **职责**:
        *   追踪每个活动小队的 `SceneTime`（场景运行时间）。
        *   监听全局事件（针对混合触发器）。
        *   监控 `DT_NarrativeScenes` 中定义的 `Timeline`。
        *   当达到时间节点（或满足条件）时：
            *   更新小队的 `CurrentPlot`（LLM 上下文）。
            *   更新小队的 `CurrentDirective`（行为倾向）。
            *   广播更新给所有成员 NPC。

### 2.2 数据流 (Data Flow)
```mermaid
graph TD
    Tick[Subsystem Tick] -->|Updates Time| Squad[Active Squad Instance]
    Events[Global Events] -->|Notify| Squad
    Squad -->|Checks| Def[DT_NarrativeScenes (Timeline)]
    Def -->|Triggers Node| Squad
    
    subgraph Update Logic
        Squad -->|1. Update Context| PlotVar[CurrentPlotOutline]
        Squad -->|2. Update Behavior| DirectiveVar[CurrentDirective]
        
        PlotVar -->|Injected| Cognition[CognitionComponent (LLM)]
        DirectiveVar -->|Injected| Goal[GoalComponent (Action)]
    end
```

## 3. 数据结构 (Data Structures)

### 3.1 [MODIFY] `FNarrativeTimelineEntry` (Struct)
定义剧情演进中的单个节点。

| 字段 (Field) | 类型 (Type) | 描述 (Description) |
| :--- | :--- | :--- |
| **TimeOffset** | `float` | 自场景开始后的时间（秒）。如果设置了 `TriggerCondition`，则此时间作为**最小等待时间 (Min Time)**，即必须满足时间且满足条件才触发。 |
| **TriggerCondition** | `FGameplayTag` | **[新增]** 可选。如果设置，节点仅在收到此事件 Tag 广播（且满足 TimeOffset）时触发。例如：`Event.PlayerDetected`。 |
| **NewPlotOutline** | `FString` | 更新叙事上下文（例如：“守卫现在充满敌意”）。 |
| **NewDirective** | `FGameplayTag` | 可选。覆盖小队当前的指令（例如：`Directive.Combat`）。 |
| **BarkID** | `FName` | 可选。触发小队队长的一句特定台词（Bark）。 |

### 3.2 [MODIFY] `FNarrativeSceneDef` (Struct)
扩展现有的场景定义以包含时间轴。

| 字段 (Field) | 类型 (Type) | 描述 (Description) |
| :--- | :--- | :--- |
| `PlotOutline` | `FString` | **初始** 上下文（T=0 时）。 |
| `Timeline` | `TArray<FNarrativeTimelineEntry>` | **[新增]** 剧情演进节点的有序列表。 |
| `...` | `...` | （现有的 Cast, Props, CompletionTags 保持不变）。 |

## 4. 技术方案与风险应对 (Technical Solutions & Risk Managment)

### 4.1 节奏匹配问题：混合触发器 (Hybrid Triggers)
*   **风险**: 纯时间触发（如固定 10秒后攻击）可能显得生硬，如果玩家移动过快或过慢，会导致剧情脱节（"空大"）。
*   **解决方案**: 使用 **混合触发逻辑 (Gate Logic)**。
    *   在 Timeline 中配置：`TimeOffset = 10s`, `TriggerCondition = Event.PlayerDetected`。
    *   **逻辑**: 等待 **至少** 10秒，**然后** 等待 `PlayerDetected` 事件。
    *   如果不需要时间限制，可设 `TimeOffset = 0`，则事件发生即触发（如：受到伤害立即反击）。
    *   这复用了现有的 `NarrativeSquadSubsystem::OnNarrativeEventRecorded` 监听机制。

### 4.2 状态同步问题：基于优先级的仲裁 (Priority Arbitration)
*   **风险**: 剧情时间轴要求“立刻逃跑”，但 NPC 当前正处于“眩晕 (Stunned)”状态或正在播放不可打断的交互动画。
*   **解决方案**: **利用 Utility AI 的架构优势，而非强制执行**。
    1.  **指令下达**: 系统将 `Directive.Flee` 标签赋予 NPC。
    2.  **仲裁 (GoalComponent)**: 确认 `Flee` 优先级高于当前的 `Idle/Work`。
    3.  **可行性检查 (UtilityAI)**: 
        *   **如果处于 Stunned**: `UtilityAction::IsAbilityAvailable` 返回 `false`。NPC 继续保持 Stunned，直到状态结束。
        *   **状态回复后**: 下一帧 Utility AI 再次评估，发现 `Flee` 分数极高且可行，立即执行逃跑。
    *   **结论**: 这种“愿望 (Desire) vs 能力 (Capability)”的分层设计完美兼容现有架构，无需硬编码打断逻辑。

### 4.3 中途加入 (Join-in-Progress)
*   **问题**: 玩家加载存档或进入一个已经“模拟”了一段时间的场景。
*   **解决方案**: 目前 MVP 阶段，`ActivateScene` 总是从 T=0 开始。如果未来需要持久化模拟，需要保存 `AccumulatedTime` 并在加载时快进时间轴状态。

### 4.4 玩家强行打断 (Player Interruption)
*   **问题**: 剧本安排 T+10s "吵架"，但 T+5s 玩家攻击了 NPC。
*   **解决方案**: **Utility AI 的威胁系统优先级高于叙事指令**。
    1.  **叙事层**: 仍然会按计划在 T+10s 下发 "Directive.Argue"（吵架指令）。
    2.  **感知层**: NPC 感知到 `HostileTarget` (玩家)。
    3.  **决策层**: Utility AI 发现 `Combat Action` (针对威胁) 的分数远高于 `Interaction.Social` (针对指令)。
    4.  **结果**: NPC 忽略吵架指令，优先攻击玩家。
    5.  **LLM 上下文**: 此时 CognitionComponent 会同时拥有 "CurrentPlot: Argue" 和 "Stimuli: Attacked by Player"。LLM 会自然生成类似 *"没空吵架了，先干掉这个疯子！"* 的反应。
    6.  **后续**: 如果由于玩家攻击导致 NPC 死亡或场景被破坏，`CompletionTags` (如 `Status.Dead`) 会触发并结束场景。

## 5. 验证计划 (Verification Plan)

### 5.1 验收标准 (Acceptance Criteria)
1.  **上下文更新**: 达到时间节点时，`CognitionComponent` 正确接收到新的 `PlotOutline`。
2.  **指令更新**: 达到节点时，`GoalComponent` 接收到新的 `Directive` 标签。
3.  **时序准确**: 更新应在 `TimeOffset` 的 1秒误差内发生。
4.  **条件触发**: 设置了 `TriggerCondition` 的节点，必须在**时间满足**且**事件发生**后才触发。

### 5.2 测试用例 (Test Cases)
*   **TC01_Evolution (基础演进)**:
    *   设置: 场景包含节点 A (T+5s: "Anger") 和节点 B (T+10s: "Fight", Directive.Combat)。
    *   观察: T+0 初始状态 -> T+5 更新描述 -> T+10 进入战斗状态。
*   **TC02_HybridTrigger (混合触发)**:
    *   设置: 节点 (T+5s, Condition: "Event.SeePlayer")。
    *   操作: 启动场景，等待 10秒（不看玩家），然后让 NPC 看到玩家。
    *   观察: 前 10秒不触发；看到玩家瞬间触发。
