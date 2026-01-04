# TestCheatManager 使用指南

## 📋 概述

`TestCheatManager` 是一个用于调试和测试的作弊管理器，提供了各种便捷的控制台命令来操作 NPC。

## 🎮 如何启用

### 方法 1: 在 PlayerController 中设置

在你的 PlayerController 类中添加：

```cpp
// 在构造函数中
AYourPlayerController::AYourPlayerController()
{
    CheatClass = UTestCheatManager::StaticClass();
}
```

### 方法 2: 在 Blueprint 中设置

1. 打开你的 PlayerController Blueprint
2. 在 Class Defaults 中找到 `Cheat Class`
3. 设置为 `TestCheatManager`

## 📝 可用命令

### NPC 伤害相关

#### `DamageAllNPCs [Damage]`
给所有 NPC 造成指定伤害

**示例：**
```
DamageAllNPCs 10      // 给所有 NPC 造成 10 点伤害
DamageAllNPCs 50      // 给所有 NPC 造成 50 点伤害
DamageAllNPCs         // 默认造成 10 点伤害
```

**效果：**
- ✅ 触发 `OnTakeAnyDamage` 事件
- ✅ 触发 `SensoryComponent::ReportDamage`
- ✅ 触发 LLM 情绪处理
- ✅ 可能触发逃跑行为（如果 Fear 足够高）

---

#### `DamageNPC [NPCName] [Damage]`
给指定名称的 NPC 造成伤害（支持部分匹配）

**示例：**
```
DamageNPC Enemy 20         // 给名称包含 "Enemy" 的 NPC 造成 20 点伤害
DamageNPC CombatEnemy 15   // 给名称包含 "CombatEnemy" 的 NPC 造成 15 点伤害
```

---

#### `KillAllNPCs`
立即杀死所有 NPC

**示例：**
```
KillAllNPCs
```

**效果：**
- 造成 9999 点伤害
- 触发死亡动画和逻辑

---

#### `HealAllNPCs`
治疗所有 NPC 到满血

**示例：**
```
HealAllNPCs
```

---

### NPC 信息相关

#### `ListAllNPCs`
列出场景中所有 NPC 的信息

**示例：**
```
ListAllNPCs
```

**输出示例：**
```
═══════════════════════════════════════
[CheatManager] Listing all NPCs (3 found)
═══════════════════════════════════════
[1] CombatEnemy_2
    Location: X=1250.0 Y=-340.0 Z=88.0
    Controller: UtilityAIController_3
[2] CombatEnemy_5
    Location: X=890.0 Y=120.0 Z=88.0
    Controller: UtilityAIController_6
[3] CombatEnemy_8
    Location: X=-450.0 Y=670.0 Z=88.0
    Controller: UtilityAIController_9
═══════════════════════════════════════
```

---

### LLM 测试相关

#### `TestLLMConnection`
测试 LLM 连接和情绪系统

**示例：**
```
TestLLMConnection
```

**效果：**
- 查找第一个 AI Controller
- 触发测试伤害（25 点）
- 验证 LLM 是否正常响应

---

### 情绪触发（待实现）

#### `TriggerEmotion [NPCName] [EmotionType]`
直接触发指定 NPC 的情绪（功能待实现）

**示例：**
```
TriggerEmotion Enemy fear     // 触发恐惧
TriggerEmotion Enemy anger    // 触发愤怒
TriggerEmotion Enemy joy      // 触发喜悦
```

---

## 🧪 测试场景示例

### 场景 1: 测试逃跑行为

```
1. ListAllNPCs              // 查看有哪些 NPC
2. DamageAllNPCs 30         // 造成 30 点伤害
3. 观察 NPC 是否开始逃跑
```

### 场景 2: 测试伤害事件传递

```
1. DamageNPC Enemy 20       // 给特定 NPC 造成伤害
2. 检查日志，确认以下输出：
   - [Sensory] 💥 ReportDamage Triggered!
   - [Cognition] Processing stimulus
   - [LLM] Sending request
```

### 场景 3: 测试 LLM 连接

```
1. TestLLMConnection        // 触发 LLM 测试
2. 检查日志中的 LLM 响应
```

### 场景 4: 压力测试

```
1. DamageAllNPCs 5          // 小伤害
2. 等待 1 秒
3. DamageAllNPCs 10         // 中等伤害
4. 等待 1 秒
5. DamageAllNPCs 20         // 大伤害
6. 观察 NPC 情绪和行为变化
```

---

## 🔍 调试技巧

### 1. 查看详细日志

在控制台输入：
```
Log LogTemp All
```

### 2. 过滤特定日志

```
Log LogTemp Warning      // 只显示警告和错误
Log LogTemp Error        // 只显示错误
```

### 3. 清空日志

```
cls
```

---

## ⚠️ 注意事项

1. **CheatManager 只在 PIE/Debug 模式下工作**
   - Shipping 构建中会被自动禁用

2. **需要启用控制台**
   - 按 `~` 或 `` ` `` 键打开控制台
   - 如果无法打开，检查项目设置中的控制台键绑定

3. **命令区分大小写**
   - 使用 `DamageAllNPCs` 而不是 `damageallnpcs`

4. **参数是可选的**
   - 大多数命令都有默认值
   - 可以省略参数使用默认值

---

## 🚀 扩展功能建议

你可以添加更多有用的命令：

```cpp
// 传送 NPC
UFUNCTION(Exec)
void TeleportNPC(const FString& NPCName, float X, float Y, float Z);

// 冻结所有 NPC
UFUNCTION(Exec)
void FreezeAllNPCs();

// 设置 NPC 移动速度
UFUNCTION(Exec)
void SetNPCSpeed(const FString& NPCName, float Speed);

// 显示 NPC 的 Mental State
UFUNCTION(Exec)
void ShowMentalState(const FString& NPCName);
```

---

## 📊 日志输出说明

成功执行命令后，你会看到类似的日志：

```
═══════════════════════════════════════
[CheatManager] Damaging all NPCs with 10.0 damage
[CheatManager] Found 3 NPCs
[CheatManager] ✅ Damaged CombatEnemy_2 (10.0 HP)
[CheatManager] ✅ Damaged CombatEnemy_5 (10.0 HP)
[CheatManager] ✅ Damaged CombatEnemy_8 (10.0 HP)
[CheatManager] Successfully damaged 3 NPCs
═══════════════════════════════════════
```

---

## 🎯 快速参考

| 命令 | 作用 | 默认参数 |
|------|------|----------|
| `DamageAllNPCs [Damage]` | 伤害所有 NPC | 10.0 |
| `DamageNPC [Name] [Damage]` | 伤害指定 NPC | 10.0 |
| `KillAllNPCs` | 杀死所有 NPC | - |
| `HealAllNPCs` | 治疗所有 NPC | - |
| `ListAllNPCs` | 列出所有 NPC | - |
| `TestLLMConnection` | 测试 LLM | - |

---

**祝测试愉快！** 🎮
