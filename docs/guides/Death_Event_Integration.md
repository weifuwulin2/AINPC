# Death Event Integration Guide / 死亡事件集成指南

## Overview / 概述

The system now supports broadcasting death events as `FSemanticEvent`, allowing NPCs to remember and react to deaths (both their own and witnessed deaths).

系统现在支持将死亡事件作为 `FSemanticEvent` 广播，允许 NPC 记住并对死亡做出反应（包括自己的死亡和目睹的死亡）。

---

## Implementation / 实现方式

### Method 1: Blueprint (推荐 / Recommended)

在你的 Character Blueprint 中：

1. **找到死亡逻辑位置**
   - 通常在 `Event AnyDamage` 或自定义的 `Die` 函数中
   - 或者在 Health 降到 0 时

2. **添加节点调用**
   ```
   Event Graph:
   
   [Health <= 0] 
        ↓
   [Get AI Controller]
        ↓
   [Get Component by Class] → SensoryComponent
        ↓
   [Handle Death]
        - Dead Actor: Self
        - Killer: Damage Causer (from Event AnyDamage)
   ```

3. **广播给附近 NPC（可选）**
   ```
   [Get All Actors of Class] → AI Character
        ↓
   [For Each Loop]
        ↓
   [Get Distance]
        ↓
   [If Distance < 1000] → 调用他们的 SensoryComponent->HandleDeath
   ```

### Method 2: C++ (高级 / Advanced)

在你的 Character 类中：

```cpp
// YourCharacter.h
UFUNCTION()
void OnDeath(AActor* Killer);

// YourCharacter.cpp
void AYourCharacter::OnDeath(AActor* Killer)
{
    // 1. 通知自己的 AI Controller
    if (AUtilityAIController* MyController = Cast<AUtilityAIController>(GetController()))
    {
        if (USensoryComponent* SensoryComp = MyController->SensoryComp)
        {
            SensoryComp->HandleDeath(this, Killer);
        }
    }

    // 2. 通知附近的 NPC 目睹了这次死亡
    TArray<AActor*> NearbyNPCs;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AYourCharacter::StaticClass(), NearbyNPCs);
    
    for (AActor* Actor : NearbyNPCs)
    {
        if (Actor == this) continue; // 跳过自己
        
        float Distance = FVector::Dist(GetActorLocation(), Actor->GetActorLocation());
        if (Distance > 1000.0f) continue; // 太远看不到
        
        if (AYourCharacter* OtherChar = Cast<AYourCharacter>(Actor))
        {
            if (AUtilityAIController* OtherController = Cast<AUtilityAIController>(OtherChar->GetController()))
            {
                if (USensoryComponent* OtherSensory = OtherController->SensoryComp)
                {
                    OtherSensory->HandleDeath(this, Killer);
                }
            }
        }
    }

    // 3. 播放死亡动画等...
}
```

---

## Event Tags / 事件标签

系统会自动使用以下标签：

| Tag | 用途 | Magnitude |
|-----|------|-----------|
| `Event.Death.Self` | 自己死亡 | 1.0 (最高) |
| `Event.Death.Witnessed` | 目睹他人死亡 | 0.7 (高) |

---

## Memory Impact / 记忆影响

死亡事件会：
1. **立即存入 MemoryStream**（不经过过滤）
2. **高重要性分数**（自己死亡 = 10.0，目睹 = 7.0）
3. **快速触发 Reflection**（因为重要性累积快）

LLM 可能会生成反思如：
- "I died to the player - they are extremely dangerous"
- "I witnessed my ally die - I should flee"

---

## Testing / 测试

1. **编译项目**
2. **在 Character Blueprint 中添加死亡调用**
3. **运行游戏，杀死一个 NPC**
4. **检查日志**：
   ```
   [Sensory] SELF DEATH EVENT: I have died (killed by PlayerCharacter)
   [Memory] Stored: I have died... (Imp: 10.0)
   [Memory] 'Slow System' Reflection Triggered! (Sum: 10.0)
   ```

5. **调用 MemoryComponent->DumpMemoryLog()** 查看记忆

---

## Notes / 注意事项

- 死亡事件**不会被过滤**（`ProcessEventFilter` 被跳过）
- 自己死亡后，`CalculateScore` 会返回 0，防止执行任何动作
- 建议在死亡动画播放**之前**调用 `HandleDeath`，确保事件被记录
