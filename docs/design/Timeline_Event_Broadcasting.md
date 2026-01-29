# Narrative Timeline Event Broadcasting Guide

## 概述

本文档说明如何在游戏中广播事件以触发 Narrative Timeline 节点。

---

## 🎯 自动事件广播（推荐）

### 1️⃣ **Event.PlayerDetected** (玩家检测)

**触发位置**: `NarrativeSceneAnchor::OnOverlapBegin`  
**触发条件**: 玩家进入 Trigger Sphere 范围

**自动处理流程**:
1. 玩家进入 `NarrativeSceneAnchor` 的 Trigger Sphere
2. `OnOverlapBegin` 检测到玩家
3. 激活场景 (`ActivateScene`)
4. **自动广播** `Event.PlayerDetected`
5. Timeline 系统接收事件，触发对应节点

**配置步骤**:
1. 在场景中放置 `NarrativeSceneAnchor` Actor
2. 设置 `SupportedSceneID` 为场景 ID (例如 `Scene_OrcRescue`)
3. 设置 `ActivationRadius` 调整触发范围（默认 2000 单位 = 20米）
4. 确保 `bAutoTriggerOnOverlap = true`

**无需额外代码！** ✅

---

## 🛠️ 手动事件广播

如果需要更复杂的触发逻辑（例如 `Event.GuardsHostile`），可以在其他系统中手动广播。

### 示例1：守卫敌对事件（伤害触发）

在 `HealthComponent` 或 `CombatComponent` 中：

```cpp
// HealthComponent.cpp - OnDamageReceived
void UHealthComponent::OnDamageReceived(float Damage, const UDamageType* DamageType, AController* InstigatedBy)
{
    // ... 现有逻辑 ...
    
    // ✅ 广播守卫敌对事件
    if (GetOwner()->ActorHasTag("Guard") || GetOwner()->ActorHasTag("Lord"))
    {
        if (UWorld* World = GetWorld())
        {
            if (UNarrativeDirectorSubsystem* Director = World->GetSubsystem<UNarrativeDirectorSubsystem>())
            {
                FNarrativeEvent Event;
                Event.Tags.Add("Event.GuardsHostile");
                Event.Description = FString::Printf(TEXT("%s became hostile after taking damage"), *GetOwner()->GetName());
                Event.Timestamp = World->GetTimeSeconds();
                Director->RecordEvent(Event);
                
                UE_LOG(LogTemp, Warning, TEXT("📡 Broadcast Event.GuardsHostile"));
            }
        }
    }
}
```

### 示例2：玩家未离开警告（定时器）

在 `NarrativeSceneAnchor` 或自定义 `WarningComponent` 中：

```cpp
// 玩家靠近后，启动 5 秒倒计时
void ANarrativeSceneAnchor::OnPlayerDetected()
{
    GetWorld()->GetTimerManager().SetTimer(
        WarningTimer,
        [this]()
        {
            // 检查玩家是否仍在范围内
            if (IsPlayerStillNear())
            {
                if (UNarrativeDirectorSubsystem* Director = GetWorld()->GetSubsystem<UNarrativeDirectorSubsystem>())
                {
                    FNarrativeEvent Event;
                    Event.Tags.Add("Event.PlayerStillNear");
                    Director->RecordEvent(Event);
                }
            }
        },
        5.0f,  // 5 秒后触发
        false  // 不循环
    );
}
```

### 示例3：劫匪被攻击（全队检测）

在 `NarrativeSquadSubsystem` 中监听成员受到伤害：

```cpp
// NarrativeSquadSubsystem.cpp - 添加新函数
void UNarrativeSquadSubsystem::OnSquadMemberDamaged(int32 SquadID, AActor* DamagedMember)
{
    if (FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID))
    {
        if (UNarrativeDirectorSubsystem* Director = GetWorld()->GetSubsystem<UNarrativeDirectorSubsystem>())
        {
            FNarrativeEvent Event;
            Event.Tags.Add("Event.BanditAttacked");
            Event.Description = FString::Printf(TEXT("Squad %d member %s was attacked"), SquadID, *DamagedMember->GetName());
            Director->RecordEvent(Event);
        }
    }
}
```

---

## 📋 事件与 Timeline 配置示例

### DT_NarrativeScenes.json
```json
{
  "Timeline": [
    {
      "TimeOffset": 10.0,
      "TriggerCondition": "Event.PlayerDetected",
      "PlotUpdate": "Guards spotted the player!",
      "DirectiveOverride": "Directive.Social"
    },
    {
      "TimeOffset": 5.0,
      "TriggerCondition": "Event.GuardsHostile",
      "PlotUpdate": "Combat erupts!",
      "DirectiveOverride": "Directive.Combat"
    }
  ]
}
```

### 运行时行为
1. **T=0**: 场景激活
2. **T=8s**: 玩家进入 NarrativeSceneAnchor → 广播 `Event.PlayerDetected`
   - Timeline 节点1等待中（需要 T≥10s）
3. **T=10s**: 时间到达 → **节点1触发** → 守卫开始对话
4. **T=12s**: 守卫受到伤害 → 广播 `Event.GuardsHostile`
   - Timeline 节点2等待中（需要 T≥5s）
5. **T=12s**: 时间早已满足 → **节点2立即触发** → 守卫进入战斗

---

## 🎮 实战建议

### ✅ **推荐做法**
1. **玩家检测事件** → 使用 `NarrativeSceneAnchor` 自动广播
2. **伤害/战斗事件** → 在 `HealthComponent` 或 `CombatComponent` 中广播
3. **复杂条件事件** → 在 `GoalComponent` 或 `UtilityAIController` 中广播

### ⚠️ **注意事项**
1. **事件名称匹配**: 确保广播的 `Event.Tags` 中的 FName 与 Timeline `TriggerCondition` 的 FGameplayTag 名称一致
   - 例如: `Event.Tags.Add("Event.PlayerDetected")` 匹配 `TriggerCondition = "Event.PlayerDetected"`
2. **避免重复广播**: 同一事件不要在短时间内重复广播（添加 Cooldown）
3. **调试日志**: 使用 `UE_LOG` 输出事件广播信息，方便调试

---

## 🔍 调试 Timeline 事件

### 查看日志
```
📜 Timeline Node 2 (T+10.0s): Waiting for event Event.PlayerDetected
📡 [NarrativeAnchor] Broadcast Event.PlayerDetected for Scene Scene_OrcRescue
📜 Event Trigger Matched! Node 2 triggered by Event.PlayerDetected for Squad 1
📜 ⚡ Timeline Node 2 Triggered (T+12.3s): A stranger has been spotted near the camp!
```

### 常见问题
**Q: 事件广播了但节点没触发？**  
A: 检查以下几点：
1. 时间是否到达 `TimeOffset`？
2. 事件名称是否精确匹配？（区分大小写）
3. 场景是否已激活？(`ActivateScene` 调用了吗)
4. Timeline 是否有配置该节点？

**Q: 节点触发了但没有效果？**  
A: 检查 `PlotUpdate` 和 `DirectiveOverride` 是否正确配置。

---

## 📚 参考代码

- **自动广播**: `NarrativeSceneAnchor.cpp::OnOverlapBegin` (Line 99-122)
- **事件判定**: `NarrativeSquadSubsystem.cpp::OnNarrativeEventRecorded` (Line 156-209)
- **Timeline 处理**: `NarrativeSquadSubsystem.cpp::TickTimeline` (Line 788-823)
