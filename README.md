# 技术设计文档：基于 UE5 的 LLM-Utility AI 混合架构

**项目名称:** LLM-Driven NPC System
**引擎版本:** Unreal Engine 5.3+
**架构类型:** Stanford Agent Logic (Brain) + Utility AI (Body)
**最后更新:** 2025-1

---

## 1. 核心架构图解 (System Architecture)

本系统旨在解决传统 LLM Agent 响应慢、动作僵硬的问题，采用双层解耦设计：

1.  **上层 (Cognitive Layer):** 使用 LLM 处理非结构化数据（对话、环境感知），输出结构化的**情绪参数 (Parameters)**。
2.  **下层 (Action Layer):** 使用 Utility AI 系统，基于情绪参数实时计算最优动作，保证毫秒级响应。

**数据流向:**
`Perception (UE5)` -> `Memory Stream (Text Array)` -> `LLM (Brain)` -> `Update Parameters (JSON)` -> `Utility Scorer (Math)` -> `Action Execution`

---

## 2. 数据结构定义 (Data Structures)

### 2.1 精神状态黑板 (BP_NPC_MentalState)
这是一个继承自 `Object` 的纯数据蓝图，作为连接 LLM 和 Utility AI 的桥梁。

**变量列表 (全部为 Float, 范围 0.0 - 1.0):**

| 变量名 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `Anger` | 0.0 | 愤怒值。影响攻击欲望。 |
| `Fear` | 0.0 | 恐惧值。影响逃跑、躲藏欲望。 |
| `Social` | 0.5 | 社交欲望。影响主动对话频率。 |
| `Confidence` | 0.5 | 自信值。高自信倾向于正面对抗，低自信倾向于迂回。 |
| `Fatigue` | 0.0 | 疲劳值 (随时间Tick增加)。影响休息行为。 |

---

## 3. 模块实现详情 (Module Implementation)

### 3.1 身体：Utility AI 系统 (The Body)

**基类设计 (`BP_UtilityAction_Base`):**
所有行为必须继承此类，并在子类中重写以下函数。

* **Function: CalculateScore (Input: MentalState)**
    * *职责:* 根据传入的精神状态，返回当前动作的得分 (Float)。
* **Function: Execute (Input: AIController)**
    * *职责:* 执行具体的 UE5 逻辑 (MoveTo, PlayMontage)。
* **Function: Exit (Input: AIController)**
    * *职责:* 清理逻辑 (StopMovement, ResetFlags)。

**具体动作示例 (Concrete Actions):**

1.  **Action_Attack (攻击)**
    * `Score Formula`: `(Anger * 1.5) + (Confidence * 0.5) - (Fear * 2.0)`
    * `Execute`: 获取最近敌人 -> `AIController.MoveToActor` -> 距离够近则播放攻击 Montage。

2.  **Action_Flee (逃跑)**
    * `Score Formula`: `Fear * 2.0`
    * `Execute`: 获取反向向量 -> `GetRandomReachablePointInRadius` -> 跑路。

3.  **Action_Chat (闲聊)**
    * `Score Formula`: `Social * (1.0 - Anger)`
    * `Execute`: 面向玩家 -> 触发 TTS (Text-to-Speech) 组件。

---

### 3.2 大脑：感知与 LLM 接口 (The Brain)

**感知系统 (`AIPerception`):**
* **配置:** 启用 `Sight` (视觉) 和 `Hearing` (听觉)。
* **逻辑:** * 当 `OnTargetPerceptionUpdated` 触发 -> 将感知对象转化为文本 (例如 "Saw Player holding a Gun") -> 存入 **Memory Stream**。

**记忆流 (`ShortTermMemories` - Array of String):**
* 维护一个长度为 10-15 的字符串数组（FIFO 队列）。
* 存储最近发生的对话和感知事件。

**LLM 通信 (HTTP Request):**
* **触发时机:** 1.  记忆队列累计更新了 X 次。
    2.  发生重大事件 (如受到伤害 `Event AnyDamage`)。
* **Prompt 模板:**
    ```text
    Role: You are an NPC in a survival game.
    Current State: { Anger: 0.1, Fear: 0.2, ... }
    Recent Memories:
    - [Time 10:01] Saw Player1.
    - [Time 10:02] Player1 said: "Get out of here or I'll shoot!"
    
    Task: Analyze the memories. Update your internal state variables based on the situation.
    Output: ONLY a JSON object.
    Example: { "Anger": 0.5, "Fear": 0.8 }
    ```
* **解析:** 使用 VaRest 或类似插件解析返回的 JSON，覆盖 `BP_NPC_MentalState` 中的变量。

---

## 4. 核心逻辑循环 (The Core Loop)

在 `BP_AIController` 中实现以下逻辑：

### 4.1 思考循环 (Thinking Loop) - 每 0.2 ~ 0.5 秒
1.  **初始化:** `BestScore = -1`, `BestAction = Null`.
2.  **遍历:** 循环 `AvailableActions` 数组中的每一个动作实例。
3.  **打分:** 调用 `Action->CalculateScore(CurrentMentalState)`.
4.  **惯性处理 (Hysteresis):** * `IF (Action == CurrentAction) Score += 0.1` (防止动作频繁抖动)。
5.  **比较:** 记录最高分的动作。
6.  **切换:** * `IF (BestAction != CurrentAction)`:
        * `CurrentAction->Exit()`
        * `BestAction->Execute()`
        * `CurrentAction = BestAction`

### 4.2 掩盖延迟 (Latency Masking) - 优化体验
LLM 响应通常有 1-2 秒延迟，为防止 NPC 发呆：
1.  **请求发出时:** 不阻断当前 Utility AI 的运行，NPC 继续基于“旧参数”行动。
2.  **可选:** 如果是因为对话触发请求，强制 Utility AI 切换到临时动作 `Action_Thinking` (播放疑惑动画)，直到 JSON 返回。

---

## 5. 开发步骤清单 (Checklist)

- [ ] **Step 1:** 创建 `BP_NPC_MentalState` 并定义好情绪变量。
- [ ] **Step 2:** 创建 `BP_UtilityAction_Base` 及其子类 (Attack, Flee, Idle)。
- [ ] **Step 3:** 在 AI Controller 中实现“评分-选择-执行”循环，先用手动修改变量的方式测试 Utility AI 是否工作。
- [ ] **Step 4:** 配置 `AIPerception`，将视觉/听觉转化为文本存入数组。
- [ ] **Step 5:** 接入 HTTP 插件 (VaRest/Ollama)，发送 Prompt 并解析 JSON 回传给 MentalState。
- [ ] **Step 6:** 调试与参数微调 (Tuning Response Curves)。
