# LLM-Utility AI 协作机制说明
# LLM-Utility AI Collaboration Mechanism

**日期 / Date:** 2026-01-09  
**版本 / Version:** 2.0

---

## 1. 核心问题 / Core Problem

**问题描述：**
- LLM 生成的 `Speech`（对话）和 Utility AI 选择的 `Action`（行为）不一致
- 例如：NPC 说 "Hello friend!" 但同时在逃跑

**根本原因：**
- LLM 和 Utility AI 是两个独立的决策系统，没有协调机制

---

## 2. 解决方案：软性意图引导 / Solution: Soft Intention Guidance

### 设计理念 / Design Philosophy

**LLM (慢系统/认知层) - "大脑"**
- 角色：提供**高层次的意图建议**
- 输出：`Intention` 字段（例如："Attack", "Flee", "Idle"）
- 性质：**建议而非命令** - 表达"我觉得应该..."

**Utility AI (快系统/执行层) - "脊髓"**
- 角色：做**最终决策**
- 考虑因素：
  1. 环境可行性（目标是否存在、距离、障碍物等）
  2. 当前状态（Health, Stamina 等）
  3. **LLM 的意图建议**（作为加分项）
- 性质：**实用主义** - 选择当前最可行的行动

### 协作机制 / Collaboration Mechanism

```
LLM Intention: "Attack" (+0.3 bonus)
                  ↓
Utility AI 评分:
  - Test_Attack:  0.6 (base) + 0.3 (intention) = 0.9 ✅ 选中
  - Test_Flee:    0.4 (base) + 0.0 (no match)  = 0.4
  - Test_Idle:    0.2 (base) + 0.0 (no match)  = 0.2
```

**关键点：**
- Intention 加成是 **+0.3**，不是强制执行
- 如果环境不允许（例如敌人跑远了），其他 Action 依然可能胜出

---

## 3. 实现细节 / Implementation Details

### 3.1 数据流 / Data Flow

```
1. Sensory → 感知到刺激（例如：看到敌人）
2. Memory → 存储事件
3. Cognition → 调用 LLM
4. LLM 返回：
   {
     "Intention": "Attack",
     "Speech": "You'll pay for this!",
     "Emotion": "Angry",
     "Anger": "Furious",
     ...
   }
5. Utility AI 评分：
   - 读取 MentalState.Intention
   - 如果 Action 名称包含 "Attack" → +0.3 分
   - 选择最高分的 Action
6. 执行 Action + 显示 Speech
```

### 3.2 代码位置 / Code Locations

**1. Intention 字段定义：**
- 文件：`LLMCommunicator.h`
- 结构体：`FMentalState`
- 字段：`FString Intention`

**2. LLM Prompt 更新：**
- 文件：`CognitionComponent.cpp`
- 函数：`ProcessStimulus()`
- 新增：`=== INTENTION ===` 部分

**3. Utility AI 加成逻辑：**
- 文件：`UtilityAIComponent.cpp`
- 函数：`EvaluateAndDecide()`
- 位置：在 Inertia Bonus 之后

---

## 4. 优势 / Advantages

✅ **保留 Utility AI 的价值**
- 依然负责最终决策
- 考虑环境约束和可行性

✅ **LLM 提供语义指导**
- Speech 会匹配 Intention
- 提供高层次的行为倾向

✅ **允许合理的不一致**
- 例如：LLM 说 "I'll attack!"（Intention: Attack）
- 但敌人跑了，Utility AI 选择 Idle
- 结果：NPC 显得"想打但打不到"，这很自然

✅ **性能优化**
- LLM 不需要了解所有环境细节
- Utility AI 处理实时环境判断

---

## 5. 配置参数 / Configuration Parameters

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `IntentionBonus` | 0.3 | Intention 匹配时的加分 |
| `InertiaBonus` | 0.1 | 当前 Action 的惯性加分 |

**调整建议：**
- 如果希望 LLM 意图更强势：增加 `IntentionBonus` 到 0.5
- 如果希望 Utility AI 更独立：减少到 0.1

---

## 6. 示例场景 / Example Scenarios

### 场景 A：完全一致
- **刺激**：看到敌人
- **LLM**：Intention="Attack", Speech="Die!"
- **Utility AI**：Test_Attack 得分最高（0.8 + 0.3 = 1.1）
- **结果**：✅ 攻击 + 说 "Die!"

### 场景 B：合理偏离
- **刺激**：看到敌人，但 Health 很低
- **LLM**：Intention="Attack", Speech="I'll fight!"
- **Utility AI**：Test_Flee 得分最高（0.9 > 0.6+0.3）
- **结果**：✅ 逃跑 + 说 "I'll fight!"（显得"嘴硬"，很自然）

### 场景 C：环境约束
- **刺激**：敌人跑远了
- **LLM**：Intention="Attack", Speech="Come back!"
- **Utility AI**：Test_Idle 得分最高（目标丢失）
- **结果**：✅ 站立 + 说 "Come back!"（显得"无奈"）

---

## 7. 未来扩展 / Future Extensions

### 可能的改进：
1. **动态调整加成**：根据 Personality 调整 IntentionBonus
   - 冲动型性格：IntentionBonus = 0.5（更听从 LLM）
   - 理性型性格：IntentionBonus = 0.1（更依赖环境判断）

2. **Intention 优先级**：
   - 某些 Intention（如 "Flee"）可能需要更高优先级

3. **多意图支持**：
   - LLM 返回主次意图：`Primary: "Attack", Secondary: "Investigate"`

---

## 8. 总结 / Summary

这个设计实现了 **"LLM 提供智慧，Utility AI 提供执行力"** 的协作模式：

- **LLM**：理解情境，生成合理的意图和对话
- **Utility AI**：根据实际情况，选择最可行的行动
- **协调机制**：Intention 加成确保两者大部分时候一致，但允许合理偏离

这样既保留了 LLM 的语义理解能力，又发挥了 Utility AI 的实时决策优势。
