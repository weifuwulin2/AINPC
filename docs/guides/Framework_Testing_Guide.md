# 🧪 框架测试指南

**目标:** 使用虚假 Action 测试整个 LLM + Utility AI 框架  
**测试范围:** 感知 → LLM → 情绪更新 → Utility AI → 动作执行

---

## 📋 测试准备

### 1. 已创建的测试 Action

我已经为你创建了 3 个测试用的 Action 类：

```
Source/AINPC/Public/Test/
├── TestAction_Attack.h
├── TestAction_Flee.h
└── TestAction_Idle.h

Source/AINPC/Private/Test/
├── TestAction_Attack.cpp
├── TestAction_Flee.cpp
└── TestAction_Idle.cpp
```

---

## 🎯 测试 Action 说明

### TestAction_Attack (攻击)
```cpp
特点:
- 执行时间: 2秒
- 日志级别: Warning (黄色)
- 用途: 测试高 Anger 时的行为

预期触发条件:
- Anger > 0.5
- Confidence > 0.3
```

### TestAction_Flee (逃跑)
```cpp
特点:
- 执行时间: 1.5秒
- 日志级别: Error (红色，突出显示)
- 用途: 测试高 Fear 时的紧急行为

预期触发条件:
- Fear > 0.7
- SelfHealth < 0.3 (需要你实现)
```

### TestAction_Idle (待机)
```cpp
特点:
- 永不完成（除非被打断）
- 日志级别: Display (白色)
- 用途: 作为默认动作

预期触发条件:
- 所有其他动作分数都很低时
```

---

## 📝 步骤1: 编译项目

### 添加到项目

1. **重新生成项目文件**
   ```bash
   # 右键 .uproject → Generate Visual Studio project files
   ```

2. **编译**
   ```bash
   # 在 Visual Studio 中
   Build → Build Solution (Ctrl+Shift+B)
   ```

3. **验证编译成功**
   ```
   查看输出窗口:
   ========== Build: 1 succeeded, 0 failed ==========
   ```

---

## 📝 步骤2: 创建 DataTable

### 2.1 创建 DataTable 资源

1. 在 Content Browser 中右键
2. 选择 `Miscellaneous` → `Data Table`
3. 选择 Row Structure: `UtilityActionConfig`
4. 命名: `DT_TestActions`

### 2.2 配置 Attack 动作

```
Row Name: Attack

ActionClass: TestAction_Attack

Considerations:
┌─────────────────────────────────────┐
│ Consideration 0:                    │
│   InputType: Anger                  │
│   ResponseCurve: (创建新曲线)       │
│     名称: Curve_Anger_Attack        │
│     配置:                           │
│       X=0.0 → Y=0.0                │
│       X=0.3 → Y=0.0                │
│       X=0.5 → Y=1.0                │
│       X=1.0 → Y=1.0                │
├─────────────────────────────────────┤
│ Consideration 1:                    │
│   InputType: Confidence             │
│   ResponseCurve: (创建新曲线)       │
│     名称: Curve_Confidence_Attack   │
│     配置:                           │
│       X=0.0 → Y=0.3                │
│       X=0.5 → Y=0.6                │
│       X=1.0 → Y=1.0                │
└─────────────────────────────────────┘

Weight: 1.5
CooldownTime: 0.0
InertiaBonus: 0.1
```

### 2.3 配置 Flee 动作

```
Row Name: Flee

ActionClass: TestAction_Flee

Considerations:
┌─────────────────────────────────────┐
│ Consideration 0:                    │
│   InputType: Fear                   │
│   ResponseCurve: (创建新曲线)       │
│     名称: Curve_Fear_Flee           │
│     配置:                           │
│       X=0.0 → Y=0.0                │
│       X=0.5 → Y=0.0                │
│       X=0.7 → Y=1.0                │
│       X=1.0 → Y=1.0                │
└─────────────────────────────────────┘

Weight: 2.0 (高优先级)
CooldownTime: 0.0
InertiaBonus: 0.0
```

### 2.4 配置 Idle 动作

```
Row Name: Idle

ActionClass: TestAction_Idle

Considerations:
(空 - 没有条件，总是可用)

Weight: 0.8 (低优先级)
CooldownTime: 0.0
InertiaBonus: 0.0
```

---

## 📝 步骤3: 配置 AI Controller

### 3.1 在蓝图中设置 DataTable

1. 打开你的 AI Controller 蓝图
2. 找到 `UtilityAIComponent`
3. 设置 `Action Config Table` = `DT_TestActions`

---

## 📝 步骤4: 运行测试

### 4.1 启动游戏

1. 点击 Play
2. 打开 Output Log (Window → Developer Tools → Output Log)
3. 过滤器设置为 `LogTemp`

### 4.2 预期日志输出

#### 初始化阶段
```
[Cognition] Brain Initialized via Config.
[UtilityAI] Dreaming timer initialized: 300.0s interval
[UtilityAI] Loaded 3 actions from DataTable
  - Test_Attack
  - Test_Flee
  - Test_Idle
```

#### 默认行为（Idle）
```
───────────────────────────────────────
[TEST] Idle Action ENTERED (Count: 1)
───────────────────────────────────────
[Idle] Waiting... Time: 2.0s
[Idle] Waiting... Time: 4.0s
```

---

## 📝 步骤5: 触发感知事件

### 5.1 手动触发（蓝图）

创建一个测试蓝图：

```
Event BeginPlay
  ↓
Delay 3.0s
  ↓
Get AI Controller
  ↓
Get Sensory Component
  ↓
Call: OnStimulusDetected
  - Description: "Saw enemy player attacking"
```

### 5.2 预期数据流

```
1. 感知事件触发
   ↓
   [Sensory] Stimulus: "Saw enemy player attacking"
   
2. 传递给认知组件
   ↓
   [Cognition] Processing stimulus...
   
3. LLM 请求
   ↓
   [LLM] Request Sent (ID: 0x...)
   [LLM] Prompt includes: Anger, Fear, Confidence...
   
4. LLM 响应
   ↓
   [LLM] Success! Parsed: Anger=0.80, Fear=0.20
   
5. 情绪更新
   ↓
   [Controller] Mental State Updated: Anger=0.80, Fear=0.20
   
6. Utility AI 评分
   ↓
   [UtilityAI] Evaluating actions...
   [Attack] BaseWeight=1.50
     - Anger: Raw=0.80, Curve=1.00
     - Confidence: Raw=0.50, Curve=0.60
     → FinalScore=0.90
   [Flee] BaseWeight=2.00
     - Fear: Raw=0.20, Curve=0.00
     → FinalScore=0.00
   [Idle] BaseWeight=0.80
     → FinalScore=0.80
   
7. 选择动作
   ↓
   [UtilityAI] Selected: Test_Attack (0.90)
   
8. 执行动作
   ↓
   ═══════════════════════════════════════
   [TEST] Attack Action ENTERED
   ═══════════════════════════════════════
   [Attack] Executing... Time: 1.0s
   [Attack] Executing... Time: 2.0s
   [Attack] Completed after 2.0s
   ═══════════════════════════════════════
   [TEST] Attack Action EXITED
   ═══════════════════════════════════════
```

---

## 🧪 测试场景

### 场景1: 测试愤怒 → 攻击

**触发:**
```
OnStimulusDetected("Player insulted me")
```

**预期LLM响应:**
```json
{
  "Anger": 0.8,
  "Fear": 0.1,
  "Confidence": 0.6
}
```

**预期行为:**
```
Attack 分数 = 1.5 × 1.0 × 0.6 = 0.9
Flee 分数 = 2.0 × 0.0 = 0.0
Idle 分数 = 0.8

→ 选择 Attack ✅
```

---

### 场景2: 测试恐惧 → 逃跑

**触发:**
```
OnStimulusDetected("Saw a huge monster")
```

**预期LLM响应:**
```json
{
  "Anger": 0.2,
  "Fear": 0.9,
  "Confidence": 0.1
}
```

**预期行为:**
```
Attack 分数 = 1.5 × 0.0 × 0.3 = 0.0
Flee 分数 = 2.0 × 1.0 = 2.0
Idle 分数 = 0.8

→ 选择 Flee ✅
```

---

### 场景3: 测试中性 → 待机

**触发:**
```
OnStimulusDetected("Nothing interesting")
```

**预期LLM响应:**
```json
{
  "Anger": 0.1,
  "Fear": 0.1,
  "Confidence": 0.5
}
```

**预期行为:**
```
Attack 分数 = 1.5 × 0.0 × 0.6 = 0.0
Flee 分数 = 2.0 × 0.0 = 0.0
Idle 分数 = 0.8

→ 选择 Idle ✅
```

---

### 场景4: 测试 Dreaming

**等待 5 分钟（或修改定时器为30秒）**

**预期日志:**
```
[Controller] Dreaming cycle triggered
[Cognition] Starting Dreaming...
[Cognition] Recent memories: 3 items
[LLM Raw] Request Sent (ID: 0x...)
[LLM Raw] Success: ["Player is hostile", "Environment is dangerous"]
[Memory] Consolidated 2 long-term insights
```

---

## ✅ 验证清单

### 编译阶段
- [ ] 项目编译成功
- [ ] 无错误和警告
- [ ] 3个测试Action类正确加载

### 配置阶段
- [ ] DataTable 创建成功
- [ ] 3个Action配置完成
- [ ] ResponseCurve 创建并配置

### 运行阶段
- [ ] AI Controller 初始化成功
- [ ] Dreaming 定时器启动
- [ ] 默认执行 Idle 动作

### 功能测试
- [ ] 感知事件能触发
- [ ] LLM 请求发送成功
- [ ] LLM 响应解析正确
- [ ] 情绪状态更新
- [ ] Utility AI 评分正确
- [ ] 动作切换正常
- [ ] 日志输出完整

### 场景测试
- [ ] 场景1: 愤怒 → Attack
- [ ] 场景2: 恐惧 → Flee
- [ ] 场景3: 中性 → Idle
- [ ] 场景4: Dreaming 触发

---

## 🐛 常见问题

### 问题1: Action 类找不到

**症状:**
```
Error: Class 'TestAction_Attack' not found
```

**解决:**
1. 确认文件在正确的文件夹
2. 重新生成项目文件
3. 重新编译

---

### 问题2: DataTable 无法选择 Action

**症状:**
DataTable 中 ActionClass 下拉列表为空

**解决:**
1. 确认编译成功
2. 重启编辑器
3. 检查类是否标记为 `Blueprintable`

---

### 问题3: LLM 不返回数据

**症状:**
```
[LLM] API Error Code: 401
```

**解决:**
1. 检查 `DefaultGame.ini` 中的 API Key
2. 验证网络连接
3. 查看 LLM 日志

---

### 问题4: 动作不切换

**症状:**
AI 一直执行 Idle

**解决:**
1. 检查 ResponseCurve 配置
2. 添加日志查看分数
3. 验证 BaseWeight 设置

---

## 📊 性能监控

### 添加性能日志

在 `UtilityAIComponent::Tick()` 中：

```cpp
double StartTime = FPlatformTime::Seconds();

// ... 评分逻辑 ...

double EndTime = FPlatformTime::Seconds();
double ElapsedMs = (EndTime - StartTime) * 1000.0;

UE_LOG(LogTemp, Verbose, TEXT("[Performance] Utility AI evaluation: %.2fms"), ElapsedMs);
```

**预期性能:**
- 评分计算: < 1ms
- LLM 请求: 500-2000ms (异步)
- 总体影响: 可忽略

---

## 🎉 测试成功标志

当你看到以下完整流程时，说明框架工作正常：

```
1. ✅ 感知事件触发
2. ✅ LLM 请求发送
3. ✅ LLM 响应解析
4. ✅ 情绪状态更新
5. ✅ Utility AI 评分
6. ✅ 动作正确选择
7. ✅ 动作正确执行
8. ✅ Dreaming 定期触发
```

---

## 📁 相关文件

- `TestAction_*.h/cpp` - 测试Action类
- `DT_TestActions` - DataTable配置
- `Utility_AI_Configuration_Guide.md` - 配置指南
- `Single_Point_Configuration_Guide.md` - 属性配置

---

**祝测试顺利！** 🚀

有问题随时问我！
