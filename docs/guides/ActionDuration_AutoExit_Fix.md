# ✅ ActionDuration 自动退出修复

## 🐛 问题

设置了 `ActionDuration = 2.0` 后，日志显示：
```
Duration expired (2.2s), action should end soon
Duration expired (2.4s), action should end soon
...
Duration expired (6.6s), action should end soon
```

**持续时长到期后，动作没有自动退出**。

## 🔍 原因

之前的实现只是**打印日志**，但没有**强制降低得分**。

Utility AI 仍然认为这个动作得分最高（因为 Hunger 仍然 > 0），所以不会切换。

## ✅ 解决方案

### 1. 添加 `ShouldExit()` 方法

**Action_SmartObject.h**:
```cpp
// 检查是否应该退出（持续时长到期）
bool ShouldExit(AAIController* Controller) const;
```

**Action_SmartObject.cpp**:
```cpp
bool UAction_SmartObject::ShouldExit(AAIController* Controller) const
{
    // 如果没有设置持续时长，永不主动退出
    if (ActionDuration <= 0.0f)
    {
        return false;
    }

    // 如果还没开始交互，不退出
    if (!bIsInteracting)
    {
        return false;
    }

    // 检查是否超过持续时长
    if (Controller && Controller->GetWorld())
    {
        float ElapsedTime = Controller->GetWorld()->GetTimeSeconds() - ActionStartTime;
        return ElapsedTime >= ActionDuration;
    }

    return false;
}
```

### 2. 在 `CalculateScore` 中检查

**UtilityActionBase.cpp**:
```cpp
float UUtilityActionBase::CalculateScore(...)
{
    // ... 其他检查 ...

    // ✅ 持续时长检查：如果持续时长已到期，得分为 0
    if (UAction_SmartObject* SmartObjectAction = Cast<UAction_SmartObject>(this))
    {
        if (SmartObjectAction->ShouldExit(Controller))
        {
            if (bLogDebug)
            {
                UE_LOG(LogTemp, Warning, TEXT("    [%s] ⏱️ Duration expired, forcing score to 0"), *ActionName);
            }
            return 0.0f;  // 强制得分为 0
        }
    }

    // ... 继续计算得分 ...
}
```

## 🎯 工作流程

### 修复前
```
Time: 0.0s → Action_Eat (Score: 2.0) ✅ 开始吃
Time: 2.0s → Duration expired! (但得分仍然是 2.0)
Time: 2.2s → Duration expired! (得分仍然是 2.0)
...
Time: 6.6s → Duration expired! (得分仍然是 2.0) ❌ 一直吃
```

### 修复后
```
Time: 0.0s → Action_Eat (Score: 2.0) ✅ 开始吃
Time: 1.8s → Action_Eat (Score: 2.0) ✅ 继续吃
Time: 2.0s → Duration expired!
Time: 2.0s → Action_Eat (Score: 0.0) ⏱️ 强制为 0
Time: 2.0s → Test_Idle (Score: 0.3) ✅ 切换到 Idle
```

## 📊 预期日志

```
[Action_Eat] Eating... Hunger: 0.03 (Rate: 1.00/s)
[Action_Eat] Eating... Hunger: 0.02 (Rate: 1.00/s)
[Action_Eat] Eating... Hunger: 0.01 (Rate: 1.00/s)
[Action_Eat] Duration expired (2.0s), action should end soon
[UtilityAI] Evaluating actions...
    [Action_Eat] ⏱️ Duration expired, forcing score to 0
    [Test_Idle] Final Score: 0.30
[UtilityAI] 🏆 Best Action: Test_Idle (Score: 0.30)
[UtilityAI] ✅ Switch Action: Test_Idle
[Action_Eat] Stopped animation: AS_EatingOnParty_Montage
```

## 🎯 使用场景

### 场景 1: 固定时长吃饭
```
ActionDuration: 2.0  // 吃 2 秒就停
```
- NPC 会吃 2 秒，然后自动停止
- 即使还饿，也会停止（可能会再次选择吃饭）

### 场景 2: 吃到饱为止
```
ActionDuration: 0.0  // 无限持续
```
- NPC 会一直吃，直到 Hunger = 0
- 由 Utility AI 自然切换（Hunger 降低导致得分降低）

### 场景 3: 混合策略
```
ActionDuration: 5.0  // 最多吃 5 秒
```
- 如果 5 秒内吃饱了（Hunger = 0），自然切换
- 如果 5 秒后还没吃饱，强制停止
- 可能会休息一下再继续吃

## 💡 推荐配置

### 吃饭动作
```
ActionDuration: 0.0  // 无限持续，吃到饱
```

### 喝水动作（快速）
```
ActionDuration: 2.0  // 喝水很快，2 秒就够
```

### 睡觉动作
```
ActionDuration: 0.0  // 无限持续，睡到恢复精力
```

### 社交动作
```
ActionDuration: 10.0  // 聊天 10 秒就够了
```

## ✅ 修复完成

现在 `ActionDuration` 会正确工作：
- ✅ 持续时长到期后，得分强制为 0
- ✅ Utility AI 自动切换到其他动作
- ✅ 动画正确停止
- ✅ 不再无限循环
