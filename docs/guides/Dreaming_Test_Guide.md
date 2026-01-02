# 🧪 Dreaming 功能测试指南

## ✅ 已完成的修改

### 1. 头文件修改 (`UtilityAIController.h`)
```cpp
private:
    // Dreaming 定时器：定期整理记忆，提取长期洞察
    FTimerHandle DreamingTimerHandle;
```

### 2. 实现文件修改 (`UtilityAIController.cpp`)
```cpp
// 在 BeginPlay() 末尾添加
if (CognitionComp)
{
    GetWorldTimerManager().SetTimer(
        DreamingTimerHandle,
        [this]() 
        { 
            if (CognitionComp)
            {
                CognitionComp->StartDreaming();
                UE_LOG(LogTemp, Log, TEXT("[%s] Dreaming cycle triggered..."), *GetName());
            }
        },
        300.0f,  // 每5分钟
        true     // 循环执行
    );
}
```

---

## 🔍 功能说明

### Dreaming 机制是什么？

**Dreaming** 是一个记忆整理系统，模拟人类睡眠时的记忆巩固过程：

1. **收集短期记忆** - AI在游戏中经历的所有事件
2. **发送给LLM分析** - 让AI总结这些经历的意义
3. **提取长期洞察** - 例如："玩家很危险"、"食物稀缺"
4. **存入长期记忆** - 这些洞察会影响未来的决策

### 工作流程

```
每5分钟触发一次
    ↓
获取所有短期记忆 (MemoryComponent)
    ↓
构建总结Prompt
    ↓
发送给LLM (SendRequestRaw)
    ↓
解析JSON数组 ["洞察1", "洞察2"]
    ↓
存入长期记忆 (ConsolidateMemories)
```

---

## 🧪 测试步骤

### 方案A: 快速测试 (修改定时器间隔)

**临时修改** `UtilityAIController.cpp` 中的定时器间隔：

```cpp
GetWorldTimerManager().SetTimer(
    DreamingTimerHandle,
    [this]() { /* ... */ },
    30.0f,   // ✅ 改为30秒，方便测试
    true
);
```

### 方案B: 手动触发测试

**添加蓝图可调用函数** (可选):

在 `UtilityAIController.h` 中添加：
```cpp
// 供调试使用：手动触发Dreaming
UFUNCTION(BlueprintCallable, Category = "AI Debug")
void TriggerDreamingNow();
```

在 `UtilityAIController.cpp` 中实现：
```cpp
void AUtilityAIController::TriggerDreamingNow()
{
    if (CognitionComp)
    {
        CognitionComp->StartDreaming();
        UE_LOG(LogTemp, Warning, TEXT("[DEBUG] Manual Dreaming triggered!"));
    }
}
```

然后在蓝图中调用这个函数即可立即测试。

---

## 📊 预期日志输出

### 1. 初始化日志
```
LogTemp: [BP_AIController_C_0] Dreaming timer initialized (interval: 5 minutes)
```

### 2. Dreaming触发日志
```
LogTemp: [BP_AIController_C_0] Dreaming cycle triggered - consolidating memories...
LogTemp: [Dreaming] Sending logs to LLM...
```

### 3. LLM响应日志
```
LogTemp: [LLM Raw] Success: ["Player is hostile", "Food is scarce", "Need to find shelter"]
```

### 4. 记忆整合日志
```
LogTemp: [Memory] Consolidated 3 long-term insights
```

---

## ✅ 验证清单

运行游戏后，检查以下内容：

- [ ] **编译成功** - 无错误和警告
- [ ] **初始化日志** - 看到 "Dreaming timer initialized"
- [ ] **定时器触发** - 等待5分钟（或30秒）后看到 "Dreaming cycle triggered"
- [ ] **LLM请求** - 看到 "Sending logs to LLM"
- [ ] **记忆整合** - 看到 "Consolidated X long-term insights"

---

## 🐛 可能的问题

### 问题1: 没有看到 "Dreaming cycle triggered"

**原因:**
- 定时器未启动
- CognitionComp 为 nullptr

**解决:**
```cpp
// 在 BeginPlay() 中添加调试日志
if (CognitionComp)
{
    UE_LOG(LogTemp, Warning, TEXT("[DEBUG] CognitionComp is valid, setting up timer..."));
    GetWorldTimerManager().SetTimer(...);
}
else
{
    UE_LOG(LogTemp, Error, TEXT("[DEBUG] CognitionComp is NULL!"));
}
```

### 问题2: "Sending logs to LLM" 但没有响应

**原因:**
- 短期记忆为空
- LLM API 调用失败

**解决:**
1. 检查是否有短期记忆：
```cpp
// 在 StartDreaming() 中添加
FString DailyLogs = MemoryComp->GetAllRecentMemoriesAsString();
UE_LOG(LogTemp, Warning, TEXT("[Dreaming] Memory count: %d chars"), DailyLogs.Len());
```

2. 检查 LLM API 配置是否正确

### 问题3: "Callback not found for request"

**原因:**
- 你还没有修复 Bug #4 (并发请求冲突)

**解决:**
- 参考 `BugFixes_Patch.md` 中的修复 #4

---

## 🎯 优化建议

### 1. 动态调整间隔

根据记忆数量动态调整Dreaming频率：

```cpp
// 记忆少时：每10分钟
// 记忆多时：每3分钟
float Interval = (MemoryCount < 10) ? 600.0f : 180.0f;
```

### 2. 添加记忆阈值

只有当记忆数量达到一定阈值时才触发：

```cpp
void UCognitionComponent::StartDreaming()
{
    if (!MemoryComp) return;
    
    int32 MemoryCount = MemoryComp->GetMemoryCount();
    if (MemoryCount < 5)
    {
        UE_LOG(LogTemp, Log, TEXT("[Dreaming] Not enough memories (%d), skipping..."), MemoryCount);
        return;
    }
    
    // ... 继续执行
}
```

### 3. 保存Dreaming结果

将长期洞察保存到文件，重启游戏后可以加载：

```cpp
// 在 GameInstance 或 SaveGame 中保存
TArray<FString> LongTermInsights;
```

---

## 📝 测试场景示例

### 场景1: 敌对玩家
1. 玩家靠近AI
2. 玩家攻击AI
3. AI逃跑
4. 等待5分钟
5. **预期洞察:** "Player is dangerous", "Avoid confrontation"

### 场景2: 友好互动
1. 玩家给AI物品
2. 玩家和AI对话
3. AI跟随玩家
4. 等待5分钟
5. **预期洞察:** "Player is friendly", "Can trust player"

---

## 🔧 调试命令

在UE编辑器控制台中使用：

```
// 查看所有定时器
stat timers

// 查看AI状态
showdebug ai

// 手动触发Dreaming (如果添加了蓝图函数)
ke * TriggerDreamingNow
```

---

## 📊 性能监控

Dreaming 是一个**异步操作**，不会阻塞游戏：

- **LLM请求:** 异步HTTP，1-3秒
- **记忆检索:** < 1ms
- **JSON解析:** < 1ms
- **总影响:** 几乎为0

---

## ✅ 完成标志

当你看到以下日志时，说明Dreaming功能已完全启用：

```
[BP_AIController_C_0] Dreaming timer initialized (interval: 5 minutes)
[BP_AIController_C_0] Dreaming cycle triggered - consolidating memories...
[Dreaming] Sending logs to LLM...
[LLM Raw] Success: ["Insight 1", "Insight 2", "Insight 3"]
[Memory] Consolidated 3 long-term insights
```

---

**测试时间:** 建议先用30秒间隔测试，确认功能正常后再改回5分钟

**下一步:** 修复 Bug #4 (并发请求冲突) 以确保Dreaming和正常感知不会互相干扰
