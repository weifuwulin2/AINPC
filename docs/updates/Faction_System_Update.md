# 阵营系统更新总结 (Faction System Update Summary)

## 更新日期 (Update Date)
2026-01-09

## 问题 (Problem)
用户反馈：两个僵尸都有 "Enemy" 标签时会互相攻击，需要一个更细致的方法来区分敌我关系。

## 解决方案 (Solution)
实现了基于阵营（Faction）的敌对判断系统，替代简单的标签检查。

---

## 主要变更 (Main Changes)

### 1. PersonalityConfig 添加 Faction 字段
**文件**: `Source/AINPC/Public/UtilityAI/PersonalityConfig.h`

```cpp
// 阵营 / Faction
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role Definition")
FName Faction = "Neutral";
```

**用途**:
- 定义 NPC 的阵营（如 "Human", "Zombie", "Monster", "Neutral"）
- 同阵营的 NPC 不会互相触发即时响应
- 不同阵营的 NPC 会触发即时响应

---

### 2. SensoryComponent 添加阵营检测函数
**文件**: `Source/AINPC/Components/SensoryComponent.h` 和 `.cpp`

**新增函数**:
```cpp
// 获取 Actor 的阵营
FName GetActorFaction(AActor* Actor) const;

// 检查两个 Actor 是否属于敌对阵营
bool AreActorsHostile(AActor* ActorA, AActor* ActorB) const;
```

**敌对规则**:
1. 中立阵营（"Neutral"）永远不敌对
2. 相同阵营不敌对
3. 不同的非中立阵营互相敌对

---

### 3. 更新即时响应逻辑
**文件**: `Source/AINPC/Components/SensoryComponent.cpp`

**变更前**:
```cpp
bool bIsHighPriority = Event.Target->ActorHasTag("Player") || Event.Target->ActorHasTag("Enemy");
```

**变更后**:
```cpp
bool bIsHostile = AreActorsHostile(GetOwner(), Event.Target);
bool bIsPlayer = Event.Target->ActorHasTag("Player");
bool bIsHighPriority = bIsHostile || (bIsPlayer && AreActorsHostile(GetOwner(), Event.Target));
```

**效果**:
- 只有敌对阵营才会触发即时响应
- 同阵营的 NPC（如两个僵尸）不会互相触发即时攻击

---

## 使用示例 (Usage Example)

### DataTable 配置示例

#### 僵尸配置
```
PersonalityID: "Zombie"
Faction: "Zombie"
RoleDescription: "You are a hostile zombie that attacks humans"
BehavioralGuidelines: "Attack humans on sight. Ignore other zombies."
```

#### 人类守卫配置
```
PersonalityID: "Guard"
Faction: "Human"
RoleDescription: "You are a human guard protecting the settlement"
BehavioralGuidelines: "Protect humans. Attack zombies and monsters."
```

#### 中立商人配置
```
PersonalityID: "Merchant"
Faction: "Neutral"
RoleDescription: "You are a neutral merchant"
BehavioralGuidelines: "Trade with anyone. Don't engage in combat."
```

---

## 行为对比 (Behavior Comparison)

### 场景 1: 僵尸 A 看到僵尸 B
- **变更前**: 立即触发即时响应（因为都有 "Enemy" 标签）
- **变更后**: 不触发即时响应（同阵营），需要累积 3 次才触发认知事件

### 场景 2: 僵尸看到人类
- **变更前**: 立即触发即时响应（人类有 "Player" 或 "Enemy" 标签）
- **变更后**: 立即触发即时响应（不同阵营，敌对）

### 场景 3: 僵尸看到中立商人
- **变更前**: 可能触发即时响应（取决于标签）
- **变更后**: 不触发即时响应（中立阵营），需要累积才触发

### 场景 4: 人类守卫看到僵尸
- **变更前**: 立即触发即时响应
- **变更后**: 立即触发即时响应（不同阵营，敌对）

---

## 日志输出示例 (Log Output Example)

### 敌对阵营触发即时响应
```
[Sensory] Hostile Target Detected: BP_PlayerCharacter (Self: Zombie, Target: Human, Magnitude: 0.50)
```

### 同阵营进入累积系统
```
[Sensory] Visual Accumulation for BP_Zombie2: 1/3
[Sensory] Visual Accumulation for BP_Zombie2: 2/3
[Sensory] Visual Accumulation Threshold Reached for BP_Zombie2 (Count: 3)
```

---

## 配置参数 (Configuration Parameters)

### SensoryComponent 参数
| 参数 | 默认值 | 说明 |
|------|--------|------|
| `bEnableImmediateResponse` | `true` | 是否启用即时响应系统 |
| `HighPriorityMagnitude` | `0.5` | 高优先级目标的感知重要性 |
| `AccumulationThreshold` | `3` | 低优先级目标的累积阈值 |
| `PerceptionCooldown` | `5.0` | 感知冷却时间（秒） |

---

## 优势 (Advantages)

1. ✅ **避免同阵营互相攻击**: 僵尸不会攻击僵尸，人类不会攻击人类
2. ✅ **灵活的阵营配置**: 通过 DataTable 轻松配置不同的阵营关系
3. ✅ **支持中立阵营**: 商人、NPC 等可以设置为中立，不会被即时敌对
4. ✅ **统一管理**: 阵营配置集成在 PersonalityConfig 中，便于管理
5. ✅ **向后兼容**: 仍然支持 "Player" 和 "Enemy" 标签作为备用方案
6. ✅ **清晰的日志**: 日志输出包含阵营信息，便于调试

---

## 迁移指南 (Migration Guide)

### 步骤 1: 更新 DataTable
打开你的 PersonalityConfig DataTable，为每个配置添加 `Faction` 字段。

### 步骤 2: 设置阵营
根据 NPC 的角色设置合适的阵营：
- 僵尸、怪物 → `"Zombie"` 或 `"Monster"`
- 人类、友军 → `"Human"`
- 商人、中立 NPC → `"Neutral"`

### 步骤 3: 移除不必要的 "Enemy" 标签
如果你的 NPC 已经通过阵营系统管理，可以移除 "Enemy" 标签（但保留 "Player" 标签给玩家角色）。

### 步骤 4: 测试
在游戏中测试，确保：
- 同阵营 NPC 不会互相攻击
- 不同阵营 NPC 会正确触发即时响应
- 中立 NPC 不会被即时敌对

---

## 相关文件 (Related Files)

- `Source/AINPC/Public/UtilityAI/PersonalityConfig.h` - 添加了 Faction 字段
- `Source/AINPC/Components/SensoryComponent.h` - 添加了阵营检测函数声明
- `Source/AINPC/Components/SensoryComponent.cpp` - 实现了阵营检测和更新了即时响应逻辑
- `docs/features/Immediate_Response_System.md` - 更新了文档，添加了阵营系统说明

---

## 下一步建议 (Next Steps)

1. **扩展阵营系统**: 可以考虑添加更复杂的阵营关系（如联盟、敌对等级）
2. **动态阵营切换**: 实现运行时改变阵营的功能（如 NPC 被感染变成僵尸）
3. **阵营声望系统**: 为每个阵营添加声望值，影响 NPC 的反应
4. **可视化调试**: 在编辑器中显示阵营关系图

---

**版本 (Version)**: 1.0  
**作者 (Author)**: Antigravity AI Assistant  
**最后更新 (Last Updated)**: 2026-01-09
