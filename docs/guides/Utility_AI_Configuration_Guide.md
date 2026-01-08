# 🎮 Utility AI 配置指南

**目标:** 使用简洁的设计创建可预测、易调试的 AI 行为  
**核心理念:** 用 ResponseCurve 控制因子重要性，而不是添加复杂的权重系统

---

## 🎯 设计哲学

### 当前架构
```cpp
最终分数 = Action.BaseWeight × Factor1 × Factor2 × Factor3 × ...
```

### 两个控制点

1. **Action BaseWeight** - 控制动作之间的全局优先级
2. **ResponseCurve** - 控制每个因子的影响方式

**不需要 Consideration Weight！** ✅

---

## 📊 BaseWeight 使用指南

### 作用
控制**不同动作之间**的相对优先级

### 推荐值

| 动作类型 | BaseWeight | 说明 |
|----------|------------|------|
| 核心动作 | 1.5 - 2.0 | AI 的主要行为（如战斗型 AI 的 Attack） |
| 常规动作 | 1.0 | 中性动作 |
| 备选动作 | 0.7 - 0.9 | 不太希望执行的动作 |
| 紧急动作 | 2.0 - 3.0 | 生存相关的动作（如 Flee） |

### 示例配置

#### 战斗型 AI
```
Attack:      BaseWeight = 1.5  (偏好攻击)
Defend:      BaseWeight = 1.0  (中性)
Flee:        BaseWeight = 2.0  (生存优先)
Patrol:      BaseWeight = 0.8  (备选)
```

#### 社交型 NPC
```
Talk:        BaseWeight = 2.0  (主要行为)
Trade:       BaseWeight = 1.5  (次要行为)
Idle:        BaseWeight = 1.0  (中性)
Wander:      BaseWeight = 0.7  (备选)
```

---

## 📈 ResponseCurve 设计指南

### 核心概念
**用曲线形状控制因子的重要性，而不是用权重！**

---

### 曲线类型1: 阈值曲线（必要条件）

**用途:** 某个条件必须满足

**形状:**
```
1.0 |        ╱────────
    |       ╱
0.5 |      ╱
    |     ╱
0.0 |────╱───────────
    0  0.3  0.5    1.0
```

**配置:**
- X=0.0 → Y=0.0
- X=0.3 → Y=0.0
- X=0.5 → Y=1.0
- X=1.0 → Y=1.0

**示例:**
```
Attack 动作的 Anger:
- Anger < 0.3 → 分数 = 0 (完全不攻击)
- Anger > 0.5 → 分数 = 1 (可以攻击)

含义: 必须愤怒才会攻击
```

---

### 曲线类型2: 平缓曲线（次要因子）

**用途:** 有帮助但不是必须

**形状:**
```
1.0 |          ╱──────
    |        ╱
0.5 |      ╱
    |    ╱
0.0 |──╱─────────────
    0  0.3  0.5    1.0
```

**配置:**
- X=0.0 → Y=0.3
- X=0.5 → Y=0.6
- X=1.0 → Y=1.0

**示例:**
```
Attack 动作的 Confidence:
- Confidence = 0.0 → 分数 = 0.3 (还行)
- Confidence = 0.5 → 分数 = 0.6 (不错)
- Confidence = 1.0 → 分数 = 1.0 (很好)

含义: 自信有帮助，但不自信也能攻击
```

---

### 曲线类型3: 反向曲线（负面因子）

**用途:** 值越低越好

**形状:**
```
1.0 |────╲
    |     ╲
0.5 |      ╲
    |       ╲
0.0 |        ╲───────
    0  0.3  0.5    1.0
```

**配置:**
- X=0.0 → Y=1.0
- X=0.3 → Y=0.5
- X=0.5 → Y=0.0
- X=1.0 → Y=0.0

**示例:**
```
Flee 动作的 SelfHealth:
- SelfHealth = 0.1 → 分数 = 1.0 (必须逃跑!)
- SelfHealth = 0.5 → 分数 = 0.0 (不需要逃跑)

含义: 血量越低越想逃跑
```

---

### 曲线类型4: 线性曲线（比例因子）

**用途:** 值越高越好，线性关系

**形状:**
```
1.0 |          ╱
    |        ╱
0.5 |      ╱
    |    ╱
0.0 |  ╱
    0  0.3  0.5    1.0
```

**配置:**
- X=0.0 → Y=0.0
- X=1.0 → Y=1.0

**示例:**
```
Attack 动作的 TargetHealth:
- TargetHealth = 0.2 → 分数 = 0.2
- TargetHealth = 0.8 → 分数 = 0.8

含义: 目标血量越高越想打
```

---

## 🎯 完整配置示例

### Attack 动作

```
BaseWeight: 1.5 (偏好攻击)

Considerations:
┌─────────────────────────────────────────┐
│ 1. Anger (必要条件)                     │
│    - 曲线类型: 阈值曲线                 │
│    - 配置: 0.3以下=0, 0.5以上=1        │
│    - 含义: 必须愤怒才攻击               │
├─────────────────────────────────────────┤
│ 2. Confidence (次要因子)                │
│    - 曲线类型: 平缓曲线                 │
│    - 配置: 0=0.3, 0.5=0.6, 1=1.0       │
│    - 含义: 自信有帮助但不必须           │
├─────────────────────────────────────────┤
│ 3. TargetHealth (比例因子)              │
│    - 曲线类型: 线性曲线                 │
│    - 配置: 0=0, 1=1                     │
│    - 含义: 目标血多更想打               │
└─────────────────────────────────────────┘

计算示例:
- Anger = 0.6 → Curve → 1.0
- Confidence = 0.4 → Curve → 0.5
- TargetHealth = 0.8 → Curve → 0.8
- 最终分数 = 1.5 × 1.0 × 0.5 × 0.8 = 0.6
```

---

### Flee 动作

```
BaseWeight: 2.0 (生存优先)

Considerations:
┌─────────────────────────────────────────┐
│ 1. Fear (必要条件)                      │
│    - 曲线类型: 阈值曲线                 │
│    - 配置: 0.5以下=0, 0.7以上=1        │
│    - 含义: 必须害怕才逃跑               │
├─────────────────────────────────────────┤
│ 2. SelfHealth (反向必要条件)            │
│    - 曲线类型: 反向陡峭曲线             │
│    - 配置: 0.1=1.0, 0.3=0.0            │
│    - 含义: 血量低必须逃跑               │
└─────────────────────────────────────────┘

计算示例:
- Fear = 0.8 → Curve → 1.0
- SelfHealth = 0.15 → Curve → 1.0
- 最终分数 = 2.0 × 1.0 × 1.0 = 2.0
```

---

### Talk 动作

```
BaseWeight: 1.2 (稍微喜欢聊天)

Considerations:
┌─────────────────────────────────────────┐
│ 1. SocialBattery (必要条件)             │
│    - 曲线类型: 阈值曲线                 │
│    - 配置: 0.3以下=0, 0.5以上=1        │
│    - 含义: 社交电量够才聊天             │
├─────────────────────────────────────────┤
│ 2. Boredom (次要因子)                   │
│    - 曲线类型: 平缓曲线                 │
│    - 配置: 0=0.2, 0.5=0.6, 1=1.0       │
│    - 含义: 无聊会增加聊天欲望           │
├─────────────────────────────────────────┤
│ 3. IsTargetPlayer (开关)                │
│    - 曲线类型: 阶跃曲线                 │
│    - 配置: 0=0.3, 1=1.0                │
│    - 含义: 更喜欢和玩家聊天             │
└─────────────────────────────────────────┘
```

---

## 🔍 调试技巧

### 1. 日志输出

在 `CalculateScore()` 中添加日志：

```cpp
UE_LOG(LogTemp, Log, TEXT("[%s] BaseWeight=%.2f"), *ActionName, BaseWeight);

for (const FUtilityConsideration& Factor : Considerations)
{
    float RawValue = GetConsiderationValue(Factor.InputType, ...);
    float FactorScore = Factor.ResponseCurve->GetFloatValue(RawValue);
    
    UE_LOG(LogTemp, Log, TEXT("  - %s: Raw=%.2f, Curve=%.2f"), 
           *UEnum::GetValueAsString(Factor.InputType), RawValue, FactorScore);
}

UE_LOG(LogTemp, Log, TEXT("  → FinalScore=%.2f"), FinalScore);
```

**输出示例:**
```
[Attack] BaseWeight=1.50
  - Anger: Raw=0.60, Curve=1.00
  - Confidence: Raw=0.40, Curve=0.50
  - TargetHealth: Raw=0.80, Curve=0.80
  → FinalScore=0.60

[Flee] BaseWeight=2.00
  - Fear: Raw=0.80, Curve=1.00
  - SelfHealth: Raw=0.15, Curve=1.00
  → FinalScore=2.00

Selected: Flee (2.00 > 0.60)
```

---

### 2. 可视化调试

在编辑器中查看曲线：
1. 打开 DataTable
2. 选择 Action
3. 双击 ResponseCurve
4. 查看曲线形状

---

### 3. 常见问题排查

#### 问题1: AI 总是选择同一个动作

**原因:** BaseWeight 差距太大

**解决:**
```
错误配置:
- Attack: BaseWeight = 5.0
- Flee: BaseWeight = 1.0

正确配置:
- Attack: BaseWeight = 1.5
- Flee: BaseWeight = 2.0
```

---

#### 问题2: AI 行为不符合预期

**原因:** 曲线配置错误

**解决:**
1. 检查曲线形状
2. 添加日志查看实际分数
3. 调整曲线阈值

---

#### 问题3: AI 在危险时不逃跑

**原因:** Flee 的 BaseWeight 太低

**解决:**
```
Flee: BaseWeight = 2.0 (提高优先级)
```

或者调整 SelfHealth 曲线：
```
更陡峭的反向曲线:
- SelfHealth = 0.2 → 分数 = 1.0
- SelfHealth = 0.3 → 分数 = 0.0
```

---

## 📋 配置检查清单

### 添加新动作时

- [ ] 设置合理的 BaseWeight (1.0 - 2.0)
- [ ] 为每个 Consideration 选择合适的曲线类型
- [ ] 测试极端情况（所有值=0, 所有值=1）
- [ ] 添加日志验证分数计算
- [ ] 与其他动作对比，确保优先级合理

---

## 🎯 最佳实践

### 1. 保持简单
- ✅ 每个动作 2-4 个 Considerations
- ❌ 不要超过 5 个

### 2. 明确必要条件
- ✅ 使用阈值曲线表达"必须满足"
- ❌ 不要用平缓曲线表达必要条件

### 3. 合理的 BaseWeight
- ✅ 大部分动作在 0.8 - 1.5 之间
- ✅ 紧急动作可以到 2.0 - 3.0
- ❌ 不要超过 5.0

### 4. 测试驱动
- ✅ 先配置，再测试，再调整
- ✅ 使用日志验证行为
- ❌ 不要盲目调参数

---

## 📚 相关文档

- `UtilityActionBase.h` - 数据结构定义
- `UtilityActionBase.cpp` - 分数计算逻辑
- `MentalStateFields.h` - 可用的输入源

---

## 🎉 总结

### 核心理念
**简单 > 复杂**

### 两个控制点
1. **BaseWeight** - 动作优先级
2. **ResponseCurve** - 因子重要性

### 不需要
- ❌ Consideration Weight
- ❌ 复杂的公式
- ❌ 过多的参数

**用曲线形状控制一切！** 🎯

---

**配置愉快！** 🚀

---

## 🛠️ DataTable Configuration Workflow / 数据表配置流程

To configure the AI system using DataTables, follow these steps:
要使用数据表配置 AI 系统，请遵循以下步骤：

### 1. Create Curve Assets (创建曲线资产)

The Utility AI uses UE's native `CurveFloat` assets.
Utility AI 使用 UE 原生的 `CurveFloat` 资产。

1.  Right-click in Content Browser -> **Miscellaneous** -> **Curve** -> **CurveFloat**.
2.  Name it logically (e.g., `Curve_Linear_0to1`, `Curve_Threshold_0.5`).
3.  Double-click to edit. Add keys (Right-click -> Add Key).
    *   **X-Axis**: Input Value (Normalized 0.0 to 1.0).
    *   **Y-Axis**: Utility Score (Normalized 0.0 to 1.0).
4.  **Important**: Ensure the X range covers 0 to 1.

### 2. Configure Action DataTable (配置动作表)

*   **Table Name**: e.g., `DT_UtilityActions`
*   **Row Structure**: `FUtilityActionConfig`

| Field | Description | Example |
|-------|-------------|---------|
| **ActionClass** | The C++ or BP class of the action | `BP_TestAction_Attack` |
| **BaseReward** | The base score of this action | `1.5` |
| **InertiaBonus** | Bonus points if already executing | `0.1` |
| **CooldownTime** | Time in seconds before re-use | `1.0` |
| **Considerations** | Array of factors | *(See below)* |

**Adding a Consideration:**
1.  Add an element to the `Considerations` array.
2.  **InputType**: Select the attribute (e.g., `Anger`, `DistanceToTarget`).
3.  **ResponseCurve**: Drag & drop your created `CurveFloat` asset here.

### 3. Configure Personality DataTable (配置个性表)

*   **Table Name**: e.g., `DT_Personalities`
*   **Row Structure**: `FPersonalityConfig`

| Field | Description | Example |
|-------|-------------|---------|
| **RoleDescription** | Who the NPC is (System Prompt) | "You are a brave knight..." |
| **BehavioralGuidelines** | How they act (Logic Mapping) | "1. Protect the weak..." |
| **Openness** | Personality Trait (0.0 - 1.0) | `0.8` |
| **Conscientiousness** | Personality Trait (0.0 - 1.0) | `0.3` |
| **Extraversion** | Personality Trait (0.0 - 1.0) | `0.9` |
| **Agreeableness** | Personality Trait (0.0 - 1.0) | `0.1` |
| **Neuroticism** | Personality Trait (0.0 - 1.0) | `0.5` |

### 4. Assigning to NPC (分配给 NPC)

In your NPC Character Blueprint (e.g., `BP_Zombie`):
1.  Select the **PersonalityComponent**.
2.  Find **"Personality Preset"** or **"Personalities Data Table"**.
3.  Select the Row Name (e.g., `Zombie`).
4.  Select the **UtilityAIComponent**.
5.  Find **"Actions Data Table"**.
6.  Select the Actions Table (the component will load ALL actions from this table).

---

