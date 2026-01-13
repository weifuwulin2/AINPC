# Metabolism System Integration Summary

## ✅ 完成的修改

### 1. **SmartObjectComponent** - 可配置恢复速率
**文件**: `Source/AINPC/Components/SmartObjectComponent.h`

**修改**:
- 添加了 `RestoreValue` 属性（默认 0.2）
- 可在蓝图中配置每个物体的恢复速率
- 例如：苹果 = 0.1，大餐 = 0.8

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social", meta = (ClampMin = "0.0", ClampMax = "1.0"))
float RestoreValue = 0.2f;
```

### 2. **Action_SmartObject** - 动态读取恢复速率
**文件**: `Source/AINPC/Private/Actions/Action_SmartObject.cpp`

**修改**:
- `RestoreStats` 函数现在从 `SmartObjectComponent` 读取 `RestoreValue`
- 不再使用硬编码的 `0.2f`
- 日志输出包含恢复速率信息

```cpp
if (USmartObjectComponent* SmartComp = TargetSmartObject->FindComponentByClass<USmartObjectComponent>())
{
    RestoreRate = SmartComp->RestoreValue;
}
```

### 3. **UtilityAIController** - 自动禁用 Monster 的 Metabolism
**文件**: `Source/AINPC/Controller/UtilityAIController.cpp`

**修改**:
- 在 `BeginPlay` 中根据 Faction 自动配置 Metabolism
- Monster 阵营：禁用 Metabolism（不需要吃饭睡觉）
- Human/Neutral 阵营：启用 Metabolism

```cpp
if (Faction == EFactionType::Monster)
{
    MetabolismComp->SetComponentTickEnabled(false);
}
```

## 🎯 系统工作流程

### 阶段 1: 初始化
1. **UtilityAIController** 创建所有组件（包括 `MetabolismComponent`）
2. **BeginPlay** 时检查 `PersonalityComponent` 的 Faction
3. 如果是 Monster → 禁用 Metabolism
4. 如果是 Human/Neutral → 启用 Metabolism

### 阶段 2: 需求增长
1. **MetabolismComponent** 每帧 Tick
2. `Hunger` 和 `Energy` 按配置的速率增长
   - `HungerRate = 0.0005` → 约 33 分钟饿死
   - `EnergyRate = 0.0003` → 约 55 分钟累死

### 阶段 3: 动作选择
1. **UtilityAIComponent** 评估所有动作
2. `Action_Eat` 的得分 = `BaseReward × Hunger × HasFoodNearby`
3. 当 Hunger 高且附近有食物时，得分最高
4. AI 选择 `Action_Eat`

### 阶段 4: 执行动作
1. **Action_Eat** 进入 `Enter`
2. 通过 `SensoryComponent` 找到最近的 `Activity.Eat` 对象
3. 移动到该对象
4. 到达后，`Execute` 调用 `RestoreStats`

### 阶段 5: 恢复状态
1. **RestoreStats** 从 SmartObject 读取 `RestoreValue`
2. 每帧减少 `Hunger -= RestoreValue * DeltaTime`
3. 日志输出：`[Action_Eat] Eating... Hunger: 0.45 (Rate: 0.30/s)`
4. 当 Hunger 降低后，Utility AI 切换回 Idle

## 📊 Faction 与 Metabolism 对应关系

| Faction | Metabolism | 说明 |
|---------|-----------|------|
| **Human** | ✅ 启用 | 人类 NPC 需要吃饭睡觉 |
| **Neutral** | ✅ 启用 | 中立 NPC（商人、村民）需要吃饭睡觉 |
| **Monster** | ❌ 禁用 | 怪物（僵尸、野兽）不需要吃饭睡觉 |

## 🔧 配置示例

### 在蓝图中配置 Smart Object

#### 食物对象（苹果）
```
Components:
  - SmartObjectComponent
    - Semantics.SocialTag: Activity.Eat
    - RestoreValue: 0.1  // 慢速恢复
```

#### 食物对象（大餐）
```
Components:
  - SmartObjectComponent
    - Semantics.SocialTag: Activity.Eat
    - RestoreValue: 0.8  // 快速恢复
```

#### 床对象
```
Components:
  - SmartObjectComponent
    - Semantics.SocialTag: Activity.Rest
    - RestoreValue: 0.5  // 中速恢复
```

### 在 DataTable 中配置 Utility Actions

#### Action_Eat
```
Row Name: Action_Eat
ActionClass: UAction_SmartObject
SmartObjectTag: Activity.Eat
BaseReward: 2.0
Considerations:
  [0]:
    InputType: Hunger
    ConsiderationType: Motivation
    CurveType: Logistic
  [1]:
    InputType: HasFoodNearby
    ConsiderationType: Context
    CurveType: TargetThreshold
```

#### Action_Sleep
```
Row Name: Action_Sleep
ActionClass: UAction_SmartObject
SmartObjectTag: Activity.Rest
BaseReward: 2.0
Considerations:
  [0]:
    InputType: Energy
    ConsiderationType: Motivation
    CurveType: Logistic
  [1]:
    InputType: HasBedNearby
    ConsiderationType: Context
    CurveType: TargetThreshold
```

## 🧪 测试验证

### 1. 创建测试场景
- 放置一个 Human NPC（PersonalityID = "FriendlyMerchant"）
- 放置一个 Monster NPC（PersonalityID = "Zombie"）
- 放置食物和床

### 2. 观察日志
```
[UtilityAIController_1] Metabolism ENABLED (Faction: Human)
[UtilityAIController_2] Metabolism DISABLED (Faction: Monster)

[Metabolism] Hunger: 0.52...
[UtilityAI] Best Action: Action_Eat (Score: 2.34)
[Action_Eat] Moving to Smart Object: BP_Apple_C_1
[Action_Eat] Eating... Hunger: 0.48 (Rate: 0.10/s)
```

### 3. 预期行为
- ✅ Human NPC 会饿，会去吃饭
- ✅ Monster NPC 不会饿，不会去吃饭
- ✅ 不同食物恢复速度不同

## 🎉 总结

现在整个 Metabolism 系统已经完全打通：

1. ✅ **MetabolismComponent** 自动增长 Hunger/Energy
2. ✅ **UtilityAI** 根据需求选择 Eat/Sleep 动作
3. ✅ **Action_SmartObject** 执行吃饭/睡觉并恢复状态
4. ✅ **SmartObject** 可配置不同的恢复速率
5. ✅ **Monster 自动禁用** Metabolism

所有 NPC 都会自动接入这个系统，除了 Monster！
