# Metabolism + Eating/Sleeping 配置与测试指南

## ✅ 已完成的代码修改

### 1. `SmartObjectComponent.h`
- ✅ 添加了 `RestoreValue` 属性（可在蓝图中配置）
- 默认值：0.2（每秒恢复 20%）

### 2. `Action_SmartObject.cpp`
- ✅ 修改 `RestoreStats` 函数，从 SmartObject 读取 `RestoreValue`
- ✅ 改进日志输出，显示恢复速率

### 3. `MetabolismComponent`
- ✅ 已存在，自动增加 Hunger 和 Energy

## 📋 配置步骤

### A. 在编辑器中配置 Utility Actions DataTable

打开 `DT_UtilityActions`，添加两个新行：

#### Row 1: `Action_Eat`
```
ActionClass: UAction_SmartObject
SmartObjectTag: Activity.Eat
BaseReward: 2.0
Considerations:
  - InputType: Hunger
    ConsiderationType: Motivation
    CurveType: Logistic (或自定义曲线)
  - InputType: HasFoodNearby
    ConsiderationType: Context
    CurveType: TargetThreshold
```

#### Row 2: `Action_Sleep`
```
ActionClass: UAction_SmartObject
SmartObjectTag: Activity.Rest
BaseReward: 2.0
Considerations:
  - InputType: Energy
    ConsiderationType: Motivation
    CurveType: Logistic
  - InputType: HasBedNearby
    ConsiderationType: Context
    CurveType: TargetThreshold
```

### B. 在关卡中放置 Smart Objects

#### 1. 创建食物 Blueprint
- 添加 `SmartObjectComponent`
- 设置 `Semantics.SocialTag` = `Activity.Eat`
- 设置 `RestoreValue` = `0.3`（苹果）或 `0.8`（大餐）

#### 2. 创建床 Blueprint
- 添加 `SmartObjectComponent`
- 设置 `Semantics.SocialTag` = `Activity.Rest`
- 设置 `RestoreValue` = `0.5`（普通床）或 `1.0`（豪华床）

### C. 配置 AI NPC

确保你的 AI Character 有：
- ✅ `UtilityAIController`
- ✅ `MetabolismComponent`
- ✅ `SensoryComponent`
- ✅ `PersonalityComponent`

## 🧪 测试流程

### 1. 加速测试（可选）
在 AI 的 `MetabolismComponent` 中：
- 设置 `HungerRate = 0.5`（快速饥饿）
- 设置 `EnergyRate = 0.3`（快速疲劳）

### 2. 观察日志
打开 Output Log，筛选：
- `Metabolism`
- `UtilityAI`
- `Action_Eat` / `Action_Sleep`

### 3. 预期行为

**阶段 1: 饥饿增长**
```
[Metabolism] Hunger: 0.15...
[Metabolism] Hunger: 0.52...
[Metabolism] Hunger: 0.78...
```

**阶段 2: 选择吃饭动作**
```
[UtilityAI] Evaluating actions...
[UtilityAI] Action_Eat score: 2.34
[UtilityAI] Best Action: Action_Eat
```

**阶段 3: 移动到食物**
```
[Action_Eat] Moving to Smart Object: BP_Apple_C_1
```

**阶段 4: 吃饭恢复**
```
[Action_Eat] Eating... Hunger: 0.75 (Rate: 0.30/s)
[Action_Eat] Eating... Hunger: 0.72 (Rate: 0.30/s)
[Action_Eat] Eating... Hunger: 0.68 (Rate: 0.30/s)
...
[Action_Eat] Eating... Hunger: 0.12 (Rate: 0.30/s)
```

**阶段 5: 返回 Idle**
```
[UtilityAI] Best Action: Test_Idle
```

## 🔍 故障排查

### 问题 1: AI 不去吃饭
- 检查 `HasFoodNearby` 是否返回 1.0
- 检查 `Action_Eat` 的 BaseReward 是否足够高
- 检查 Hunger 值是否真的在增长

### 问题 2: AI 到达食物但不恢复
- 检查 SmartObject 是否有 `SmartObjectComponent`
- 检查 `SocialTag` 是否设置为 `Activity.Eat`
- 检查日志中是否有 "Eating..." 输出

### 问题 3: 恢复速度不对
- 检查 SmartObject 的 `RestoreValue` 设置
- 日志会显示 `(Rate: X.XX/s)`，确认是否符合预期

## 📊 推荐配置值

| 对象类型 | RestoreValue | 说明 |
|---------|-------------|------|
| 苹果/面包 | 0.1 - 0.2 | 慢速恢复，需要多吃几次 |
| 普通餐食 | 0.3 - 0.5 | 中速恢复 |
| 大餐/盛宴 | 0.8 - 1.0 | 快速恢复，几秒就饱 |
| 简易床铺 | 0.2 - 0.3 | 慢速恢复能量 |
| 普通床 | 0.5 | 标准恢复 |
| 豪华床 | 1.0 | 瞬间满血复活 |

## 🎯 下一步优化建议

1. **添加动画**: 在 `RestoreStats` 中触发吃饭/睡觉动画
2. **添加音效**: 播放咀嚼/打鼾声音
3. **添加 UI**: 显示 Hunger/Energy 进度条
4. **多样化需求**: 添加 Thirst（口渴）、Social（社交）等需求
