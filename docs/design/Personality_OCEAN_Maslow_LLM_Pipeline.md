# Personality → OCEAN → Maslow → LLM 完整数据流
# Complete Data Flow: Personality → OCEAN → Maslow → LLM

**文档版本 / Document Version:** 1.0  
**创建日期 / Created:** 2026-01-08  
**目标 / Purpose:** 详细阐述从静态性格配置到动态心理状态的完整转换链路

---

## 🎯 核心概念 / Core Concept

本系统实现了一个**三层心理模型**,模拟真实的人类认知过程:

```
┌─────────────────────────────────────────────────────────────────┐
│  Layer 1: Personality (静态特质)                                 │
│  ↓ OCEAN Big Five Traits                                        │
│  Layer 2: Maslow Weights (动态权重)                              │
│  ↓ Psychological Sensitivity Matrix                             │
│  Layer 3: LLM Cognition (实时推理)                               │
│  ↓ Semantic Labels → Numeric Values                             │
│  Layer 4: Utility AI (行为选择)                                  │
└─────────────────────────────────────────────────────────────────┘
```

**关键创新点:**
- **Personality不是装饰**: OCEAN特质通过数学转换直接影响行为
- **Maslow作为中间层**: 将抽象性格映射到具体心理需求
- **LLM作为认知引擎**: 基于权重化的需求进行语义推理
- **Utility AI作为执行层**: 将心理状态转化为具体动作

---

## 📊 Layer 1: Personality (OCEAN Big Five)

### 1.1 数据结构 / Data Structure

```cpp
USTRUCT(BlueprintType)
struct FPersonalityConfig
{
    GENERATED_BODY()
    
    // === OCEAN Personality Traits (Big Five) ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Openness;           // 开放性 (0.0 - 1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Conscientiousness;  // 尽责性 (0.0 - 1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Extraversion;       // 外向性 (0.0 - 1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Agreeableness;      // 宜人性 (0.0 - 1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Neuroticism;        // 神经质 (0.0 - 1.0)
    
    // === Role-Playing System ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RoleDescription;      // 角色描述 (Who am I?)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString BehavioralGuidelines; // 行为准则 (How should I act?)
};
```

### 1.2 OCEAN特质的心理学含义 / Psychological Meaning

| Trait | 特质 | Low (0.0-0.3) | High (0.7-1.0) | 影响的Maslow需求 |
|-------|------|---------------|----------------|-----------------|
| **Openness** | 开放性 | 保守、传统 | 好奇、创新 | **Curiosity** (好奇心) |
| **Conscientiousness** | 尽责性 | 冲动、散漫 | 自律、可靠 | **Duty_Urgency** (责任感) |
| **Extraversion** | 外向性 | 内向、独处 | 外向、社交 | **Loneliness** (孤独感) |
| **Agreeableness** | 宜人性 | 竞争、怀疑 | 合作、信任 | **Anger** (愤怒), **Trust** (信任) |
| **Neuroticism** | 神经质 | 冷静、坚韧 | 焦虑、敏感 | **Perceived_Threat** (威胁感), **Anger** (愤怒) |

### 1.3 预设性格模板示例 / Preset Personality Templates

#### 勇敢战士 / Brave Warrior
```cpp
Openness: 0.4          // 传统战斗风格
Conscientiousness: 0.7 // 高度自律
Extraversion: 0.6      // 适度社交
Agreeableness: 0.3     // 竞争性强
Neuroticism: 0.2       // 情绪稳定

RoleDescription: "You are a battle-hardened warrior who values honor and courage."
BehavioralGuidelines:
  1. Never flee from a fair fight
  2. Protect the weak
  3. Anger rises when honor is questioned
```

#### 谨慎守卫 / Cautious Guard
```cpp
Openness: 0.3          // 保守
Conscientiousness: 0.9 // 极度尽责
Extraversion: 0.4      // 内向
Agreeableness: 0.5     // 中立
Neuroticism: 0.6       // 警觉性高

RoleDescription: "You are a vigilant guard responsible for castle security."
BehavioralGuidelines:
  1. Strangers are potential threats (Perceived_Threat = 0.6)
  2. Duty comes before personal safety (Duty_Urgency = 0.9)
  3. Trust only verified allies
```

---

## 🔄 Layer 2: OCEAN → Maslow 转换矩阵

### 2.1 转换机制 / Transformation Mechanism

**核心公式 / Core Formula:**
```
MaslowWeight[Need] = Σ (OCEAN[Trait] × Coefficient[Trait→Need])
```

**实现位置 / Implementation:**
- **配置文件**: `UPsychologyModel` DataAsset
- **计算组件**: `UPersonalityComponent::RecalculateWeights()`
- **调用时机**: `BeginPlay()` 一次性计算

### 2.2 转换系数表 / Coefficient Matrix

#### 完整映射表 / Complete Mapping Table

| Maslow Need | 马斯洛需求 | 影响因子 (OCEAN Trait × Coefficient) |
|-------------|-----------|-------------------------------------|
| **Hunger** | 饥饿 | ❌ Engine-Exclusive (引擎独占,不受OCEAN影响) |
| **Energy** | 精力 | ❌ Engine-Exclusive |
| **Perceived_Threat** | 威胁感 | **Neuroticism × 3.0** (神经质越高,越敏感) |
| **Resource_Anxiety** | 资源焦虑 | **Neuroticism × 2.0** + **Conscientiousness × 1.5** |
| **Loneliness** | 孤独感 | **Extraversion × 2.5** (外向者更需要社交) |
| **Trust** | 信任 | **Agreeableness × 2.0** (宜人性高,更容易信任) |
| **Anger** | 愤怒 | **Neuroticism × 2.5** - **Agreeableness × 1.5** |
| **Social_Status** | 社会地位 | **Extraversion × 1.8** + **Conscientiousness × 1.2** |
| **Duty_Urgency** | 责任紧迫 | **Conscientiousness × 2.5** |
| **Curiosity** | 好奇心 | **Openness × 3.0** |

### 2.3 计算示例 / Calculation Examples

#### 示例1: 高神经质NPC (Neuroticism = 0.8)

```cpp
// Input OCEAN
Neuroticism = 0.8
Agreeableness = 0.3

// Calculation
Perceived_Threat_Weight = 0.8 × 3.0 = 2.4  // 非常敏感
Anger_Weight = (0.8 × 2.5) - (0.3 × 1.5) = 2.0 - 0.45 = 1.55  // 易怒

// Impact on Utility AI
当LLM输出 Perceived_Threat = "Moderate" (0.5) 时:
实际影响 = 0.5 × 2.4 = 1.2  // 被放大到超过正常值
```

**行为表现**: 这个NPC会对轻微威胁产生强烈反应,更容易选择逃跑动作。

#### 示例2: 高外向性NPC (Extraversion = 0.9)

```cpp
// Input OCEAN
Extraversion = 0.9

// Calculation
Loneliness_Weight = 0.9 × 2.5 = 2.25  // 非常需要社交

// Impact on Utility AI
当NPC长时间独处时:
Loneliness值逐渐上升 → 0.7
实际影响 = 0.7 × 2.25 = 1.575  // 强烈的社交需求
```

**行为表现**: 这个NPC会主动寻找其他角色互动,不喜欢独自巡逻。

### 2.4 代码实现 / Code Implementation

```cpp
// PersonalityComponent.cpp
void UPersonalityComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 从DataTable加载性格配置
    LoadPersonalityFromDataTable();
    
    // 计算Maslow权重
    if (PsychologyModel)
    {
        MaslowWeights = PsychologyModel->RecalculateWeights(PersonalityConfig);
        
        UE_LOG(LogTemp, Log, TEXT("Maslow Weights Calculated:"));
        UE_LOG(LogTemp, Log, TEXT("  Perceived_Threat: %.2f"), MaslowWeights.PerceivedThreatWeight);
        UE_LOG(LogTemp, Log, TEXT("  Anger: %.2f"), MaslowWeights.AngerWeight);
        UE_LOG(LogTemp, Log, TEXT("  Loneliness: %.2f"), MaslowWeights.LonelinessWeight);
    }
}

// PsychologyModel.cpp
FMaslowWeights UPsychologyModel::RecalculateWeights(const FPersonalityConfig& Personality)
{
    FMaslowWeights Weights;
    
    // Perceived_Threat = Neuroticism × 3.0
    Weights.PerceivedThreatWeight = Personality.Neuroticism * Neuroticism_To_Threat;
    
    // Anger = (Neuroticism × 2.5) - (Agreeableness × 1.5)
    Weights.AngerWeight = (Personality.Neuroticism * Neuroticism_To_Anger) 
                        - (Personality.Agreeableness * Agreeableness_To_Anger);
    
    // Loneliness = Extraversion × 2.5
    Weights.LonelinessWeight = Personality.Extraversion * Extraversion_To_Loneliness;
    
    // ... 其他权重计算
    
    return Weights;
}
```

---

## 🧠 Layer 3: LLM 认知处理

### 3.1 Prompt构建 / Prompt Construction

LLM接收的Prompt包含**三部分信息**:

```json
{
  "role": "system",
  "content": "
    === PERSONALITY ===
    Role: You are a cautious guard...
    Guidelines:
      1. Strangers trigger Moderate Perceived_Threat (0.5)
      2. Duty_Urgency is always High (0.8)
    
    === CURRENT STATE ===
    Health: 100/100
    Hunger: 0.3 (Engine-Managed, DO NOT MODIFY)
    Energy: 0.8 (Engine-Managed, DO NOT MODIFY)
    
    === SENSORY INPUT ===
    You see a stranger approaching the gate.
    Distance: 10 meters.
    
    === OUTPUT REQUIREMENTS ===
    Output JSON with semantic labels (NOT numeric values):
    {
      \"Perceived_Threat\": \"None\" | \"Slight\" | \"Moderate\" | \"Strong\" | \"Extreme\",
      \"Anger\": \"None\" | \"Slight\" | \"Moderate\" | \"Strong\" | \"Extreme\",
      \"Trust\": \"None\" | \"Slight\" | \"Moderate\" | \"Strong\" | \"Extreme\",
      \"Duty_Urgency\": \"None\" | \"Slight\" | \"Moderate\" | \"Strong\" | \"Extreme\"
    }
  "
}
```

### 3.2 LLM的职责边界 / LLM's Jurisdiction

#### ✅ LLM可以修改 (Psychological Attributes)

| Attribute | 属性 | 管辖权 | 原因 |
|-----------|------|--------|------|
| **Anger** | 愤怒 | LLM-Exclusive | 纯心理状态,需要语义推理 |
| **Trust** | 信任 | LLM-Exclusive | 基于社交互动的判断 |
| **Social_Status** | 社会地位 | LLM-Exclusive | 需要理解社会关系 |
| **Curiosity** | 好奇心 | LLM-Exclusive | 需要理解新奇性 |
| **Perceived_Threat** | 威胁感 | Mixed | LLM判断语义威胁,Engine提供物理数据 |
| **Duty_Urgency** | 责任紧迫 | Mixed | LLM理解任务,Engine提供时间压力 |
| **Loneliness** | 孤独感 | Mixed | LLM判断社交质量,Engine计算独处时间 |
| **Resource_Anxiety** | 资源焦虑 | Mixed | LLM评估风险,Engine提供资源数据 |

#### ❌ LLM不能修改 (Physical Attributes)

| Attribute | 属性 | 管辖权 | 原因 |
|-----------|------|--------|------|
| **Hunger** | 饥饿 | Engine-Exclusive | 物理状态,时间驱动 |
| **Energy** | 精力 | Engine-Exclusive | 物理状态,行动消耗 |

### 3.3 语义标签系统 / Semantic Label System

**为什么使用标签而不是数值?**
- ✅ **稳定性**: LLM更擅长输出离散标签
- ✅ **可解释性**: "Furious"比"0.95"更直观
- ✅ **容错性**: 标签映射允许模糊匹配

#### 标签映射表 / Label Mapping Table

```cpp
// SentimentMapper.cpp
float USentimentMapper::MapLabelToValue(const FString& Label)
{
    // 5-Point Scale
    if (Label.Contains("None") || Label.Contains("Calm")) return 0.0f;
    if (Label.Contains("Slight") || Label.Contains("Mild")) return 0.25f;
    if (Label.Contains("Moderate")) return 0.5f;
    if (Label.Contains("Strong") || Label.Contains("High")) return 0.75f;
    if (Label.Contains("Extreme") || Label.Contains("Intense")) return 1.0f;
    
    // Anger-Specific Labels
    if (Label.Contains("Annoyed")) return 0.3f;
    if (Label.Contains("Angry")) return 0.6f;
    if (Label.Contains("Furious")) return 0.9f;
    
    // Trust-Specific Labels
    if (Label.Contains("Distrustful")) return 0.2f;
    if (Label.Contains("Neutral")) return 0.5f;
    if (Label.Contains("Trusting")) return 0.8f;
    
    return 0.5f; // Default
}
```

### 3.4 LLM输出示例 / LLM Output Examples

#### 场景1: 守卫看到陌生人

**LLM输出:**
```json
{
  "Perceived_Threat": "Moderate",
  "Anger": "None",
  "Trust": "Slight",
  "Duty_Urgency": "Strong"
}
```

**语义映射:**
```cpp
Perceived_Threat: "Moderate" → 0.5
Anger: "None" → 0.0
Trust: "Slight" → 0.25
Duty_Urgency: "Strong" → 0.75
```

**权重调制 (基于OCEAN):**
```cpp
// 假设这是一个高神经质守卫 (Neuroticism = 0.7)
Perceived_Threat_Final = 0.5 × (0.7 × 3.0) = 0.5 × 2.1 = 1.05 → Clamped to 1.0
Duty_Urgency_Final = 0.75 × (0.8 × 2.5) = 0.75 × 2.0 = 1.5 → Clamped to 1.0
```

**结果**: 高神经质放大了威胁感,守卫会更警惕。

---

## 🎮 Layer 4: Utility AI 行为选择

### 4.1 双阶段评分算法 / Two-Phase Scoring

```cpp
// UtilityActionBase.cpp
float UUtilityActionBase::CalculateScore(const FAttributeState& State, const FMaslowWeights& Weights)
{
    float MotivationScore = 0.0f;
    float ContextScore = 1.0f;
    
    // === Phase 1: Motivation (加法) ===
    for (const FConsiderationConfig& Config : Considerations)
    {
        if (Config.Type == EConsiderationType::Motivation)
        {
            float RawValue = GetAttributeValue(State, Config.AttributeName);
            float Weight = GetMaslowWeight(Weights, Config.AttributeName);
            float WeightedValue = RawValue * Weight;  // 应用OCEAN权重
            
            MotivationScore += WeightedValue;
        }
    }
    
    // === Phase 2: Context (乘法) ===
    for (const FConsiderationConfig& Config : Considerations)
    {
        if (Config.Type == EConsiderationType::Context)
        {
            float RawValue = GetAttributeValue(State, Config.AttributeName);
            ContextScore *= RawValue;  // 任何为0则整体为0
        }
    }
    
    // === Final Score ===
    float FinalScore = BaseReward * MotivationScore * ContextScore;
    
    return FinalScore;
}
```

### 4.2 权重如何影响动作选择 / How Weights Affect Action Selection

#### 示例: Flee动作配置

```cpp
// DataTable: UtilityActionConfig
ActionName: "Test_Flee"
BaseReward: 3.0

Considerations:
  - Type: Motivation
    AttributeName: "Perceived_Threat"
    ResponseCurve: Linear (0→1)
    
  - Type: Context
    AttributeName: "Energy"
    ResponseCurve: Threshold (>0.2)
```

#### 计算过程 (两个不同性格的NPC)

**NPC A: 低神经质 (Neuroticism = 0.2)**
```cpp
// Maslow Weight
Perceived_Threat_Weight = 0.2 × 3.0 = 0.6

// LLM Output
Perceived_Threat = "Moderate" → 0.5

// Motivation Calculation
MotivationScore = 0.5 × 0.6 = 0.3

// Context (Energy = 0.8)
ContextScore = 1.0

// Final Score
FinalScore = 3.0 × 0.3 × 1.0 = 0.9
```

**NPC B: 高神经质 (Neuroticism = 0.8)**
```cpp
// Maslow Weight
Perceived_Threat_Weight = 0.8 × 3.0 = 2.4

// LLM Output (相同场景)
Perceived_Threat = "Moderate" → 0.5

// Motivation Calculation
MotivationScore = 0.5 × 2.4 = 1.2

// Context (Energy = 0.8)
ContextScore = 1.0

// Final Score
FinalScore = 3.0 × 1.2 × 1.0 = 3.6
```

**结果对比:**
- NPC A (冷静): Flee Score = 0.9 (可能不逃跑)
- NPC B (焦虑): Flee Score = 3.6 (极可能逃跑)

**相同的LLM输出,不同的行为结果!** 这就是OCEAN权重的作用。

---

## 🔍 完整数据流示例 / Complete Data Flow Example

### 场景: 僵尸NPC看到玩家

#### Step 1: Personality配置
```cpp
// Zombie Personality
Openness: 0.1          // 无好奇心
Conscientiousness: 0.0 // 无自律
Extraversion: 0.0      // 无社交需求
Agreeableness: 0.0     // 无同理心
Neuroticism: 0.0       // 无恐惧

RoleDescription: "You are a mindless zombie driven by hunger."
BehavioralGuidelines:
  1. Attack any living creature immediately
  2. Your Hunger translates to EXTREME Anger (1.0)
  3. You feel NO Perceived_Threat (always 0.0)
```

#### Step 2: OCEAN → Maslow转换
```cpp
// Calculated Weights
Perceived_Threat_Weight = 0.0 × 3.0 = 0.0  // 无威胁感
Anger_Weight = (0.0 × 2.5) - (0.0 × 1.5) = 0.0  // 基础无愤怒
Loneliness_Weight = 0.0 × 2.5 = 0.0  // 无孤独感

// 注意: 僵尸的Anger由BehavioralGuidelines强制设置,不依赖OCEAN
```

#### Step 3: LLM推理
```json
// LLM Input
{
  "role": "system",
  "content": "You are a mindless zombie... Guidelines: 1. Attack immediately..."
}
{
  "role": "user",
  "content": "You see a human 5 meters away. Hunger=0.8 (Engine-Managed)."
}

// LLM Output
{
  "Anger": "Extreme",           // 遵循Guideline #2
  "Perceived_Threat": "None",   // 遵循Guideline #3
  "Trust": "None",
  "Curiosity": "None"
}
```

#### Step 4: 语义映射
```cpp
Anger: "Extreme" → 1.0
Perceived_Threat: "None" → 0.0
Trust: "None" → 0.0
```

#### Step 5: Utility AI评分
```cpp
// Test_Attack动作
BaseReward = 5.0
Considerations:
  - Motivation: Anger (Weight = 1.0, 因为没有OCEAN调制)
  - Context: Energy > 0.1

MotivationScore = 1.0 × 1.0 = 1.0
ContextScore = 1.0
FinalScore = 5.0 × 1.0 × 1.0 = 5.0  // 最高分!

// Test_Flee动作
BaseReward = 3.0
Considerations:
  - Motivation: Perceived_Threat (Weight = 0.0)

MotivationScore = 0.0 × 0.0 = 0.0
FinalScore = 3.0 × 0.0 × 1.0 = 0.0  // 零分
```

#### Step 6: 行为执行
```cpp
SelectedAction = "Test_Attack" (Score: 5.0)
→ 僵尸攻击玩家
```

---

## 📈 系统优势总结 / System Advantages

### 1. 可扩展性 / Scalability
- **添加新性格**: 只需调整OCEAN值,无需修改代码
- **添加新需求**: 在PsychologyModel中添加新的转换系数
- **添加新动作**: Utility AI自动使用权重化的需求

### 2. 可调试性 / Debuggability
```cpp
// 日志输出示例
[PersonalityComponent] OCEAN: N=0.8, A=0.3
[PersonalityComponent] Weights: Threat=2.4, Anger=1.55
[LLM] Output: Perceived_Threat="Moderate"
[CognitionComponent] Mapped: Perceived_Threat=0.5
[UtilityAI] Weighted: 0.5 × 2.4 = 1.2
[UtilityAI] Flee Score: 3.6
```

### 3. 心理学真实性 / Psychological Realism
- 基于Big Five模型 (学术验证)
- 基于Maslow需求层次 (行为心理学)
- 符合人类认知过程 (特质→需求→行为)

### 4. 性能优化 / Performance
- **OCEAN → Maslow**: 仅在BeginPlay计算一次
- **LLM推理**: 按需触发 (事件驱动)
- **Utility AI**: 每帧计算,但使用缓存的权重

---

## 🛠️ 配置最佳实践 / Configuration Best Practices

### 1. 设计性格时的思考流程

```
问题1: 这个NPC的核心特质是什么?
→ 确定OCEAN值

问题2: 这些特质如何影响行为?
→ 检查Maslow权重映射

问题3: 需要强制执行哪些规则?
→ 编写BehavioralGuidelines

问题4: LLM是否理解角色身份?
→ 编写RoleDescription
```

### 2. 常见错误 / Common Mistakes

❌ **错误1**: 所有OCEAN值都设为0.5 (中庸)
- **问题**: NPC没有个性,行为平淡
- **解决**: 至少有2-3个特质偏离0.5

❌ **错误2**: 在BehavioralGuidelines中使用数值
- **问题**: "Set Anger to 0.8" → LLM可能忽略
- **解决**: 使用语义标签 "Anger is Strong"

❌ **错误3**: 忘记Engine-Exclusive属性
- **问题**: LLM尝试修改Hunger → 被拒绝 → 困惑
- **解决**: 在Prompt中明确标注 "(Engine-Managed, DO NOT MODIFY)"

### 3. 测试检查清单 / Testing Checklist

- [ ] OCEAN值是否合理? (避免全0或全1)
- [ ] Maslow权重是否符合预期? (查看BeginPlay日志)
- [ ] LLM是否输出了正确的标签? (查看LLM Response)
- [ ] 权重调制是否生效? (对比不同性格的NPC)
- [ ] 行为是否符合角色设定? (观察实际动作)

---

## 📚 相关文档 / Related Documentation

- **实现指南**: `docs/guides/PersonalityComponent_Integration_Guide.md`
- **配置指南**: `docs/guides/PersonalityTable_Configuration_Guide.md`
- **测试指南**: `docs/guides/Testing_Guide.md`
- **架构设计**: `docs/design/LLM_Utility_Architecture.md`
- **工作流程**: `docs/design/Workflow_Personality_Sensory_LLM.md`

---

## 🔮 未来扩展方向 / Future Extensions

### 1. 动态性格演化
- OCEAN值随经历变化 (受伤→Neuroticism上升)
- 长期记忆影响性格 (被背叛→Agreeableness下降)

### 2. 社交网络影响
- NPC之间的性格互补/冲突
- 群体性格分布影响团队行为

### 3. 情境化权重
- 战斗中: Neuroticism权重×2
- 社交中: Agreeableness权重×2

---

**版本历史 / Version History:**
- v1.0 (2026-01-08): 初始版本,完整数据流文档
