# Action_SmartObject 动画支持文档

## ✅ 新增功能

### 1. **动画播放支持**
- 可配置 AnimMontage 在执行动作时播放
- 支持循环播放（Loop）
- 自动在动作结束时停止动画

### 2. **持续时长控制**
- 可设置动作的持续时间
- 0 = 无限持续（由 Utility AI 决定何时切换）
- > 0 = 固定时长后自动结束

## 📋 配置字段

### 在 DataTable (`DT_UtilityActions`) 中配置

#### Action_Eat 示例
```
Row Name: Action_Eat
ActionClass: UAction_SmartObject
SmartObjectTag: Activity.Eat
BaseReward: 5.0

=== Animation Configuration ===
InteractionMontage: AM_Eating  // 你的吃饭动画 Montage
bLoopAnimation: true           // 循环播放
ActionDuration: 5.0            // 吃 5 秒钟

=== Considerations ===
[0]:
  InputType: Hunger
  ConsiderationType: Motivation
  CurveType: Logistic
[1]:
  InputType: HasFoodNearby
  ConsiderationType: Context
  CurveType: TargetThreshold
```

#### Action_Sleep 示例
```
Row Name: Action_Sleep
ActionClass: UAction_SmartObject
SmartObjectTag: Activity.Rest
BaseReward: 5.0

=== Animation Configuration ===
InteractionMontage: AM_Sleeping  // 你的睡觉动画 Montage
bLoopAnimation: true             // 循环播放
ActionDuration: 10.0             // 睡 10 秒钟

=== Considerations ===
[0]:
  InputType: Energy
  ConsiderationType: Motivation
  CurveType: Logistic
[1]:
  InputType: HasBedNearby
  ConsiderationType: Context
  CurveType: TargetThreshold
```

## 🎬 动画 Montage 设置

### 创建吃饭动画 Montage

1. **打开动画序列**（例如 `Anim_Eating`）
2. **右键 → Create → Create AnimMontage**
3. **命名**: `AM_Eating`
4. **设置循环**:
   - 在 Montage 编辑器中，确保有一个 `Default` Section
   - 如果需要循环，Section 应该能无缝衔接

### 创建睡觉动画 Montage

1. **打开动画序列**（例如 `Anim_Sleeping`）
2. **右键 → Create → Create AnimMontage**
3. **命名**: `AM_Sleeping`
4. **设置循环**: 同上

## 🔄 工作流程

### 阶段 1: 选择动作
```
[UtilityAI] Hunger: 0.8
[UtilityAI] Best Action: Action_Eat (Score: 4.0)
```

### 阶段 2: 移动到对象
```
[Action_Eat] Moving to Smart Object: BP_Apple_C_1
```

### 阶段 3: 到达并播放动画
```
[Action_Eat] Playing animation: AM_Eating (Loop: Yes)
```

### 阶段 4: 执行交互（恢复 Hunger）
```
[Action_Eat] Eating... Hunger: 0.75 (Rate: 0.30/s)
[Action_Eat] Eating... Hunger: 0.70 (Rate: 0.30/s)
...
```

### 阶段 5: 持续时长到期
```
[Action_Eat] Duration expired (5.0s), action should end soon
```

### 阶段 6: 停止动画并退出
```
[Action_Eat] Stopped animation: AM_Eating
[UtilityAI] ✅ Switch Action: Test_Idle (Score: 1.2)
```

## ⚙️ 高级配置

### 无限持续动作
如果你希望动作一直持续到 Utility AI 自然切换（例如 Hunger 降到很低），设置：
```
ActionDuration: 0.0
```

这样 NPC 会一直吃，直到不饿了。

### 固定时长动作
如果你希望 NPC 吃固定时间（例如 5 秒），无论是否吃饱：
```
ActionDuration: 5.0
```

5 秒后，即使还饿，也会停止吃饭（然后 Utility AI 可能会重新选择吃饭）。

### 不循环动画
如果动画本身就很长（例如 10 秒的完整吃饭动画），不需要循环：
```
bLoopAnimation: false
ActionDuration: 10.0  // 与动画长度匹配
```

## 🎯 推荐配置

### 吃饭动作
```
InteractionMontage: AM_Eating
bLoopAnimation: true
ActionDuration: 0.0  // 无限，直到吃饱
```

**效果**: NPC 会循环播放吃饭动画，直到 Hunger 降低到不再需要吃饭。

### 睡觉动作
```
InteractionMontage: AM_Sleeping
bLoopAnimation: true
ActionDuration: 0.0  // 无限，直到恢复精力
```

**效果**: NPC 会循环播放睡觉动画，直到 Energy 降低到不再疲劳。

### 喝水动作（快速）
```
InteractionMontage: AM_Drinking
bLoopAnimation: false
ActionDuration: 2.0  // 喝水很快
```

**效果**: NPC 播放一次喝水动画（2 秒），然后结束。

## 🔍 调试

### 检查动画是否播放
在 Output Log 中搜索：
```
Playing animation: AM_Eating
```

### 检查动画是否停止
```
Stopped animation: AM_Eating
```

### 检查持续时长
```
Duration expired (5.0s)
```

## 📝 注意事项

1. **AnimMontage 必须存在**: 如果 `InteractionMontage` 为 `nullptr`，不会播放动画，但动作仍会执行。

2. **循环设置**: `bLoopAnimation = true` 会调用 `Montage_SetNextSection`，确保你的 Montage 有正确的 Section 设置。

3. **持续时长 vs Utility AI**: 
   - `ActionDuration = 0`: Utility AI 完全控制何时切换
   - `ActionDuration > 0`: 强制在指定时间后结束（但 Utility AI 仍可能提前切换）

4. **动画混合**: 如果你的 AnimBP 有复杂的混合逻辑，确保 Montage 的优先级足够高。

## 🎉 总结

现在 `Action_SmartObject` 支持：
- ✅ 播放自定义动画
- ✅ 循环播放控制
- ✅ 持续时长控制
- ✅ 自动停止动画

这让吃饭、睡觉等动作更加真实和可配置！
