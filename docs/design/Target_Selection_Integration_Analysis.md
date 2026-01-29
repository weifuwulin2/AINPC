# Universal Target Selection - 架构集成分析

## 🤔 核心问题

**Q1: 会不会和现有架构冲突？**  
**Q2: 根据周围情况的更新是如何做的？**

---

## 📊 现有架构分析

### 当前 AI 决策流程

```
┌──────────────────────────────────────────────────────────┐
│              UtilityAIComponent (每帧)                    │
├──────────────────────────────────────────────────────────┤
│  1. EvaluateAndDecide()                                  │
│     - 遍历 AvailableActions                              │
│     - 每个 Action 调用 ScoreAction(MentalState)          │
│     - 选择最高分的 Action                                 │
│                                                          │
│  2. CanTransition() 检查                                 │
│     - Priority: 新 Action 优先级 > 当前 Action           │
│     - Commitment: 当前 Action 是否可中断                 │
│     - Inertia: 避免频繁切换                              │
│                                                          │
│  3. SwitchAction()                                       │
│     - CurrentAction->Exit()                             │
│     - NewAction->Enter()                                │
└──────────────────────────────────────────────────────────┘
                            ↓
┌──────────────────────────────────────────────────────────┐
│              Action_Attack::Enter()                      │
├──────────────────────────────────────────────────────────┤
│  ❌ OLD: TargetActor = Controller->GetFocusActor();      │
│  ✅ NEW: TargetActor = TargetSystem->SelectTarget(...);  │
│                                                          │
│  Controller->SetFocus(TargetActor);                     │
└──────────────────────────────────────────────────────────┘
```

### 关键发现

**✅ 没有架构冲突！原因：**

1. **目标选择不在 Utility 评分中**
   - UtilityAI 只决定 "做什么" (Attack vs Flee vs Speak)
   - Action::Enter() 决定 "对谁做" (Target Selection)
   - **两者职责分离，互不干扰**

2. **现有 Actions 已经在选择目标**
   - `Action_Attack`: `GetFocusActor()` (粗暴)
   - `Action_TalkTo`: `SetFocus(CurrentTarget)` (手动设置)
   - **我们只是替换目标选择的实现，不改变调用时机**

3. **SetFocus 仍然保留**
   - 新系统选出目标后，仍调用 `SetFocus()`
   - 保持与现有系统的兼容性

---

## 🔄 周围情况更新机制

### 问题：如何感知周围变化并更新目标？

**答案：事件驱动 + 缓存失效**

### 更新触发链

```
┌──────────────────────────────────────────────────────────┐
│           周围情况变化的传播路径                          │
└──────────────────────────────────────────────────────────┘

【情况1：受到攻击】
SensoryComponent::OnTargetPerceptionUpdated()
  ↓ (检测到攻击刺激)
HealthComponent::OnDamageReceived(Damage, InstigatedBy)
  ↓ (新增)
TargetSelectionSubsystem::InvalidateCache(Controller)
  ↓ (清除缓存)
下次 Action::Execute() 调用 SelectTarget()
  ↓ (重新评分)
选择攻击者为新目标 ✅

【情况2：队友死亡】
SensoryComponent::HandleDeath(Victim, Killer)
  ↓ (记录死亡事件)
CognitionComponent::CommitMemory("Player killed Orc_1", Salience=9.0)
  ↓ (新增)
TargetSelectionSubsystem::InvalidateCache(Controller)
  ↓
下次 SelectTarget() 将 Killer 评分 +1000 (复仇)
选择 Killer 为目标 ✅

【情况3：Narrative 指令变化】
NarrativeSquadSubsystem::TriggerTimelineNode(NodeIndex)
  ↓ (应用 Directive.Combat)
GoalComponent::AddContextTag(Directive.Combat)
  ↓ (新增) 为所有 Squad 成员清除缓存
TargetSelectionSubsystem::InvalidateCache(SquadMember)
  ↓
下次 SelectTarget() 会根据新 Directive 选择目标
```

### 实时感知流程

```cpp
// SensoryComponent 已有的感知系统
void USensoryComponent::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    // 现有逻辑：记录感知到的 Actor
    // ...

    // ✅ 新增：如果是攻击刺激，清除缓存
    if (Stimulus.Type == EAIStimulus::Damage)
    {
        if (AAIController* AI = Cast<AAIController>(GetOwnerController()))
        {
            if (UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
            {
                TargetSystem->InvalidateCache(AI);
                NARRATIVE_LOG(Verbose, "Cache invalidated due to damage perception");
            }
        }
    }
}
```

---

## 🔗 集成点详解

### 1. UtilityAI 层（不变）

```cpp
// UtilityAIComponent::EvaluateAndDecide() - 完全不变
void UUtilityAIComponent::EvaluateAndDecide()
{
    // 评分所有 Actions
    for (UUtilityActionBase* Action : AvailableActions)
    {
        float Score = Action->ScoreAction(MentalState);
        // ... 选出最高分 Action
    }

    // CanTransition 检查
    if (CanTransition(CurrentAction, BestAction, BestScore))
    {
        SwitchAction(BestAction);  // 切换到新 Action
    }
}
```

**影响：无**  
Utility 评分不关心目标是谁，只评估 "Attack" 这个行为的价值。

---

### 2. Action 层（轻微修改）

#### 修改前（粗暴）

```cpp
void UAction_Attack::Enter_Implementation(AAIController* Controller)
{
    Super::Enter_Implementation(Controller);
    
    // ❌ 粗暴获取目标
    TargetActor = Controller->GetFocusActor();  
    
    if (!TargetActor)
    {
        AINPC_LOG_WARNING("No target focused!");
        return;
    }
}
```

#### 修改后（智能）

```cpp
void UAction_Attack::Enter_Implementation(AAIController* Controller)
{
    Super::Enter_Implementation(Controller);
    
    // ✅ 智能选择目标
    UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>();
    if (TargetSystem)
    {
        FTargetSelectionConfig Config;
        Config.bUseLLMEnhancement = false;  // Combat: Force Rule-Based
        Config.MaxDistance = 3000.0f;
        
        TargetActor = TargetSystem->SelectTarget(
            Controller,
            ETargetSelectionContext::Combat,
            Config
        );
    }
    
    if (!TargetActor)
    {
        NARRATIVE_LOG(Warning, "No valid combat target found!");
        return;
    }
    
    // ✅ 保持兼容性：设置 Focus
    Controller->SetFocus(TargetActor);
}
```

**影响：最小**  
只改变目标选择方式，不改变 Action 执行流程。

---

### 3. 感知层（新增缓存失效）

```cpp
// HealthComponent.cpp - 受到伤害时
void UHealthComponent::OnDamageReceived(float Damage, AController* InstigatedBy)
{
    // 现有逻辑：扣血、播放特效等
    CurrentHealth -= Damage;
    // ...
    
    // ✅ 新增：清除目标缓存（攻击者成为高优先级目标）
    if (AAIController* AI = Cast<AAIController>(GetOwner()->GetInstigatorController()))
    {
        if (UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
        {
            TargetSystem->InvalidateCache(AI);
        }
    }
}
```

**影响：无**  
只是添加缓存失效调用，不影响现有伤害系统。

---

## ⚙️ 数据流对比

### 旧系统（粗暴）

```
Utility 评分 → 选择 Attack Action → Enter() 
  → GetFocusActor() 
    → 返回上次设置的 Focus（可能已过时）
      → 攻击错误目标或无目标 ❌
```

### 新系统（智能）

```
Utility 评分 → 选择 Attack Action → Enter()
  → TargetSystem->SelectTarget()
    ↓
    1. 检查缓存（5秒内复用）
    ↓
    2. If 缓存失效 → 重新评分
       ├─ 获取 SensoryComponent->GetPerceivedActors()
       ├─ 过滤：移除死亡、友军、超距离
       ├─ 评分：
       │   ├─ Memory: +1000 (杀了队友)
       │   ├─ Faction: +0-100 (敌对度)
       │   ├─ Distance: +0-200 (近的优先)
       │   └─ Aggression: +300 (正在攻击我)
       └─ 返回最高分目标
    ↓
    3. SetFocus(新目标)
      → 攻击正确目标 ✅
```

---

## 🚨 潜在风险与缓解

### 风险1：频繁缓存失效导致性能下降

**场景**：100 NPC 同时战斗，频繁受伤 → 频繁清除缓存 → 频繁重新评分

**缓解**：
```cpp
// 添加 "最小缓存间隔"
float MinCacheInterval = 0.5f;  // 至少保持 0.5 秒

void InvalidateCache(AAIController* Controller)
{
    FTargetCacheKey Key = FindKey(Controller);
    float Age = CurrentTime - Key.Timestamp;
    
    if (Age < MinCacheInterval)
    {
        NARRATIVE_LOG(Verbose, "Cache too fresh, ignoring invalidate request");
        return;  // 太快了，暂不清除
    }
    
    TargetCache.Remove(Key);
}
```

---

### 风险2：LLM 调用阻塞 Tick

**场景**：Social Action 调用 LLM 选择谈话对象，阻塞 1-2 秒

**缓解**：
```cpp
// 强制降级策略已包含在设计中
if (Context == ETargetSelectionContext::Combat)
{
    bShouldUseLLM = false;  // Combat 永不使用 LLM
}

if (Candidates.Num() > 10)
{
    bShouldUseLLM = false;  // 候选太多，降级
}
```

---

### 风险3：与 GoalComponent 的 Directive 不一致

**场景**：Timeline 设置 `Directive.Combat`，但 Action 仍选择 Social 目标

**缓解**：
```cpp
// 在 CalculateTargetScore 中考虑 Directive
float UTargetSelectionSubsystem::CalculateTargetScore(...)
{
    // ...
    
    // ✅ 检查 GoalComponent 的当前 Directive
    UGoalComponent* GoalComp = MyPawn->FindComponentByClass<UGoalComponent>();
    if (GoalComp && GoalComp->ContextTags.HasTag(FSocialGameplayTags::Get().Directive_Combat))
    {
        // 战斗指令激活，提升敌对目标优先级
        if (Reputation < -30.0f)
        {
            Score += 200.0f;  // 额外奖励
        }
    }
}
```

---

## ✅ 结论

### 与现有架构的关系

| 系统 | 职责 | 集成方式 | 冲突？ |
|------|------|----------|--------|
| **UtilityAI** | 决定做什么 (Attack/Flee/Speak) | 无需修改 | ❌ 无 |
| **Actions** | 执行行为 + 选择目标 | 替换目标选择实现 | ❌ 无 |
| **Sensory** | 感知周围 | 添加缓存失效调用 | ❌ 无 |
| **Cognition** | 记忆管理 + LLM | 添加 SuggestTarget() | ❌ 无 |
| **GoalComponent** | Directive 管理 | 评分时参考 ContextTags | ❌ 无 |
| **NarrativeSquad** | 场景管理 | Directive 改变时清缓存 | ❌ 无 |

### 周围情况更新机制

**实时感知 → 事件触发 → 缓存失效 → 下次重评分**

1. **被攻击** → `HealthComponent::OnDamageReceived()` → 清缓存
2. **队友死亡** → `SensoryComponent::HandleDeath()` → 清缓存
3. **指令变化** → `NarrativeSquadSubsystem::TriggerTimelineNode()` → 清缓存
4. **新敌人出现** → 缓存自动过期（5秒） → 重新评分

**不会阻塞，不会冲突，完全兼容现有架构！** ✅

---

## 🎯 建议的实施顺序

1. **Phase 1**: 实现 TargetSelectionSubsystem（无 LLM）
2. **Phase 2**: 在 Action_Attack 中测试
3. **Phase 3**: 添加缓存失效到 HealthComponent
4. **Phase 4**: 确认无性能问题后，再添加 LLM
