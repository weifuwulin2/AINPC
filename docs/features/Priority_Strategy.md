# 优先级策略总结 (Priority Strategy Summary)

## 最终设计 (Final Design)

### 优先级分级 (Priority Levels)

| 目标类型 | Magnitude | 即时响应 | 累积阈值 | 说明 |
|---------|-----------|---------|---------|------|
| **敌对阵营** | **0.8** | ✅ **是** | - | 真正的威胁，立即触发认知事件 |
| **玩家** | **0.4** | ❌ 否 | 3 次 | 中等优先级，给 LLM 思考空间 |
| **同阵营** | **0.05** | ❌ 否 | 3 次 | 低优先级，避免过度关注 |
| **中立/未知** | **0.1** | ❌ 否 | 3 次 | 默认低优先级 |

---

## 设计理由 (Design Rationale)

### 1. 敌对阵营 = 高优先级 (0.8)

**中文**:
- **为什么高优先级？** 敌对阵营代表真正的威胁，需要立即反应
- **示例场景**:
  - 僵尸看到人类守卫 → 立即攻击
  - 人类守卫看到僵尸 → 立即警戒/攻击
  - 怪物看到人类 → 立即追击

**English**:
- **Why high priority?** Hostile factions represent real threats requiring immediate reaction
- **Example scenarios**:
  - Zombie sees human guard → Immediate attack
  - Human guard sees zombie → Immediate alert/attack
  - Monster sees human → Immediate chase

---

### 2. 玩家 = 中等优先级 (0.4)

**中文**:
- **为什么中等优先级？** 玩家虽然重要，但不应立即敌对，给 LLM 根据性格和情境决定反应
- **好处**:
  - ✅ 友好 NPC 看到玩家可能打招呼，而不是攻击
  - ✅ 商人看到玩家可能推销商品
  - ✅ 胆小的 NPC 看到玩家可能逃跑或躲藏
  - ✅ 僵尸看到玩家会累积观察，然后根据 LLM 决定是攻击还是忽略
- **示例场景**:
  - 友好守卫看到玩家 → 累积 3 次后 LLM 决定："打招呼"
  - 商人看到玩家 → 累积 3 次后 LLM 决定："推销商品"
  - 僵尸看到玩家 → 累积 3 次后 LLM 决定："攻击"（基于 Intention 系统）

**English**:
- **Why medium priority?** Player is important but shouldn't be immediately hostile, let LLM decide reaction based on personality and context
- **Benefits**:
  - ✅ Friendly NPC seeing player might greet instead of attack
  - ✅ Merchant seeing player might promote goods
  - ✅ Timid NPC seeing player might flee or hide
  - ✅ Zombie seeing player will accumulate observations, then LLM decides to attack or ignore
- **Example scenarios**:
  - Friendly guard sees player → After 3 accumulations, LLM decides: "Greet"
  - Merchant sees player → After 3 accumulations, LLM decides: "Promote goods"
  - Zombie sees player → After 3 accumulations, LLM decides: "Attack" (based on Intention system)

---

### 3. 同阵营 = 低优先级 (0.05)

**中文**:
- **为什么低优先级？** 避免同阵营 NPC 互相攻击，减少不必要的 LLM 调用
- **示例场景**:
  - 僵尸 A 看到僵尸 B → 累积 3 次后才触发认知事件（可能忽略或打招呼）
  - 人类守卫 A 看到人类守卫 B → 累积 3 次后才触发认知事件（可能打招呼）

**English**:
- **Why low priority?** Avoid same-faction NPCs attacking each other, reduce unnecessary LLM calls
- **Example scenarios**:
  - Zombie A sees Zombie B → After 3 accumulations, trigger cognitive event (might ignore or greet)
  - Human guard A sees Human guard B → After 3 accumulations, trigger cognitive event (might greet)

---

## 行为对比表 (Behavior Comparison Table)

### 僵尸的视角 (Zombie's Perspective)

| 看到的目标 | 阵营 | Magnitude | 即时响应 | 行为 |
|-----------|------|-----------|---------|------|
| 人类守卫 | Human | **0.8** | ✅ 是 | **立即攻击** |
| 玩家 | Human (Player tag) | **0.4** | ❌ 否 | 累积 3 次后 LLM 决定（通常攻击） |
| 另一个僵尸 | Zombie | **0.05** | ❌ 否 | 累积 3 次后 LLM 决定（通常忽略） |
| 中立商人 | Neutral | **0.1** | ❌ 否 | 累积 3 次后 LLM 决定 |

### 人类守卫的视角 (Human Guard's Perspective)

| 看到的目标 | 阵营 | Magnitude | 即时响应 | 行为 |
|-----------|------|-----------|---------|------|
| 僵尸 | Zombie | **0.8** | ✅ 是 | **立即警戒/攻击** |
| 玩家 | Human (Player tag) | **0.4** | ❌ 否 | 累积 3 次后 LLM 决定（通常打招呼） |
| 另一个守卫 | Human | **0.05** | ❌ 否 | 累积 3 次后 LLM 决定（通常打招呼） |
| 中立商人 | Neutral | **0.1** | ❌ 否 | 累积 3 次后 LLM 决定 |

### 友好商人的视角 (Friendly Merchant's Perspective)

| 看到的目标 | 阵营 | Magnitude | 即时响应 | 行为 |
|-----------|------|-----------|---------|------|
| 僵尸 | Zombie | **0.1** | ❌ 否 | 累积 3 次后 LLM 决定（可能逃跑） |
| 玩家 | Human (Player tag) | **0.4** | ❌ 否 | 累积 3 次后 LLM 决定（通常推销） |
| 人类守卫 | Human | **0.1** | ❌ 否 | 累积 3 次后 LLM 决定 |
| 另一个商人 | Neutral | **0.1** | ❌ 否 | 累积 3 次后 LLM 决定 |

**注意**: 商人是中立阵营，所以即使看到僵尸也不会触发即时响应（因为 `AreActorsHostile` 对中立阵营返回 false）

---

## 代码实现 (Code Implementation)

### HandleTargetPerceived 中的 Magnitude 计算

```cpp
// 优先级 1: 检查阵营敌对关系（最高优先级）
if (AreActorsHostile(GetOwner(), Actor))
{
    Magnitude = 0.8f; // 敌对阵营：高重要性，触发即时响应
}
// 优先级 2: 玩家是中等优先级（不触发即时响应，但比友军重要）
else if (Actor->ActorHasTag("Player"))
{
    Magnitude = 0.4f; // 玩家：中等重要性，不触发即时响应
}
// 优先级 3: 同阵营（友军）
else
{
    FName SelfFaction = GetActorFaction(GetOwner());
    FName TargetFaction = GetActorFaction(Actor);
    
    if (SelfFaction == TargetFaction && SelfFaction != "Neutral")
    {
        Magnitude = 0.05f; // 同阵营：低重要性
    }
    else
    {
        Magnitude = 0.1f; // 中立或未知：默认低重要性
    }
}
```

### ProcessEventFilter 中的即时响应判断

```cpp
// 高 Magnitude (>= 0.5) 表示高优先级目标
bool bIsHighPriority = Event.Magnitude >= 0.5f;

if (bIsHighPriority)
{
    // 立即触发认知事件
    return true;
}
```

---

## 优势 (Advantages)

### 1. 更智能的玩家交互
**中文**: 不同性格的 NPC 对玩家有不同的反应，而不是一律敌对或一律友好。

**English**: Different personality NPCs react differently to players, instead of uniformly hostile or friendly.

### 2. 真正的威胁立即响应
**中文**: 敌对阵营会立即触发认知事件，确保 AI 快速反应。

**English**: Hostile factions immediately trigger cognitive events, ensuring quick AI response.

### 3. 减少不必要的 LLM 调用
**中文**: 同阵营和中立目标需要累积才触发，避免频繁的 LLM 调用。

**English**: Same-faction and neutral targets require accumulation to trigger, avoiding frequent LLM calls.

### 4. 灵活的行为空间
**中文**: LLM 可以根据性格、情境、Intention 系统决定对玩家的反应。

**English**: LLM can decide reaction to player based on personality, context, and Intention system.

---

## 配置建议 (Configuration Recommendations)

### 如果你想让僵尸立即攻击玩家

**选项 1**: 将玩家的阵营设置为与僵尸敌对
```
// 在 DataTable 中
Player Faction: "Human"
Zombie Faction: "Zombie"
```

**选项 2**: 给玩家添加 "Enemy" 标签（但这会让所有 NPC 都立即敌对）

**选项 3**: 调整 `HighPriorityMagnitude` 参数
```cpp
// 在 SensoryComponent 中
HighPriorityMagnitude = 0.3f; // 降低阈值，让玩家也触发即时响应
```

### 如果你想让友好 NPC 立即欢迎玩家

**选项 1**: 在 `HandleTargetPerceived` 中添加特殊逻辑
```cpp
// 检查自己是否是友好阵营
if (Actor->ActorHasTag("Player") && GetActorFaction(GetOwner()) == "Human")
{
    Magnitude = 0.8f; // 友好阵营看到玩家，高优先级
}
```

---

**版本 (Version)**: 1.0  
**最后更新 (Last Updated)**: 2026-01-09
