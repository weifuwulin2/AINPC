# LLM + Utility AI 架构重构设计 / Architecture Refactoring Design

**设计日期 / Design Date:** 2026-01-05  
**设计目标 / Design Goal:** 让LLM成为真正的"认知大脑"，Utility AI成为"运动神经系统" / Make LLM the true "Cognitive Brain" and Utility AI the "Motor Nervous System"

---

## 🎯 核心理念 / Core Philosophy

### 当前问题 / Current Problem
从日志可以看出，LLM只是在**被动地输出情绪值**：
From the logs, we can see that LLM is only **passively outputting emotion values**:
```
Anger=0.20, Fear=0.40, Confidence=0.60
```

然后Utility AI机械地用这些值计算分数：
```
[Test_Attack] Final Score: 0.240
[Test_Flee] Final Score: 1.200
```

**这不是"大脑"，这只是一个"情绪传感器"！**
**This is not a "brain", it's just an "emotion sensor"!**

### 新的架构理念 / New Architecture Philosophy

```
人类大脑的工作方式：
┌─────────────────────────────────────────┐
│ 感知 → 理解 → 推理 → 意图 → 行动计划    │
└─────────────────────────────────────────┘

我们的新架构：
┌──────────────────────────────────────────────────────────┐
│ Perception → LLM Cognition → Intent → Utility Execution │
│  (感知层)      (认知大脑)      (意图)    (执行系统)      │
└──────────────────────────────────────────────────────────┘
```

---

## 🧠 重新定义职责 / Redefining Responsibilities

### LLM的职责：高层认知和意图生成 / LLM's Responsibility: High-Level Cognition and Intent Generation

**LLM应该输出的不是情绪值，而是：**
**LLM should output not emotion values, but:**

1. **情境理解** (Situation Assessment)
   - "我看到了一个玩家正在接近"
   - "这个玩家之前伤害过我"
   - "我现在处于劣势"

2. **意图生成** (Intent Formation)
   - "我想要逃离危险"
   - "我想要保持警惕"
   - "我想要寻找掩护"

3. **行为倾向** (Behavioral Tendencies)
   - 不是直接指定动作，而是表达**倾向性**
   - "我倾向于防御性行为"
   - "我需要保持距离"
   - "我应该避免正面冲突"

### Utility AI的职责：将意图转化为具体行动 / Utility AI's Responsibility: Translating Intent into Concrete Actions

**Utility AI应该：**
**Utility AI should:**

1. **解释意图** (Intent Interpretation)
   - 将LLM的高层意图映射到可执行的动作

2. **评估可行性** (Feasibility Evaluation)
   - 考虑当前环境约束
   - 考虑物理限制（距离、冷却时间等）

3. **执行动作** (Action Execution)
   - 实际的移动、攻击、交互

---

## 📋 新的数据结构设计 / New Data Structure Design

### 当前的 MentalState（需要废弃）/ Current MentalState (To Be Deprecated)

```cpp
// ❌ 旧设计：只有情绪值
struct FMentalState {
    float Anger;
    float Fear;
    float Confidence;
    float SocialBattery;
    float Hunger;
};
```

### 新的 CognitiveState（推荐）/ New CognitiveState (Recommended)

```cpp
// ✅ 新设计：认知状态 + 意图
USTRUCT(BlueprintType)
struct FCognitiveState
{
    GENERATED_BODY()
    
    // ========== 情境评估 ==========
    UPROPERTY(BlueprintReadOnly)
    FString SituationSummary;  // "玩家正在接近，距离200米"
    
    UPROPERTY(BlueprintReadOnly)
    float ThreatLevel;  // 0.0-1.0 威胁程度
    
    UPROPERTY(BlueprintReadOnly)
    float OpportunityLevel;  // 0.0-1.0 机会程度
    
    // ========== 当前意图 ==========
    UPROPERTY(BlueprintReadOnly)
    FString PrimaryIntent;  // "逃离" / "攻击" / "观察" / "交互"
    
    UPROPERTY(BlueprintReadOnly)
    float IntentConfidence;  // 0.0-1.0 意图的确定性
    
    // ========== 行为倾向 ==========
    UPROPERTY(BlueprintReadOnly)
    TMap<FString, float> ActionPreferences;  // {"Flee": 0.8, "Attack": 0.2, "Hide": 0.6}
    
    // ========== 情绪状态（保留，但作为辅助）==========
    UPROPERTY(BlueprintReadOnly)
    float Anger;
    
    UPROPERTY(BlueprintReadOnly)
    float Fear;
    
    UPROPERTY(BlueprintReadOnly)
    float Confidence;
    
    // ========== 元认知 ==========
    UPROPERTY(BlueprintReadOnly)
    FString Reasoning;  // LLM的推理过程（用于调试）
};
```

---

## 🔄 新的Prompt设计 / New Prompt Design

### 当前Prompt（问题所在）/ Current Prompt (The Problem)

```
You are an AI game engine. Analyze the input and output a STRICT JSON object.
Return specific float values (0.0 to 1.0) for these fields:
{
  "Anger": float,
  "Fear": float,
  "Confidence": float,
  ...
}
```

**问题：** 这把LLM当成了一个"情绪计算器"，完全浪费了它的推理能力！

### 新的Prompt（推荐）/ New Prompt (Recommended)

```json
{
  "role": "system",
  "content": "You are the cognitive brain of an NPC in a game world. Your role is to:
  
  1. UNDERSTAND the situation based on sensory input and memories
  2. REASON about threats, opportunities, and social context
  3. FORM intentions about what you want to achieve
  4. EXPRESS behavioral preferences (not direct commands)
  
  You do NOT control the body directly. You provide high-level cognitive guidance,
  and the motor system (Utility AI) will translate your intentions into actions.
  
  Output Format (JSON):
  {
    \"situation_summary\": \"Brief description of what's happening\",
    \"threat_level\": 0.0-1.0,
    \"opportunity_level\": 0.0-1.0,
    \"primary_intent\": \"escape\" | \"attack\" | \"observe\" | \"interact\" | \"rest\",
    \"intent_confidence\": 0.0-1.0,
    \"action_preferences\": {
      \"Flee\": 0.0-1.0,
      \"Attack\": 0.0-1.0,
      \"Hide\": 0.0-1.0,
      \"Patrol\": 0.0-1.0,
      \"Idle\": 0.0-1.0
    },
    \"emotions\": {
      \"anger\": 0.0-1.0,
      \"fear\": 0.0-1.0,
      \"confidence\": 0.0-1.0
    },
    \"reasoning\": \"Why you formed this intent (1-2 sentences)\"
  }
  
  Example:
  Input: \"I see a player approaching. They damaged me 30 seconds ago.\"
  Output: {
    \"situation_summary\": \"Hostile player returning, I'm wounded\",
    \"threat_level\": 0.8,
    \"opportunity_level\": 0.1,
    \"primary_intent\": \"escape\",
    \"intent_confidence\": 0.9,
    \"action_preferences\": {
      \"Flee\": 0.9,
      \"Attack\": 0.1,
      \"Hide\": 0.7,
      \"Patrol\": 0.0,
      \"Idle\": 0.0
    },
    \"emotions\": {
      \"anger\": 0.3,
      \"fear\": 0.8,
      \"confidence\": 0.2
    },
    \"reasoning\": \"This player is dangerous and I'm already hurt. I should escape to survive.\"
  }"
}
```

---

## 🔧 Utility AI的新计算逻辑 / New Utility AI Calculation Logic

### 当前逻辑（问题）/ Current Logic (Problem)

```cpp
// ❌ 机械地用情绪值计算
float score = BaseWeight;
score *= GetEmotionValue("Anger");  // 0.2
score *= GetEmotionValue("Confidence");  // 0.6
// Result: 2.0 * 0.2 * 0.6 = 0.24
```

**问题：** 完全忽略了LLM的"意图"！

### 新逻辑（推荐）/ New Logic (Recommended)

```cpp
float UUtilityActionBase::CalculateScore(AAIController* Controller)
{
    FCognitiveState CogState = GetCognitiveState(Controller);
    
    float FinalScore = 0.0f;
    
    // ========== 1. LLM的意图权重（最重要！）==========
    float IntentWeight = 0.0f;
    if (CogState.ActionPreferences.Contains(ActionName))
    {
        IntentWeight = CogState.ActionPreferences[ActionName];
        IntentWeight *= CogState.IntentConfidence;  // 考虑LLM的确定性
    }
    
    // ========== 2. 环境可行性（Utility AI的职责）==========
    float FeasibilityScore = 1.0f;
    
    // 距离检查
    if (RequiresTarget)
    {
        float Distance = GetDistanceToTarget(Controller);
        FeasibilityScore *= EvaluateDistance(Distance);
    }
    
    // 冷却时间
    if (IsOnCooldown())
    {
        FeasibilityScore = 0.0f;
    }
    
    // 资源检查（体力、弹药等）
    if (!HasRequiredResources())
    {
        FeasibilityScore *= 0.1f;
    }
    
    // ========== 3. 综合计算 ==========
    // 意图占70%，可行性占30%
    FinalScore = (IntentWeight * 0.7f) + (FeasibilityScore * BaseWeight * 0.3f);
    
    // ========== 4. 惯性奖励 ==========
    if (IsCurrentAction())
    {
        FinalScore += InertiaBonus;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[%s] Intent=%.2f, Feasibility=%.2f, Final=%.2f"),
           *ActionName, IntentWeight, FeasibilityScore, FinalScore);
    
    return FinalScore;
}
```

---

## 📊 对比：旧 vs 新 / Comparison: Old vs New

### 旧架构的数据流

```
感知事件 → LLM → [Anger=0.2, Fear=0.4] → Utility AI → 机械计算 → 动作
              ↑                                ↑
           "情绪传感器"                    "计算器"
```

### 新架构的数据流

```
感知事件 → LLM → [Intent="escape", Preferences={Flee:0.9}] → Utility AI → 考虑环境 → 动作
              ↑                                                    ↑
         "认知大脑"                                          "运动神经"
         (理解+推理)                                      (可行性+执行)
```

---

## 🎮 实际例子 / Practical Example

### 场景：玩家接近受伤的NPC / Scenario: Player Approaching Wounded NPC

#### 旧系统的处理

```
LLM输出：
{
  "Anger": 0.2,
  "Fear": 0.4,
  "Confidence": 0.6
}

Utility AI计算：
Attack: 2.0 * 0.2 * 0.6 = 0.24
Flee: 3.0 * 0.4 = 1.2  ← 选择这个

结果：NPC逃跑
```

**问题：** 
- LLM没有表达"为什么"要逃跑
- Utility AI不知道LLM的"意图"
- 纯粹是数值碰撞的结果

#### 新系统的处理

```
LLM输出：
{
  "situation_summary": "Hostile player approaching, I'm wounded",
  "primary_intent": "escape",
  "intent_confidence": 0.9,
  "action_preferences": {
    "Flee": 0.9,
    "Hide": 0.7,
    "Attack": 0.1
  },
  "reasoning": "I'm injured and this player is dangerous. I need to escape."
}

Utility AI计算：
Flee:
  - Intent Weight: 0.9 * 0.9 = 0.81
  - Feasibility: 1.0 (有导航路径)
  - Final: 0.81 * 0.7 + 1.0 * 0.3 = 0.87

Hide:
  - Intent Weight: 0.7 * 0.9 = 0.63
  - Feasibility: 0.3 (附近没有掩体)
  - Final: 0.63 * 0.7 + 0.3 * 0.3 = 0.53

Attack:
  - Intent Weight: 0.1 * 0.9 = 0.09
  - Feasibility: 0.8 (在攻击范围内)
  - Final: 0.09 * 0.7 + 0.8 * 0.3 = 0.30

结果：选择Flee (0.87)
```

**优势：**
- LLM明确表达了"逃跑"的意图和原因
- Utility AI考虑了环境可行性（有没有路可逃）
- 决策过程可解释、可调试

---

## 🚀 实施步骤 / Implementation Steps

### Phase 1: 数据结构重构
1. 创建 `FCognitiveState` 结构体
2. 更新 `LLMCommunicator` 的返回类型
3. 更新 `CognitionComponent` 的状态管理

### Phase 2: Prompt重构
1. 重写System Prompt，强调"认知大脑"角色
2. 要求LLM输出意图和偏好，而不只是情绪
3. 添加推理过程输出（用于调试）

### Phase 3: Utility AI重构
1. 修改 `CalculateScore()` 逻辑
2. 优先考虑LLM的意图权重
3. 添加环境可行性评估

### Phase 4: 测试和调优
1. 对比新旧系统的决策质量
2. 调整意图权重 vs 可行性权重的比例
3. 优化Prompt以获得更好的意图表达

---

## 💡 理论基础 / Theoretical Foundation

这个架构基于：

1. **双过程理论** (Dual Process Theory)
   - System 1 (快速、直觉) = Utility AI的可行性评估
   - System 2 (慢速、推理) = LLM的认知推理

2. **意图-行为分离** (Intent-Action Separation)
   - 意图是"想要什么" (LLM负责)
   - 行为是"如何做到" (Utility AI负责)

3. **分层控制** (Hierarchical Control)
   - 高层：战略意图 (LLM)
   - 中层：战术选择 (Utility AI)
   - 低层：运动执行 (Animation/Movement)

---

## 📈 预期效果 / Expected Results

### 可解释性提升
```
旧：为什么NPC逃跑？"因为Fear=0.4"
新：为什么NPC逃跑？"因为LLM判断威胁高，意图是escape，且环境允许逃跑"
```

### 灵活性提升
```
旧：只能通过调整Response Curve来改变行为
新：可以通过改变Prompt让LLM产生不同的意图
```

### 通用性提升
```
旧：每个情绪-动作映射需要手动配置
新：LLM自动理解情境并生成合理的意图
```

---

**下一步：** 是否要我开始实现这个新架构？
