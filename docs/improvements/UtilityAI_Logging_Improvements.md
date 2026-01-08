# Utility AI 日志改进 / Utility AI Logging Improvements

## 修改内容

为 `UtilityAIComponent` 的所有日志添加了 PersonalityID 标识，使日志更容易区分不同的 NPC。

## 修改的文件

- `UtilityAIComponent.cpp`

## 修改前的日志

```
[UtilityAI] Evaluating Actions (Count: 3)
[UtilityAI] Currently Running: Test_Attack
[UtilityAI] 🏆 Best Action: Test_Attack (Score: 1.500)
[UtilityAI] ✅ Switch Action: Test_Idle (Score: 0.500)
```

**问题**：无法区分是哪个 NPC 在做决策

## 修改后的日志

```
[UtilityAI|Warrior] Evaluating Actions (Count: 3)
[UtilityAI|Warrior] Currently Running: Test_Attack
[UtilityAI|Warrior] 🏆 Best Action: Test_Attack (Score: 1.500)
[UtilityAI|Warrior] ✅ Switch Action: Test_Idle (Score: 0.500)

[UtilityAI|Zombie] Evaluating Actions (Count: 3)
[UtilityAI|Zombie] Currently Running: Test_Flee
[UtilityAI|Zombie] 🏆 Best Action: Test_Flee (Score: 0.800)
```

**优势**：清楚地看到 Warrior 和 Zombie 各自的决策过程

## 实现细节

在 `EvaluateAndDecide()` 函数开始时获取 PersonalityID：

```cpp
// 获取 PersonalityID 用于日志
FString PersonalityID = "Unknown";
if (OwnerController && OwnerController->PersonalityComp)
{
    PersonalityID = OwnerController->PersonalityComp->PersonalityID.ToString();
}
```

然后在所有日志中使用：

```cpp
UE_LOG(LogTemp, Warning, TEXT("[UtilityAI|%s] Evaluating Actions (Count: %d)"), 
       *PersonalityID, AvailableActions.Num());
```

## 日志格式

所有 UtilityAI 相关的日志现在使用统一格式：

```
[UtilityAI|<PersonalityID>] <Message>
```

例如：
- `[UtilityAI|Warrior] 🏆 Best Action: Test_Attack (Score: 1.500)`
- `[UtilityAI|Zombie] ✅ Switch Action: Test_Flee (Score: 0.800)`
- `[UtilityAI|Merchant] Currently Running: Test_Idle`

## 好处

1. **易于调试**：快速识别哪个 NPC 在执行什么动作
2. **日志过滤**：可以通过 PersonalityID 过滤特定 NPC 的日志
3. **多 NPC 场景**：在有多个 NPC 的场景中，日志不会混乱
4. **问题定位**：更容易发现特定 NPC 的行为问题

## 示例场景

场景中有 1 个 Warrior 和 2 个 Zombie：

```
[UtilityAI|Warrior] 🏆 Best Action: Test_Attack (Score: 1.500)
[UtilityAI|Warrior] ✅ Switch Action: Test_Attack (Score: 1.500)
[Attack] Chasing Zombie... Distance: 300.0

[UtilityAI|Zombie] 🏆 Best Action: Test_Flee (Score: 0.800)
[UtilityAI|Zombie] ✅ Switch Action: Test_Flee (Score: 0.800)
[Flee] Running from Warrior (Dist: 250)...

[UtilityAI|Zombie] 🏆 Best Action: Test_Attack (Score: 1.200)
[UtilityAI|Zombie] ✅ Switch Action: Test_Attack (Score: 1.200)
[Attack] Chasing Player... Distance: 400.0
```

现在可以清楚地看到：
- Warrior 在攻击 Zombie
- 第一个 Zombie 在逃跑
- 第二个 Zombie 在攻击玩家

