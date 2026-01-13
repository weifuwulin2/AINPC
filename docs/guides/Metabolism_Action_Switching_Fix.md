# 🔧 Metabolism 动作切换问题修复

## 🐛 观察到的问题

NPC 在吃饭和 Idle 之间快速切换：
```
[Action_Eat] Eating... Hunger: 0.04 (只吃一口)
[Action_Eat] Stopped animation
✅ Switch Action: Test_Idle
✅ Switch Action: Action_Eat (又回来吃)
```

## 📊 原因分析

### 1. Hunger 降低导致得分下降
- NPC 吃了一口，Hunger 从 0.06 → 0.04
- Action_Eat 的得分 = `BaseReward(2.0) × Hunger(0.04) = 0.08`
- Test_Idle 的得分 = `0.30`（有 Inertia Bonus）
- **Idle 得分更高，切换！**

### 2. 下一帧 Hunger 又增长
- MetabolismComponent 每帧增加 Hunger
- Hunger 从 0.04 → 0.05
- Action_Eat 得分又变高了
- **切换回 Eating！**

### 3. 循环往复
- 形成了"吃一口 → Idle → 吃一口"的循环

## ✅ 解决方案

### 方案 A: 增加 Eating 的惯性（推荐）

**在 DT_UtilityActions 中配置 Action_Eat**:
```
Row Name: Action_Eat
BaseReward: 5.0          // 提高基础奖励
InertiaBonus: 2.0        // 正在吃时 +2.0 分，防止被打断
CooldownTime: 0.0
ActionDuration: 0.0      // 无限持续，直到吃饱

Considerations:
  [0]:
    InputType: Hunger
    ConsiderationType: Motivation
    CurveType: Logistic   // S 曲线，中间变化大
  [1]:
    InputType: HasFoodNearby
    ConsiderationType: Context
    CurveType: TargetThreshold
```

**效果**:
- 开始吃时：`5.0 × 0.6 = 3.0`
- 正在吃时：`5.0 × 0.5 + 2.0 = 4.5`（有惯性加成）
- Idle 得分：`0.3`
- **吃饭得分远高于 Idle，不会被打断**

### 方案 B: 降低 Idle 的基础得分

**在 DT_UtilityActions 中配置 Test_Idle**:
```
Row Name: Test_Idle
BaseReward: 0.1          // 降低到 0.1（原来可能是 0.3）
InertiaBonus: 0.05
```

**效果**:
- Idle 得分降低，不容易抢占其他动作

### 方案 C: 使用更平缓的 Hunger 曲线

**修改 Action_Eat 的 Consideration**:
```
Considerations[0]:
  InputType: Hunger
  CurveType: InverseQuadratic  // 即使 Hunger 降低，得分仍然高
```

**效果**:
- Hunger = 0.6 → 得分 = 0.84
- Hunger = 0.4 → 得分 = 0.64
- Hunger = 0.2 → 得分 = 0.36
- **即使吃了一些，得分仍然较高**

### 方案 D: 设置固定持续时间

**在 DT_UtilityActions 中配置 Action_Eat**:
```
ActionDuration: 5.0  // 强制吃 5 秒
```

**效果**:
- NPC 会吃满 5 秒，无论 Hunger 如何变化
- 5 秒后自动结束（或被 Utility AI 切换）

## 🎯 推荐配置（综合方案）

### Action_Eat
```
ActionClass: UAction_SmartObject
SmartObjectTag: Activity.Eat
BaseReward: 5.0          ⬅️ 提高
InertiaBonus: 2.0        ⬅️ 添加惯性
CooldownTime: 0.0
ActionDuration: 0.0      ⬅️ 无限持续

Animation:
  InteractionMontage: AS_EatingOnParty_Montage
  bLoopAnimation: true
  ActionDuration: 0.0

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

### Test_Idle
```
BaseReward: 0.15         ⬅️ 降低（原来可能是 0.3）
InertiaBonus: 0.05
```

## 📊 预期效果

### 修复前
```
Hunger: 0.06 → Action_Eat (Score: 0.12)
吃一口
Hunger: 0.04 → Test_Idle (Score: 0.30) ⚠️ 切换！
Hunger: 0.05 → Action_Eat (Score: 0.10) ⚠️ 又切换！
```

### 修复后
```
Hunger: 0.06 → Action_Eat (Score: 5.0 × 0.06 = 0.30)
开始吃
Hunger: 0.05 → Action_Eat (Score: 5.0 × 0.05 + 2.0 = 2.25) ✅ 继续吃
Hunger: 0.04 → Action_Eat (Score: 5.0 × 0.04 + 2.0 = 2.20) ✅ 继续吃
...
Hunger: 0.01 → Action_Eat (Score: 5.0 × 0.01 + 2.0 = 2.05) ✅ 继续吃
Hunger: 0.00 → Test_Idle (Score: 0.15) ✅ 吃饱了，切换到 Idle
```

## 🧪 测试步骤

1. **修改 DataTable**:
   - Action_Eat: `BaseReward = 5.0`, `InertiaBonus = 2.0`
   - Test_Idle: `BaseReward = 0.15`

2. **重新运行测试**

3. **观察日志**:
   - 应该看到 NPC 持续吃饭，直到 Hunger 接近 0
   - 不再频繁切换

## 💡 额外优化

### 添加"吃饱阈值"

如果你希望 NPC 不用吃到 Hunger = 0 就停止，可以使用 Step 曲线：

```
Considerations[0]:
  InputType: Hunger
  CurveType: Step  // Hunger > 0.5 时得分 1.0，否则 0.0
```

这样 NPC 会吃到 Hunger < 0.5 就停止。

### 添加"吃饭冷却"

如果你希望 NPC 吃完后一段时间内不再吃：

```
Action_Eat:
  CooldownTime: 30.0  // 吃完后 30 秒内不能再吃
```

这样更符合真实行为。
