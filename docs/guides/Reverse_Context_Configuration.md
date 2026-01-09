# 反向 Context 配置指南
# Reverse Context Configuration Guide

**日期 / Date:** 2026-01-09  
**用途 / Purpose:** 配置"当条件为真时禁止动作"的逻辑

---

## 问题场景 / Problem Scenario

**需求：**
- Talk 动作应该在**没有敌人**时才能执行
- 如果周围有敌人（`HasEnemyNearby = 1.0`），Talk 的分数应该变为 **0**

**传统 Context 的问题：**
- Context 使用乘法：`Score = BaseReward × Motivations × Contexts`
- 如果直接用 `HasEnemyNearby` 作为 Context：
  - 有敌人时：`Context = 1.0` → 分数正常
  - 没敌人时：`Context = 0.0` → 分数为 0
- **这是反的！**

---

## 解决方案：Inverse 曲线 / Solution: Inverse Curve

### 原理 / Principle

使用 **Inverse** 曲线类型，将输入值反转：
```
y = 1 - x

输入 (HasEnemyNearby) → 输出 (Context Value)
0.0 (没敌人)           → 1.0 (允许)
1.0 (有敌人)           → 0.0 (禁止)
```

### 配置步骤 / Configuration Steps

#### 1. 打开 Talk Action 的 DataTable 配置

在 `UtilityActionConfig` DataTable 中找到 **Talk** 行。

#### 2. 添加 Context Consideration

在 `Considerations` 数组中添加一个新元素：

| 字段 | 值 | 说明 |
|------|-----|------|
| **ConsiderationType** | `Context` | 必要条件（乘法） |
| **InputType** | `HasEnemyNearby` | 检查附近是否有敌人 |
| **CurveType** | `Inverse` | 反向曲线 |

#### 3. 保存并测试

现在 Talk 动作的计算逻辑：
```
Score = BaseReward × Motivations × (1 - HasEnemyNearby)

情况 A：没有敌人
  HasEnemyNearby = 0.0
  Context = 1 - 0.0 = 1.0
  Score = 1.0 × Motivations × 1.0 = Motivations ✅

情况 B：有敌人
  HasEnemyNearby = 1.0
  Context = 1 - 1.0 = 0.0
  Score = 1.0 × Motivations × 0.0 = 0.0 ✅ (被禁止)
```

---

## 其他反向 Context 示例 / Other Reverse Context Examples

### 示例 1：只在安全时休息
**需求：** Rest 动作只在威胁低时执行

```
Consideration:
  - ConsiderationType: Context
  - InputType: PerceivedThreat
  - CurveType: Inverse

结果：
  威胁高 (0.8) → Context = 0.2 → 分数很低
  威胁低 (0.1) → Context = 0.9 → 分数正常
```

### 示例 2：只在有友军时社交
**需求：** Greet 动作只在有友军时执行

```
Consideration:
  - ConsiderationType: Context
  - InputType: HasFriendlyNearby
  - CurveType: Linear (不需要反向)

结果：
  有友军 (1.0) → Context = 1.0 → 允许
  没友军 (0.0) → Context = 0.0 → 禁止
```

---

## 可用的曲线类型 / Available Curve Types

| 曲线类型 | 公式 | 用途 |
|---------|------|------|
| **Linear** | y = x | 直接使用原始值 |
| **Inverse** | y = 1 - x | **反向 Context**（本文重点） |
| **Quadratic** | y = x² | 只有很高时才有用 |
| **InverseQuadratic** | y = 1 - (1-x)² | 一点点就很有用 |
| **Step** | y = x > 0.5 ? 1 : 0 | 二元判断 |
| **TargetThreshold** | y = x > 0.1 ? 1 : 0 | 自定义阈值 |

---

## 调试技巧 / Debugging Tips

### 1. 启用日志
在收到刺激后，Utility AI 会自动打印详细日志：
```
[Context 0] HasEnemyNearby: (Inverse: 1.00->0.00) 1.000 × 0.000 = 0.000
⚠️ Context became 0! Action is impossible.
```

### 2. 检查 Context 值
如果 Talk 依然在有敌人时执行，检查：
- ✅ `HasEnemyNearby` 是否正确返回 1.0？
- ✅ Consideration 的 `ConsiderationType` 是否设为 `Context`？
- ✅ `CurveType` 是否设为 `Inverse`？

---

## 总结 / Summary

**反向 Context 的核心思路：**
1. 使用 `Context` 类型（乘法）
2. 选择 `Inverse` 曲线
3. 当条件为真（1.0）时，输出为 0.0，导致整个分数为 0

这样就实现了"当 X 为真时禁止动作"的逻辑，而不需要修改代码！
