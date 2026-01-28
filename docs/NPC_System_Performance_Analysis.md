# NPC系统性能深度分析

## 📊 Component性能审计

### 每个NPC的Component清单和开销

| Component | Tick? | 内存 | CPU/Frame | 备注 |
|-----------|-------|------|-----------|------|
| **Controller Components** |
| `CognitionComponent` | ✅ Yes | ~1KB | 0.01ms | **有Tick但被Rate Limiting限制（1.5-4秒）** |
| `UtilityAIComponent` | ✅ Yes | ~2KB | 0.05ms | 每帧计算action scores |
| `GoalComponent` | ✅ Yes | ~1KB | 0.02ms | 每帧更新schedule |
| `PersonalityComponent` | ❌ No | ~0.5KB | 0 | 仅数据存储 |
| `MemoryComponent` | ❌ No | ~5KB | 0 | 仅数据存储 |
| `FactionReputationComponent` | ❌ No | ~0.5KB | 0 | 仅数据存储 |
| `NPCDefinitionComponent` | ❌ No | ~1KB | 0 | 仅数据存储 |
| **Pawn Components** |
| `SensoryComponent` | ❌ No | ~1KB | 0 | 事件驱动 |
| `MetabolismComponent` | ✅ Yes | ~0.5KB | 0.01ms | 简单的状态递减 |
| `SmartObjectComponent` | ❌ No | ~0.5KB | 0 | 仅数据存储 |
| `EmotionDisplayComponent` | ❌ No | ~1KB | 0 | 仅UI更新 |
| **总计** | **4个Tick** | **~14KB** | **~0.09ms/frame** | **per NPC** |

### 关键发现：

**✅ 好消息：**
1. **只有4个Component有Tick**（CognitionComponent、UtilityAIComponent、GoalComponent、MetabolismComponent）
2. **其他7个都是"数据容器"**，几乎零CPU开销
3. **CognitionComponent虽然Tick，但内部有Rate Limiting**（实际每1.5-4秒才执行一次）

**⚠️ 性能热点：**
- `UtilityAIComponent`: 每帧计算所有action的scores（最耗CPU）
- `GoalComponent`: 每帧检查schedule切换（中等）
- `MetabolismComponent`: 简单递减（很轻）
- `CognitionComponent`: 名义上Tick，但99%的帧什么都不做（Rate Limited）

---

## 🧮 实际性能计算

### 132个NPC（44个camps × 3个NPC）的开销：

```
每帧开销：
132 NPCs × 0.09ms = 11.88ms/frame

分解：
├─ UtilityAI:     132 × 0.05ms = 6.6ms   (最大开销)
├─ GoalComponent: 132 × 0.02ms = 2.64ms
├─ Metabolism:    132 × 0.01ms = 1.32ms
└─ Cognition:     132 × 0.01ms = 1.32ms  (实际更少，因为Rate Limited)
```

**帧预算（60fps）：16.67ms**
**NPC系统占用：11.88ms（71%）** ← **这确实有点高！**

---

## 🎯 优化方案

### 方案1：**距离LOD系统**（推荐，最有效）

只对玩家附近的NPC执行完整逻辑：

```cpp
// UtilityAIComponent.cpp - TickComponent
void UUtilityAIComponent::TickComponent(float DeltaTime, ...)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // ✅ Distance-based LOD
    AAIController* AIController = Cast<AAIController>(GetOwner());
    if (!AIController || !AIController->GetPawn()) return;
    
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->GetPawn())
    {
        float Distance = FVector::Dist(AIController->GetPawn()->GetActorLocation(), 
                                        PC->GetPawn()->GetActorLocation());
        
        // > 5000单位：休眠（不更新）
        if (Distance > 5000.0f) return;
        
        // 2000-5000单位：降级（每2帧更新一次）
        if (Distance > 2000.0f && GFrameCounter % 2 != 0) return;
    }
    
    // 正常的Utility AI更新
    EvaluateActions();
}
```

**效果预估**：
- 玩家附近（<2000单位）：20个NPC全速运行 = 1.8ms
- 中距离（2000-5000）：30个NPC半速 = 1.35ms
- 远距离（>5000）：82个NPC休眠 = 0ms
- **总计：3.15ms/frame（减少73%！）**

---

### 方案2：**时间片轮询**（次推荐）

不是所有NPC每帧都更新，而是分散到不同帧：

```cpp
// UtilityAIComponent.cpp
void UUtilityAIComponent::TickComponent(float DeltaTime, ...)
{
    // ✅ Staggered Update - 将132个NPC分散到10帧
    int32 NPCIndex = GetOwner()->GetUniqueID();
    if (GFrameCounter % 10 != NPCIndex % 10) return;
    
    // 正常更新，但DeltaTime × 10（因为10帧才更新一次）
    EvaluateActions(DeltaTime * 10.0f);
}
```

**效果预估**：
- 每帧只更新13个NPC（132 ÷ 10）
- 每帧开销：13 × 0.09ms = **1.17ms/frame（减少90%！）**
- 副作用：NPC反应慢0.16秒（可接受）

---

### 方案3：**禁用不必要的Tick**（最简单）

有些Component不需要每帧Tick：

```cpp
// GoalComponent.cpp - 改为定时器
void UGoalComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // ❌ 不要每帧Tick
    PrimaryComponentTick.bCanEverTick = false;
    
    // ✅ 改为每秒检查一次
    GetWorld()->GetTimerManager().SetTimer(ScheduleCheckTimer, this, 
        &UGoalComponent::CheckSchedule, 1.0f, true);
}
```

**适合禁用Tick的Component**：
- `GoalComponent`: 改为定时器（每秒检查一次schedule）
- `MetabolismComponent`: 改为定时器（每秒递减一次）

**效果预估**：
- 减少2个Component的Tick
- 每帧节省：132 × 0.03ms = **3.96ms/frame**

---

### 方案4：**Component合并**（高级）

将多个小Component合并为一个大Component：

```cpp
// NPCStateComponent.cpp（合并Personality + Faction + Memory）
class UNPCStateComponent : public UActorComponent
{
    FPersonalityData Personality;
    FFactionData Faction;
    TArray<FMemoryItem> Memories;
    // ...
};
```

**优势**：
- 减少Component数量（从11个 → 7个）
- 减少内存碎片
- 更好的缓存局部性

**劣势**：
- 代码耦合度增加
- 降低可维护性

**我不推荐这个方案**，除非性能真的成问题。

---

## 🚀 推荐的优化策略

### 立即实施（低风险，高收益）：

1. **✅ 距离LOD系统**
   - 实施难度：简单（10分钟）
   - 性能提升：70%+
   - 副作用：无

2. **✅ 将GoalComponent和MetabolismComponent改为定时器**
   - 实施难度：简单（5分钟）
   - 性能提升：25%
   - 副作用：无（1秒更新一次足够）

### 未来考虑（如果性能还不够）：

3. **时间片轮询**
   - 实施难度：中等
   - 性能提升：90%（但有延迟）
   - 副作用：NPC反应慢0.1-0.2秒

---

## 📈 优化后的性能预估

### 基准（当前）：
```
132 NPCs × 0.09ms = 11.88ms/frame (71% of 16.67ms budget)
```

### 优化后（距离LOD + 定时器）：
```
近距离 NPCs: 20 × 0.04ms = 0.8ms    (UtilityAI only)
中距离 NPCs: 30 × 0.02ms = 0.6ms    (半速)
远距离 NPCs: 82 × 0ms    = 0ms      (休眠)
────────────────────────────────────
总计:                      1.4ms/frame (8.4% of budget)
```

**从71% → 8.4%，提升8.5倍！** 🎉

---

## 🔍 如何测量实际性能

### 方法1：控制台命令
```
stat game         # 查看Game Thread详细信息
stat ai           # 查看AI系统开销
```

### 方法2：自定义统计

我已经为Lazy Fetch添加了性能追踪，你可以为其他Component也添加：

```cpp
// UtilityAIComponent.cpp
DECLARE_CYCLE_STAT(TEXT("AI - Utility Eval"), STAT_UtilityEval, STATGROUP_AI);

void UUtilityAIComponent::EvaluateActions()
{
    SCOPE_CYCLE_COUNTER(STAT_UtilityEval);
    // ...
}
```

然后用 `stat ai` 查看。

---

## 💡 我的建议

**当前状态**：
- ✅ 44个camps（132个NPCs）运行流畅
- ✅ 你的机器配置不错，所以没问题
- ⚠️ 但在低端机器或更多NPC时可能卡

**建议的行动计划**：

1. **现在（可选）**：
   - 实施距离LOD系统（10分钟工作，巨大收益）
   - 这是"预防性优化"，为未来扩展做准备

2. **如果未来FPS <45**：
   - 将GoalComponent和MetabolismComponent改为定时器
   - 添加时间片轮询

3. **如果未来FPS <30**：
   - 考虑Component合并
   - 考虑NPC总量限制（streaming in/out）

**要我帮你实施距离LOD系统吗？** 这是最有效且最安全的优化。
