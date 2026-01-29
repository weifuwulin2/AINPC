# Attack Target Selection Design

## 问题描述
当前 `Action_Attack` 的目标选择过于简单：
```cpp
TargetActor = Controller->GetFocusActor();  // ❌ 粗暴递进选择
```

这导致：
- 忽略友好度（可能攻击中立或友好单位）
- 忽略记忆（忘记谁杀了队友）
- 忽略威胁度（不优先攻击高威胁目标）
- 忽略叙事上下文（Directive.Combat 指令的语境）

---

## 设计方案：智能目标选择系统

### 核心思路
在 `Enter_Implementation` 中，不使用 `GetFocusActor()`，而是调用新函数：
```cpp
TargetActor = SelectBestCombatTarget(Controller);
```

### 目标选择优先级（降序）

1. **记忆驱动** (最高优先级)
   - CognitionComponent 中有 "killed my ally" 记忆 → 优先攻击凶手
   - 有 "attacked me" 记忆 → 优先反击

2. **派系敌对**
   - FactionReputationComponent: Reputation < -50 → 敌对单位
   - 优先攻击敌对派系中**威胁度最高**的单位

3. **威胁度评估**
   - 距离近的敌人 > 距离远的敌人
   - 正在攻击我的敌人 > 未攻击的敌人
   - 生命值低的敌人（可快速击杀）

4. **叙事上下文**
   - 如果在 NarrativeSquad 中且有 Directive.Combat
   - 从 PlotOutline 中解析敌人身份（例如 "守卫"、"玩家"）

---

## 实现计划

### 1. 添加智能目标选择函数

**位置**: `Action_Attack.h/.cpp`

```cpp
// Action_Attack.h
private:
    /** 
     * Intelligently selects combat target based on:
     * - Memory (who killed allies, who attacked me)
     * - Faction reputation
     * - Threat level (distance, health, aggression)
     * - Narrative context
     */
    AActor* SelectBestCombatTarget(AAIController* Controller);
    
    /** Calculate threat score for a potential target */
    float CalculateThreatScore(AActor* PotentialTarget, APawn* MyPawn, 
                               UCognitionComponent* Cognition, 
                               UFactionReputationComponent* FactionComp);
```

### 2. 实现逻辑

```cpp
AActor* UAction_Attack::SelectBestCombatTarget(AAIController* Controller)
{
    APawn* MyPawn = Controller->GetPawn();
    if (!MyPawn) return nullptr;
    
    // 1. Get Components
    UCognitionComponent* Cognition = MyPawn->FindComponentByClass<UCognitionComponent>();
    UFactionReputationComponent* FactionComp = MyPawn->FindComponentByClass<UFactionReputationComponent>();
    USensoryComponent* Sensory = MyPawn->FindComponentByClass<USensoryComponent>();
    
    if (!Sensory) return nullptr;
    
    // 2. Get Perceived Actors
    TArray<AActor*> PerceivedActors = Sensory->GetPerceivedActors();
    
    AActor* BestTarget = nullptr;
    float BestScore = -1.0f;
    
    // 3. Score Each Potential Target
    for (AActor* Actor : PerceivedActors)
    {
        // Skip dead, self, allies
        if (!IsValidTarget(Actor, MyPawn, FactionComp)) continue;
        
        float Score = CalculateThreatScore(Actor, MyPawn, Cognition, FactionComp);
        
        if (Score > BestScore)
        {
            BestScore = Score;
            BestTarget = Actor;
        }
    }
    
    return BestTarget;
}
```

### 3. 威胁度评分算法

```cpp
float UAction_Attack::CalculateThreatScore(AActor* Target, APawn* MyPawn,
                                            UCognitionComponent* Cognition,
                                            UFactionReputationComponent* FactionComp)
{
    float Score = 0.0f;
    
    // ✅ 记忆驱动 (最高权重)
    if (Cognition)
    {
        FString TargetName = Target->GetName();
        
        // 检查是否有"杀死队友"记忆
        if (Cognition->HasMemoryAbout(TargetName, "killed"))
        {
            Score += 1000.0f;  // 极高优先级
        }
        
        // 检查是否有"攻击我"记忆
        if (Cognition->HasMemoryAbout(TargetName, "attacked me"))
        {
            Score += 500.0f;
        }
    }
    
    // ✅ 派系敌对度
    if (FactionComp)
    {
        FName MyFaction = FactionComp->GetFactionID();
        FName TargetFaction = GetActorFaction(Target);
        
        float Reputation = FactionComp->GetReputationWith(TargetFaction);
        
        if (Reputation < -50.0f)  // 敌对
        {
            Score += FMath::Abs(Reputation);  // -100 → +100 score
        }
        else if (Reputation > 50.0f)  // 友好
        {
            Score -= 10000.0f;  // 避免攻击友军
        }
    }
    
    // ✅ 距离因素（近距离威胁更高）
    float Distance = FVector::Dist(MyPawn->GetActorLocation(), Target->GetActorLocation());
    Score += (2000.0f - Distance) / 10.0f;  // 近的敌人 +200, 远的敌人 +0
    
    // ✅ 生命值（低血量优先击杀）
    UHealthComponent* TargetHealth = Target->FindComponentByClass<UHealthComponent>();
    if (TargetHealth)
    {
        float HealthPercent = TargetHealth->GetHealthPercent();
        Score += (1.0f - HealthPercent) * 100.0f;  // 低血量 +100
    }
    
    // ✅ 正在攻击我？
    AActor* TargetsFocus = GetActorFocus(Target);
    if (TargetsFocus == MyPawn)
    {
        Score += 300.0f;  // 优先反击
    }
    
    return Score;
}
```

### 4. 辅助函数

```cpp
bool UAction_Attack::IsValidTarget(AActor* Actor, APawn* MyPawn, UFactionReputationComponent* FactionComp)
{
    if (!Actor || Actor == MyPawn) return false;
    if (Actor->ActorHasTag("Dead")) return false;
    
    // 检查是否是友军
    if (FactionComp)
    {
        FName TargetFaction = GetActorFaction(Actor);
        float Reputation = FactionComp->GetReputationWith(TargetFaction);
        
        if (Reputation > 50.0f)  // 友好派系
        {
            return false;
        }
    }
    
    return true;
}

FName UAction_Attack::GetActorFaction(AActor* Actor)
{
    UFactionReputationComponent* FactionComp = Actor->FindComponentByClass<UFactionReputationComponent>();
    return FactionComp ? FactionComp->GetFactionID() : NAME_None;
}

AActor* UAction_Attack::GetActorFocus(AActor* Actor)
{
    AAIController* AI = Cast<AAIController>(Cast<APawn>(Actor)->GetController());
    return AI ? AI->GetFocusActor() : nullptr;
}
```

---

## 集成点

### 修改 `Enter_Implementation`

```cpp
void UAction_Attack::Enter_Implementation(AAIController* Controller)
{
    Super::Enter_Implementation(Controller);
    
    if (!Controller)
    {
        AINPC_LOG_ERROR("Action_Attack: Controller is null!");
        return;
    }

    OwningController = Controller;
    
    // ✅ 智能目标选择（替换粗暴的 GetFocusActor）
    TargetActor = SelectBestCombatTarget(Controller);
    
    bIsAttacking = false;
    bHasDealtDamage = false;
    
    if (!TargetActor)
    {
        AINPC_LOG_WARNING("Action_Attack: No valid combat target found!");
        return;
    }
    
    // 设置 Focus（用于朝向目标）
    Controller->SetFocus(TargetActor);

    AINPC_LOG(Log, "⚔️ ATTACK ACTION ENTERED - Target: %s (Selected via intelligent scoring)", 
        *TargetActor->GetName());
}
```

---

## 效果示例

### 场景1：复仇
```
[Memory] Orc_Guard 看到 Player 杀死了 Orc_Worker
[CombatTarget] Scoring:
  - Player: 1000 (killed ally) + 150 (distance) + 300 (attacking me) = 1450 ⭐
  - Wolf: 80 (hostile faction) + 200 (close) = 280
[Action_Attack] Selected Target: Player (智能选择)
```

### 场景2：威胁优先
```
[CombatTarget] Scoring:
  - Bandit_Archer (20m, 30% HP, attacking me): 100 + 70 + 300 = 470 ⭐
  - Bandit_Warrior (50m, 100% HP): 150 + 0 = 150
[Action_Attack] Selected Target: Bandit_Archer (优先击杀低血量威胁)
```

---

## 下一步
1. 实现 `SelectBestCombatTarget` 和辅助函数
2. 添加 HealthComponent 查询支持
3. 测试不同场景下的目标选择逻辑
4. 添加 NARRATIVE_LOG 输出目标选择原因
