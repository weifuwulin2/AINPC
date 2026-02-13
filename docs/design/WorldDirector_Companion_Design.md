# WorldDirector & Companion (Paimon) 系统设计

## 1. 设计背景

### 1.1 现有架构的缺口

当前叙事架构:

```
NarrativeDirectorSubsystem (历史学家) ← 只记录，不思考
       ↓ 提供世界状态
NarrativeSquadSubsystem (舞台导演) ← 执行预编排的 Scene
       ↓ 注入角色/剧情
NPC (CognitionComponent + UtilityAI) ← 演出
       ↓ 行为产生事件
NarrativeCompanion (观察者) ← 记录回 Director
```

**缺失环节**: Director 和 Squad 之间没有"大脑"。Director 只是数据库，Squad 只是执行器。没有系统在思考"接下来应该发生什么"。

### 1.2 否决的方案

#### 方案 A: Checkpoint 模板库
设计师预先写好剧情模板（"背叛"、"权力斗争"等），WorldDirector 用张力分数选择下一个模板。

**否决原因**: 本质上和 UtilityAI 一样 — 用数值从预编排内容中选择。模板写多了就是育碧式支线任务编辑器，内容有限，分支指数爆炸。

#### 方案 B: 纯 LLM 自由生成
让 LLM 读取世界状态，完全自己想接下来的故事。

**否决原因**: LLM 没有戏剧直觉。它会生成逻辑正确但无聊的故事 — "商人继续卖货，农民继续种地"。没有冲突就没有游戏性。

---

## 2. 核心设计: WorldDirector 作为戏剧节奏控制器

### 2.1 核心理念

WorldDirector **不知道任何具体故事**。它只知道**戏剧节奏**。

参考 Left 4 Dead 的 AI Director:
- 玩家太轻松 → 加压
- 玩家快死了 → 减压
- 张力到顶 → 释放

WorldDirector 控制的是**曲线**，不是内容。

### 2.2 两个职责

**职责 1: 张力评估 (C++, 确定性)**

周期性扫描世界状态，量化戏剧张力值:

| 张力维度 | 计算来源 | 示例 |
|:---|:---|:---|
| FactionTension | 阵营关系数据 | A 和 B 关系降到 -50 以下 |
| PowerVacuum | VIP 死亡记录 | 某重要 NPC 刚死 |
| PlayerPattern | 玩家行为追踪 | 连续3天只做商人任务（舒适区） |
| UnresolvedThread | 关系/事件记录 | 有 NPC 对玩家有仇但还活着 |
| ResourcePressure | 世界资源状态 | 某区域食物短缺 |

**职责 2: 戏剧动作决策 (查曲线)**

根据张力曲线，决定当前需要什么类型的戏剧动作:

| 戏剧动作 | 含义 | 触发条件 |
|:---|:---|:---|
| **ESCALATE** | 升级现有冲突 | 某条张力线持续上升但未爆发 |
| **DISRUPT** | 引入意外 | 世界过于平静，玩家在舒适区 |
| **CONVERGE** | 让不相关的线撞在一起 | 多条独立张力线同时存在 |
| **REVEAL** | 暴露隐藏信息 | 存在未解决的悬念 |
| **RELIEVE** | 喘息/奖励 | 刚经历了高张力事件 |

**只有这 5 种动作，不是 50 个模板。**

### 2.3 LLM 的角色: 实例化

LLM 收到 `{ 戏剧动作 + 当前世界状态 }`, 输出具体要发生什么。

示例输入:
```
戏剧动作: ESCALATE
世界状态: 铁匠和商人有矛盾，玩家刚帮过商人，最近没什么大事
可用NPC: [铁匠(愤怒), 商人(焦虑), 旅行者(中立)]
可用场所: [广场(Public,Outdoor), 酒馆(Social,Indoor), 小巷(Private,Shady)]
```

示例输出:
```
事件: 铁匠拒绝给商人供货，商人来找玩家帮忙施压
参与者: 铁匠, 商人
场所需求: Public, Outdoor
```

LLM 不需要输出 `FNarrativeSceneDef`，只输出叙事意图。C++ 负责落地（匹配 NPC、匹配场所、组装 Scene）。

---

## 3. 固定终点 + Emergent 过程

### 3.1 世界主线结构

```
固定起点: 世界是和平的
固定终点: 魔王入侵 — 赢 或 输
中间过程: 完全 emergent
```

WorldDirector 知道一条**固定时间轴**上的锚点事件:

```
Day 30:  兽潮
Day 60:  前哨战
Day 100: 魔王入侵
```

它的工作是在锚点之间，根据世界状态制造**过程中的戏剧**。这些戏剧不改变终点，只改变到达终点时**世界的状态** — 谁活着、谁恨谁、资源够不够。

### 3.2 为什么收束到极简更好

传统游戏的"深度"来自编剧写了多少分支。本系统的"深度"来自每个玩家走过的路不一样。

同一个"赢了"的结局:
- **玩家 A**: 铁匠在中期背叛商人，小镇经济崩溃，守城战没有足够武器，惨胜
- **玩家 B**: 帮铁匠和商人和解，但铁匠在兽潮中牺牲，Paimon 在终局说"要是他还在就好了"

**结局相同，体验完全不同。** 价值不在故事本身有多复杂，而在于这个故事是"我的"。

---

## 4. Companion (Paimon) 作为叙事界面

### 4.1 核心定位: 软 Cutscene

NPC 自己在演，玩家可能根本没注意到。传统游戏用镜头强制拉过去，但 emergent 系统无法预制镜头 — 不知道 NPC 会怎么演。

**Paimon 不替代 cutscene，她替代的是旁白。** Cutscene 是"看"，她是"说"。对于 LLM 驱动的 emergent 系统，"说"比"看"更合适。

### 4.2 三阶段叙事职能

#### Before — 预兆
WorldDirector 决定要 ESCALATE，Scene 还没触发。Paimon 基于 MemoryComponent 中的积累 + WorldDirector 的 `NarrativeHint`, 通过 CognitionComponent 生成有倾向性的评论:

> "那个铁匠最近脾气好像越来越差了..."

不是系统告诉她说什么，是 hint + 记忆 → 自然生成。

#### During — 引导
Scene 触发，铁匠和商人在广场对峙。Paimon 通过 SensoryComponent 感知到事件:

- 如果玩家离得远: "快看，那边好像吵起来了！"
- 如果玩家在旁边: "就是上次跟你买东西的那个商人，好像是铁匠断了他的货..."

她补充**上下文** — 将当前事件和玩家的历史经验串联。

#### After — 收束
Scene 结束，Paimon 总结并固化为"共同记忆":

> "没想到最后铁匠自己走了... 你觉得他会善罢甘休吗？"

收束反馈给 WorldDirector，影响下一次张力评估。

### 4.3 深层联结的来源

联结不来自 Paimon 有多聪明，而来自她和玩家**经历了同样的事情**并且有自己的看法。

- 她是唯一记得"我们一起经历了什么"的角色
- 终局时她说的话，每个玩家听到的完全不同
- 玩家觉得"她也在关注这件事" — 这就是 bond

---

## 5. 系统闭环

```
WorldDirector (张力曲线 + 戏剧动作)
    ↓ 叙事意图 + NPC 查询 + 场所匹配
NarrativeSquadSubsystem (组装 Scene, 执行)
    ↓ NPC 演出 (UtilityAI + Cognition)
Companion 观察 + 向玩家叙述 (Before / During / After)
    ↓ 事件回流
NarrativeDirectorSubsystem (记录历史)
    ↓ 世界状态更新
WorldDirector (重新评估张力)
```

### 5.1 NPC 来源

- **现有 NPC**: WorldDirector 通过注册系统查询可用 NPC（按 Profession、Personality、关系状态筛选）
- **Runtime 生成**: 故事需要世界中不存在的角色时，动态 Spawn（带生成的 Personality + RoleDescription）

### 5.2 场所匹配

给 `NarrativeSceneAnchor` 打语义标签:

```
SceneAnchor_Tavern     → Tags: [Indoor, Social, Public]
SceneAnchor_Alley      → Tags: [Outdoor, Private, Shady]
SceneAnchor_Square     → Tags: [Outdoor, Social, Public, Large]
SceneAnchor_Forest     → Tags: [Outdoor, Private, Remote]
```

LLM 输出场景需求标签，C++ 匹配最近的合适 Anchor。

---

## 6. 与现有系统的关系

| 现有系统 | 角色不变 | WorldDirector 新增的交互 |
|:---|:---|:---|
| NarrativeDirectorSubsystem | 历史记录 + 世界状态 | WorldDirector 读取其数据做张力评估 |
| NarrativeSquadSubsystem | Scene 执行器 | WorldDirector 向其提交动态生成的 Scene |
| NarrativeCompanion | 事件观察者 | 新增: 接收 WorldDirector 的 NarrativeHint |
| UtilityAI + Cognition | NPC 行为 + 语言 | 不变，Scene 内 NPC 照常自主演出 |
| LLMCommunicator | 双模式 API | WorldDirector 用 FunctionalRequest 做剧情实例化 |

---

## 7. 待解决问题

- [ ] WorldDirector 张力评估的具体公式和刷新频率
- [ ] LLM 实例化的 Prompt 结构和输出 Schema
- [ ] C++ 如何将 LLM 的叙事意图转化为 `FNarrativeSceneDef`
- [ ] NPC 注册/查询系统的接口设计
- [ ] Companion 如何接收和消化 NarrativeHint
- [ ] 固定时间轴锚点事件的配置方式
- [ ] Scene 结果如何量化反馈给张力系统
