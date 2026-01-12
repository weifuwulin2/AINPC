# 🐛 Faction 未加载问题修复指南

## 问题描述

所有 NPC 的 Faction 都显示为 `Neutral`，即使在 DataTable 中设置了不同的 Faction。

### 症状
```
LogTemp: Warning: [Sensory] Perception: Self=BP_NPCController_Base_C_0 (Faction: EFactionType::Neutral)
LogTemp: Warning: [Sensory] Perception: Self=BP_NPCController_Base_C_2 (Faction: EFactionType::Neutral)
```

即使 Zombie 的 PersonalityID 正确加载，Faction 仍然是 Neutral。

---

## 根本原因

`PersonalityConfig` 结构体中的 `Faction` 字段是最近添加的，但 **DataTable 资产可能还没有这个字段**。

当 DataTable 缺少某个字段时，UE 会使用结构体的默认值：
```cpp
EFactionType Faction = EFactionType::Neutral;  // 默认值
```

---

## 解决步骤

### 步骤 1: 检查 DataTable 是否有 Faction 列

1. 在 Unreal Editor 中打开你的 `DT_Personalities` DataTable
2. 检查是否有 `Faction` 列
3. 如果**没有**，说明 DataTable 需要重新生成

### 步骤 2: 刷新 DataTable 结构

#### 方法 A: 强制刷新（推荐）

1. 关闭 Unreal Editor
2. 删除以下文件夹（如果存在）:
   ```
   AINPC/Intermediate/
   AINPC/Saved/
   ```
3. 重新生成项目文件:
   ```powershell
   # 右键 AINPC.uproject → Generate Visual Studio project files
   ```
4. 重新编译项目
5. 打开 Unreal Editor
6. 打开 `DT_Personalities`，应该会看到新的 `Faction` 列

#### 方法 B: 手动重新创建 DataTable

1. 在 Content Browser 中，右键 `DT_Personalities`
2. 选择 **Duplicate**，命名为 `DT_Personalities_New`
3. 打开 `DT_Personalities_New`
4. 复制所有行的数据
5. 删除旧的 `DT_Personalities`
6. 将 `DT_Personalities_New` 重命名为 `DT_Personalities`

### 步骤 3: 设置正确的 Faction 值

在 DataTable 中，为每个 Personality 设置 `Faction`:

| PersonalityID | Faction | RoleDescription |
|---------------|---------|-----------------|
| `Zombie` | **Monster** | you are a mindless zombie... |
| `BraveWarrior` | **Human** | You are a brave warrior... |
| `FriendlyMerchant` | **Human** 或 **Neutral** | You are a friendly merchant... |
| `CautiousGuard` | **Human** | You are a cautious guard... |

### 步骤 4: 验证修复

1. 运行游戏（PIE）
2. 检查日志，应该看到：
   ```
   LogTemp: Warning: [Sensory] Perception: Self=BP_NPCController_Base_C_2 (Faction: EFactionType::Monster)
   LogTemp: Warning: [Sensory] Perception: Self=BP_NPCController_Base_C_1 (Faction: EFactionType::Human)
   ```

3. Zombie 应该能够攻击 Human，因为：
   ```
   LogTemp: Warning: [Sensory] → Hostile faction detected! Magnitude set to 0.8
   LogTemp: [Sensory] → Set FocusActor to hostile target: BP_NPC_C_UAID_...
   ```

---

## 为什么 Action 分数都是 0？

即使 LLM 返回了 `Anger=1.0`，Utility AI 看到的仍然是 `0.0`，这是因为：

### 问题 1: 插值延迟

```
LogTemp: [MentalStateInterpolator] Set target for Anger: 1.000 (original: 1.000)
```

插值系统需要时间将值从 `0.0` 平滑过渡到 `1.0`。

**解决方案**: Utility AI 应该使用 `GetTargetValue()` 而不是当前插值后的值。

### 问题 2: Attack Action 没有 FocusActor

```
LogTemp: Warning: [Attack] No FocusActor set! Attack will fail.
```

即使 Zombie 想要攻击，但因为所有人都是 `Neutral` 阵营，`SensoryComponent` 没有调用 `SetFocus()`。

**解决方案**: 修复 Faction 后，`AreActorsHostile()` 会返回 `true`，`SetFocus()` 会被调用。

---

## 快速测试脚本

在 Blueprint 或 C++ 中添加调试命令：

```cpp
// 在 UtilityAIController.cpp 的 BeginPlay() 中添加
if (PersonalityComp)
{
    UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
    UE_LOG(LogTemp, Warning, TEXT("[DEBUG] PersonalityID: %s"), *PersonalityComp->PersonalityID.ToString());
    UE_LOG(LogTemp, Warning, TEXT("[DEBUG] Faction: %s"), *UEnum::GetValueAsString(PersonalityComp->Personality.Faction));
    UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
}
```

预期输出：
```
[DEBUG] PersonalityID: Zombie
[DEBUG] Faction: EFactionType::Monster
```

---

## 常见问题

### Q: 我已经设置了 Faction，但还是 Neutral？

**A**: 检查以下几点：
1. DataTable 是否正确保存？（Ctrl+S）
2. Blueprint 中的 `PersonalityTable` 引用是否正确？
3. `PersonalityID` 是否与 DataTable 中的行名完全匹配？（大小写敏感）

### Q: 为什么 Merchant 应该是 Neutral 而不是 Human？

**A**: 取决于你的设计：
- **Human**: 会被 Monster 攻击，会与其他 Human 友好
- **Neutral**: 不会被任何人主动攻击，适合商人、NPC

### Q: 如何让某些 NPC 动态改变 Faction？

**A**: 在运行时修改：
```cpp
PersonalityComp->Personality.Faction = EFactionType::Monster;
PersonalityComp->RecalculateWeights(); // 重新计算权重
```

---

**修复完成后，Zombie 应该能够正确攻击 Human，Attack action 也会有正确的分数！** 🎉
