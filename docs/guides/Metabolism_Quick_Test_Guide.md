# 🚀 Metabolism 快速测试指南

## ✅ 最新修改（为测试优化）

### 1. **加速新陈代谢速率**
- **HungerRate**: `0.0005` → `0.01` (20倍加速)
- **EnergyRate**: `0.0003` → `0.008` (26倍加速)
- **效果**: NPC 约 **1-2 分钟**就会饿/累，而不是 30+ 分钟

### 2. **添加调试日志**
- **MetabolismComponent**: 每 5 秒打印 Hunger/Energy 值
- **UtilityAIComponent**: 显示所有 Action 的评分（当收到刺激时）

## 📋 测试步骤

### 步骤 1: 编译项目
在 Unreal Editor 中点击 **Compile** 或 **Hot Reload**

### 步骤 2: 配置 DataTable
打开 `DT_UtilityActions`，添加新行：

**Row Name**: `Action_Eat`
```
ActionClass: UAction_SmartObject
SmartObjectTag: Activity.Eat
BaseReward: 5.0  // 提高到 5.0 让吃饭更有吸引力
Considerations:
  [0]:
    InputType: Hunger
    ConsiderationType: Motivation
    CurveType: Logistic
    Weight: 1.0
  [1]:
    InputType: HasFoodNearby
    ConsiderationType: Context
    CurveType: TargetThreshold
```

### 步骤 3: 创建食物对象
1. 创建一个新的 Blueprint Actor（例如 `BP_Apple`）
2. 添加 `SmartObjectComponent`
3. 配置：
   - `Semantics.SocialTag`: 选择 `Activity.Eat`
   - `RestoreValue`: `0.3`（每秒恢复 30%）

### 步骤 4: 放置测试场景
1. 放置一个 Human NPC（PersonalityID = "FriendlyMerchant" 或其他非 Zombie）
2. 在 NPC 附近放置 `BP_Apple`
3. 确保距离在 1500cm 以内（SensoryComponent 的视野范围）

### 步骤 5: 开始测试
1. **Play**
2. **打开 Output Log**
3. **筛选关键词**: `Metabolism`、`UtilityAI`、`Action_Eat`

## 📊 预期日志输出

### 阶段 1: Metabolism 启动（立即）
```
[UtilityAIController_1] Metabolism ENABLED (Faction: Human)
```

### 阶段 2: Hunger 增长（每 5 秒）
```
[Metabolism] UtilityAIController_1 - Hunger: 0.050, Energy: 0.040
[Metabolism] UtilityAIController_1 - Hunger: 0.100, Energy: 0.080
[Metabolism] UtilityAIController_1 - Hunger: 0.150, Energy: 0.120
...
[Metabolism] UtilityAIController_1 - Hunger: 0.550, Energy: 0.440
```

### 阶段 3: UtilityAI 选择 Action_Eat（当 Hunger > 0.5）
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[UtilityAI|FriendlyMerchant] Evaluating Actions (Count: 4) [Triggered]
[UtilityAI|FriendlyMerchant] Currently Running: Test_Idle
[UtilityAI|FriendlyMerchant] 🏆 Best Action: Action_Eat (Score: 2.750)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[UtilityAI|FriendlyMerchant] ✅ Switch Action: Action_Eat (Score: 2.75)
```

### 阶段 4: 移动到食物
```
[Action_Eat] Moving to Smart Object: BP_Apple_C_1
```

### 阶段 5: 吃饭恢复
```
[Action_Eat] Eating... Hunger: 0.520 (Rate: 0.30/s)
[Action_Eat] Eating... Hunger: 0.490 (Rate: 0.30/s)
[Action_Eat] Eating... Hunger: 0.460 (Rate: 0.30/s)
...
[Action_Eat] Eating... Hunger: 0.120 (Rate: 0.30/s)
```

### 阶段 6: 返回 Idle
```
[UtilityAI|FriendlyMerchant] ✅ Switch Action: Test_Idle (Score: 1.20)
```

## 🔍 故障排查

### ❌ 问题 1: 没有看到 "Metabolism ENABLED"
**原因**: 
- NPC 的 Faction 是 Monster
- PersonalityComponent 没有正确初始化

**解决**:
- 检查 NPC 的 PersonalityID 是否设置
- 确保不是 "Zombie"（Zombie 是 Monster 阵营）

### ❌ 问题 2: Hunger 不增长
**原因**:
- MetabolismComponent 被禁用
- HungerRate = 0

**解决**:
- 检查日志中是否有 "Metabolism ENABLED"
- 在 NPC Details 中检查 MetabolismComponent 的 HungerRate

### ❌ 问题 3: 没有看到 "Action_Eat"
**原因**:
- DataTable 中没有配置 Action_Eat
- ActionClass 设置错误

**解决**:
- 确认 DT_UtilityActions 中有 Action_Eat 行
- ActionClass 必须是 `UAction_SmartObject`

### ❌ 问题 4: NPC 不移动到食物
**原因**:
- HasFoodNearby 返回 0（找不到食物）
- SmartObject 的 Tag 不对

**解决**:
- 确保食物在 1500cm 范围内
- 检查 SmartObjectComponent 的 SocialTag 是否为 `Activity.Eat`
- 添加临时日志验证（见下方）

### ❌ 问题 5: 到了食物但不恢复
**原因**:
- RestoreValue = 0
- SmartObjectComponent 没有正确配置

**解决**:
- 设置 RestoreValue > 0
- 检查日志是否有 "Eating... Hunger: X.XX"

## 🔧 临时调试代码

如果还是有问题，在 `UtilityActionBase.cpp` 的 `GetConsiderationValue` 中添加：

```cpp
case EUtilityInputType::HasFoodNearby:
{
    if (USensoryComponent* Sensory = Controller->FindComponentByClass<USensoryComponent>())
    {
        AActor* Food = Sensory->FindBestSmartObject(FGameplayTag::RequestGameplayTag("Activity.Eat"));
        UE_LOG(LogTemp, Warning, TEXT("[DEBUG] HasFoodNearby check: %s"), 
               Food ? *FString::Printf(TEXT("FOUND: %s"), *Food->GetName()) : TEXT("NOT FOUND"));
        return Food ? 1.0f : 0.0f;
    }
    return 0.0f;
}
```

## ⏱️ 时间线参考

使用新的加速速率：
- **0-30 秒**: Hunger 从 0 → 0.3（不饿）
- **30-60 秒**: Hunger 从 0.3 → 0.6（开始饿）
- **60 秒**: Hunger = 0.6，**AI 应该选择 Action_Eat**
- **60-90 秒**: NPC 移动到食物并开始吃
- **90-120 秒**: Hunger 从 0.6 → 0.1（吃饱了）
- **120 秒+**: 返回 Idle，循环

## 🎯 成功标志

如果你看到以下日志序列，说明系统完全正常：

1. ✅ `Metabolism ENABLED`
2. ✅ `Hunger: 0.XXX` 逐渐增长
3. ✅ `Best Action: Action_Eat`
4. ✅ `Moving to Smart Object`
5. ✅ `Eating... Hunger: 0.XXX` 逐渐降低
6. ✅ `Switch Action: Test_Idle`

恭喜！Metabolism 系统完全打通！🎉
