# 即时响应系统 (Immediate Response System)

## 概述 (Overview)

**中文**: 即时响应系统是 `SensoryComponent` 的一个新特性，用于解决 AI 对高优先级目标（如玩家、敌人）反应延迟的问题。在此系统之前，所有视觉感知都需要累积多次才能触发认知事件，导致僵尸看到玩家后不会立即攻击。

**English**: The Immediate Response System is a new feature of `SensoryComponent` designed to solve the issue of AI reaction delay to high-priority targets (such as players and enemies). Before this system, all visual perceptions required multiple accumulations to trigger a cognitive event, causing zombies to not attack immediately after seeing the player.

---

## 问题背景 (Problem Background)

### 原有机制 (Original Mechanism)

**中文**:
- **视觉累积系统**: 为了防止频繁的 LLM 调用，`SensoryComponent` 实现了视觉累积机制
- **累积阈值**: 默认需要看到同一目标 3 次（`AccumulationThreshold = 3`）才会触发认知事件
- **感知冷却**: 同一目标的感知冷却时间为 5 秒
- **问题**: 僵尸看到玩家后，需要等待多次感知累积才会触发 LLM 调用，导致反应延迟

**English**:
- **Visual Accumulation System**: To prevent frequent LLM calls, `SensoryComponent` implemented a visual accumulation mechanism
- **Accumulation Threshold**: By default, the same target needs to be seen 3 times (`AccumulationThreshold = 3`) to trigger a cognitive event
- **Perception Cooldown**: Perception cooldown for the same target is 5 seconds
- **Problem**: After a zombie sees the player, it needs to wait for multiple perception accumulations to trigger an LLM call, causing reaction delay

---

## 解决方案 (Solution)

### 设计思路 (Design Philosophy)

**中文**:
即时响应系统采用**双轨制 + 阵营系统**设计：
1. **敌对阵营** → 即时响应（Magnitude 0.8），立即触发认知事件
2. **玩家** → 中等优先级（Magnitude 0.4），不触发即时响应，给 LLM 更多思考空间
3. **同阵营/中立** → 累积响应（Magnitude 0.05-0.1），需要多次观察才触发

**设计理由**:
- **敌对阵营**：真正的威胁，需要立即反应（如僵尸看到人类守卫）
- **玩家**：虽然重要，但不应立即敌对，让 LLM 根据性格和情境决定反应（如友好 NPC 看到玩家可能打招呼，而不是攻击）
- **同阵营**：避免过度关注，减少不必要的 LLM 调用

这种设计既保证了对真正威胁的快速反应，又给了 AI 对玩家更灵活、更智能的反应空间。

**English**:
The Immediate Response System adopts a **dual-track + faction system** design:
1. **Hostile Factions** → Immediate response (Magnitude 0.8), trigger cognitive event instantly
2. **Player** → Medium priority (Magnitude 0.4), no immediate response, gives LLM more thinking space
3. **Same Faction/Neutral** → Accumulated response (Magnitude 0.05-0.1), requires multiple observations to trigger

**Design Rationale**:
- **Hostile Factions**: Real threats requiring immediate reaction (e.g., zombie seeing human guard)
- **Player**: Important but shouldn't be immediately hostile, let LLM decide reaction based on personality and context (e.g., friendly NPC seeing player might greet instead of attack)
- **Same Faction**: Avoid excessive attention, reduce unnecessary LLM calls

This design ensures quick response to real threats while giving AI more flexible and intelligent reaction space towards players.

---

## 实现细节 (Implementation Details)

### 代码流程 (Code Flow)

```cpp
// In ProcessEventFilter():

// Rule 0: Immediate Threat Response (Bypass Accumulation)
if (Event.Verb.MatchesTag(AINPCTags::Event_Danger))
{
    // Already upgraded to Danger in HandleTargetPerceived
    return true; // Immediate pass-through
}

// Check if target is high-priority (Player or Enemy)
if (bEnableImmediateResponse && Event.Target && Event.Verb.MatchesTag(AINPCTags::Perception_Vision))
{
    bool bIsHighPriority = Event.Target->ActorHasTag("Player") || Event.Target->ActorHasTag("Enemy");
    
    if (bIsHighPriority)
    {
        // Upgrade to immediate awareness
        Event.Verb = AINPCTags::Social_Greet;
        Event.Magnitude = FMath::Max(Event.Magnitude, HighPriorityMagnitude);
        Event.Content = FString::Printf(TEXT("I noticed %s"), *Event.Target->GetName());
        
        // Reset accumulation to prevent duplicate events
        ResetVisualAccumulation(Event.Target);
        
        return true; // Immediate pass-through
    }
}

// Rule 1: Visual Accumulation Filter (for low-priority targets)
// ... existing accumulation logic ...
```

### 关键机制 (Key Mechanisms)

**中文**:
1. **标签检测**: 通过 `ActorHasTag("Player")` 和 `ActorHasTag("Enemy")` 识别高优先级目标
2. **事件升级**: 将 `Perception.Vision` 升级为 `Social.Greet`（或可自定义为 `Social.Awareness`）
3. **重要性保证**: 使用 `FMath::Max()` 确保高优先级目标的 Magnitude 不低于配置值
4. **累积重置**: 调用 `ResetVisualAccumulation()` 防止重复触发

**English**:
1. **Tag Detection**: Identifies high-priority targets via `ActorHasTag("Player")` and `ActorHasTag("Enemy")`
2. **Event Upgrade**: Upgrades `Perception.Vision` to `Social.Greet` (or customizable to `Social.Awareness`)
3. **Magnitude Guarantee**: Uses `FMath::Max()` to ensure high-priority target Magnitude is not below configured value
4. **Accumulation Reset**: Calls `ResetVisualAccumulation()` to prevent duplicate triggers

---

## 配置参数 (Configuration Parameters)

### 在编辑器中配置 (Configuration in Editor)

**中文**: 在 `SensoryComponent` 的 Details 面板中，你可以找到以下配置选项：

**English**: In the `SensoryComponent` Details panel, you can find the following configuration options:

#### AI Sensory | Immediate Response

| 参数 (Parameter) | 类型 (Type) | 默认值 (Default) | 说明 (Description) |
|-----------------|------------|-----------------|-------------------|
| `bEnableImmediateResponse` | bool | `true` | **中文**: 是否启用即时响应系统<br>**English**: Enable immediate response system |
| `HighPriorityMagnitude` | float | `0.5` | **中文**: 高优先级目标的感知重要性（0.0-1.0）<br>**English**: Perception magnitude for high-priority targets (0.0-1.0) |

#### AI Sensory | Attention

| 参数 (Parameter) | 类型 (Type) | 默认值 (Default) | 说明 (Description) |
|-----------------|------------|-----------------|-------------------|
| `PerceptionCooldown` | float | `5.0` | **中文**: 同一目标的感知冷却时间（秒）<br>**English**: Perception cooldown for same target (seconds) |
| `AccumulationThreshold` | int32 | `3` | **中文**: 低优先级目标触发认知事件所需的累积次数<br>**English**: Accumulation count needed for low-priority targets to trigger cognitive event |

---

## 使用指南 (Usage Guide)

### 1. 配置阵营系统 (Configure Faction System)

**中文**:
阵营系统是即时响应的核心。只有不同阵营的 NPC 才会互相触发即时响应。

**配置步骤**:
1. 打开你的 PersonalityConfig DataTable（例如 `DT_PersonalityConfig`）
2. 为每个性格配置设置 `Faction` 字段：
   - `"Human"` - 人类阵营（玩家、友军NPC）
   - `"Zombie"` - 僵尸阵营
   - `"Monster"` - 怪物阵营
   - `"Neutral"` - 中立阵营（不会与任何阵营敌对）

**示例配置**:
```
PersonalityID: "Zombie"
Faction: "Zombie"
RoleDescription: "You are a hostile zombie"

PersonalityID: "FriendlyGuard"
Faction: "Human"
RoleDescription: "You are a friendly guard"

PersonalityID: "Merchant"
Faction: "Neutral"
RoleDescription: "You are a neutral merchant"
```

**English**:
The faction system is the core of immediate response. Only NPCs from different factions will trigger immediate response to each other.

**Configuration Steps**:
1. Open your PersonalityConfig DataTable (e.g., `DT_PersonalityConfig`)
2. Set the `Faction` field for each personality configuration:
   - `"Human"` - Human faction (players, friendly NPCs)
   - `"Zombie"` - Zombie faction
   - `"Monster"` - Monster faction
   - `"Neutral"` - Neutral faction (won't be hostile to any faction)

**Example Configuration**:
```
PersonalityID: "Zombie"
Faction: "Zombie"
RoleDescription: "You are a hostile zombie"

PersonalityID: "FriendlyGuard"
Faction: "Human"
RoleDescription: "You are a friendly guard"

PersonalityID: "Merchant"
Faction: "Neutral"
RoleDescription: "You are a neutral merchant"
```

---

### 2. 阵营敌对规则 (Faction Hostility Rules)

**中文**:
- ✅ **不同阵营** → 敌对，触发即时响应
  - 例如：Zombie 看到 Human → 立即触发认知事件
- ❌ **相同阵营** → 友军，不触发即时响应
  - 例如：Zombie 看到 Zombie → 需要累积 3 次才触发
- ❌ **中立阵营** → 永远不敌对
  - 例如：Neutral 看到任何人 → 需要累积才触发

**English**:
- ✅ **Different factions** → Hostile, trigger immediate response
  - Example: Zombie sees Human → Immediately trigger cognitive event
- ❌ **Same faction** → Friendly, no immediate response
  - Example: Zombie sees Zombie → Requires 3 accumulations to trigger
- ❌ **Neutral faction** → Never hostile
  - Example: Neutral sees anyone → Requires accumulation to trigger

---

### 3. 确保目标有正确的标签 (Ensure Targets Have Correct Tags)

**中文**:
为了让即时响应系统正常工作，你需要确保：
- 玩家角色有 `"Player"` 标签
- ~~敌对 NPC 有 `"Enemy"` 标签~~（不再需要，使用阵营系统）

在 Blueprint 中设置标签：
1. 选择你的 Actor（如 BP_PlayerCharacter）
2. 在 Details 面板中找到 `Tags` 数组
3. 添加 `"Player"`

**English**:
For the immediate response system to work properly, you need to ensure:
- Player characters have the `"Player"` tag
- ~~Hostile NPCs have the `"Enemy"` tag~~（No longer needed, use faction system）

To set tags in Blueprint:
1. Select your Actor (e.g., BP_PlayerCharacter)
2. Find the `Tags` array in the Details panel
3. Add `"Player"`

---

### 4. 调整响应灵敏度 (Adjust Response Sensitivity)

**中文**:
根据你的游戏需求调整参数：

- **快速反应的僵尸**:
  - `bEnableImmediateResponse = true`
  - `HighPriorityMagnitude = 0.7-0.9` (高重要性)
  
- **迟钝的僵尸**:
  - `bEnableImmediateResponse = false` (禁用即时响应)
  - `AccumulationThreshold = 5-10` (需要更多次观察)

- **平衡设置**:
  - `bEnableImmediateResponse = true`
  - `HighPriorityMagnitude = 0.5`
  - `AccumulationThreshold = 3`

**English**:
Adjust parameters based on your game requirements:

- **Fast-reacting zombies**:
  - `bEnableImmediateResponse = true`
  - `HighPriorityMagnitude = 0.7-0.9` (high importance)
  
- **Slow zombies**:
  - `bEnableImmediateResponse = false` (disable immediate response)
  - `AccumulationThreshold = 5-10` (require more observations)

- **Balanced settings**:
  - `bEnableImmediateResponse = true`
  - `HighPriorityMagnitude = 0.5`
  - `AccumulationThreshold = 3`

### 5. 调试和验证 (Debugging and Verification)

**中文**:
启用日志输出来验证系统是否正常工作：

```cpp
// 在 Output Log 中查找以下日志：

// 敌对阵营触发即时响应
[Sensory] Hostile Target Detected: BP_PlayerCharacter (Self: Zombie, Target: Human, Magnitude: 0.50)

// 危险事件（Enemy 标签）
[Sensory] Immediate Threat Detected: BP_Zombie (Danger Event)

// 同阵营不触发即时响应，进入累积系统
[Sensory] Visual Accumulation for BP_Zombie2: 1/3
[Sensory] Visual Accumulation for BP_Zombie2: 2/3
[Sensory] Visual Accumulation Threshold Reached for BP_Zombie2 (Count: 3)
```

**English**:
Enable log output to verify the system is working correctly:

```cpp
// Look for these logs in Output Log:

// Hostile faction triggers immediate response
[Sensory] Hostile Target Detected: BP_PlayerCharacter (Self: Zombie, Target: Human, Magnitude: 0.50)

// Danger event (Enemy tag)
[Sensory] Immediate Threat Detected: BP_Zombie (Danger Event)

// Same faction doesn't trigger immediate response, enters accumulation system
[Sensory] Visual Accumulation for BP_Zombie2: 1/3
[Sensory] Visual Accumulation for BP_Zombie2: 2/3
[Sensory] Visual Accumulation Threshold Reached for BP_Zombie2 (Count: 3)
```

---

## 扩展建议 (Extension Suggestions)

### 1. 自定义感知标签 (Custom Perception Tags)

**中文**:
当前实现使用 `Social.Greet` 作为即时感知的标签。你可以创建一个专用的标签：

```cpp
// In AINPCTags.h
static const FGameplayTag Social_Awareness = FGameplayTag::RequestGameplayTag(FName("Social.Awareness"));

// In SensoryComponent.cpp
Event.Verb = AINPCTags::Social_Awareness; // Instead of Social_Greet
```

**English**:
The current implementation uses `Social.Greet` as the tag for immediate perception. You can create a dedicated tag:

```cpp
// In AINPCTags.h
static const FGameplayTag Social_Awareness = FGameplayTag::RequestGameplayTag(FName("Social.Awareness"));

// In SensoryComponent.cpp
Event.Verb = AINPCTags::Social_Awareness; // Instead of Social_Greet
```

### 2. 基于距离的动态调整 (Distance-based Dynamic Adjustment)

**中文**:
根据目标距离动态调整 Magnitude：

```cpp
float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Event.Target->GetActorLocation());
float DistanceFactor = FMath::Clamp(1.0f - (Distance / 2000.0f), 0.3f, 1.0f);
Event.Magnitude = HighPriorityMagnitude * DistanceFactor;
```

**English**:
Dynamically adjust Magnitude based on target distance:

```cpp
float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Event.Target->GetActorLocation());
float DistanceFactor = FMath::Clamp(1.0f - (Distance / 2000.0f), 0.3f, 1.0f);
Event.Magnitude = HighPriorityMagnitude * DistanceFactor;
```

### 3. 性格特质影响 (Personality Trait Influence)

**中文**:
结合 `PersonalityComponent` 让不同性格的 AI 有不同的反应速度：

```cpp
// In SensoryComponent
if (UPersonalityComponent* Personality = GetOwner()->FindComponentByClass<UPersonalityComponent>())
{
    float Neuroticism = Personality->GetOCEANTrait(EOCEANTrait::Neuroticism);
    // High neuroticism = more reactive
    Event.Magnitude = HighPriorityMagnitude * (0.5f + Neuroticism * 0.5f);
}
```

**English**:
Combine with `PersonalityComponent` to give different personalities different reaction speeds:

```cpp
// In SensoryComponent
if (UPersonalityComponent* Personality = GetOwner()->FindComponentByClass<UPersonalityComponent>())
{
    float Neuroticism = Personality->GetOCEANTrait(EOCEANTrait::Neuroticism);
    // High neuroticism = more reactive
    Event.Magnitude = HighPriorityMagnitude * (0.5f + Neuroticism * 0.5f);
}
```

---

## 性能影响 (Performance Impact)

**中文**:
- **LLM 调用频率**: 对高优先级目标的即时响应会增加 LLM 调用频率
- **建议**: 结合 `PerceptionCooldown` 参数控制调用频率，避免同一目标在短时间内重复触发
- **优化**: 考虑在 `CognitionComponent` 中实现请求队列和优先级管理

**English**:
- **LLM Call Frequency**: Immediate response to high-priority targets will increase LLM call frequency
- **Recommendation**: Use `PerceptionCooldown` parameter to control call frequency and prevent the same target from triggering repeatedly in a short time
- **Optimization**: Consider implementing request queue and priority management in `CognitionComponent`

---

## 总结 (Summary)

**中文**:
即时响应系统通过**双轨制 + 阵营系统**的设计，在保持原有累积过滤机制优势的同时，为敌对阵营目标提供了快速响应能力。这使得 AI 能够：
- ✅ 立即对敌对阵营的目标做出反应（如僵尸看到人类）
- ✅ 避免同阵营 NPC 互相攻击（如僵尸不会攻击僵尸）
- ✅ 支持中立阵营，不会对任何阵营产生即时敌对
- ✅ 通过编辑器参数灵活调整行为
- ✅ 保持良好的性能和可扩展性
- ✅ 使用 PersonalityConfig 统一管理阵营配置

**English**:
The Immediate Response System, through its **dual-track + faction system** design, provides fast response capability for hostile faction targets while maintaining the advantages of the original accumulation filtering mechanism. This enables AI to:
- ✅ React immediately to hostile faction targets (e.g., zombies seeing humans)
- ✅ Avoid same-faction NPCs attacking each other (e.g., zombies won't attack zombies)
- ✅ Support neutral factions that won't be immediately hostile to any faction
- ✅ Flexibly adjust behavior through editor parameters
- ✅ Maintain good performance and extensibility
- ✅ Manage faction configuration uniformly through PersonalityConfig

---

## 相关文件 (Related Files)

- `Source/AINPC/Components/SensoryComponent.h`
- `Source/AINPC/Components/SensoryComponent.cpp`
- `Source/AINPC/Social/SocialTypes.h` (for GameplayTags)

---

**版本 (Version)**: 1.0  
**最后更新 (Last Updated)**: 2026-01-09
