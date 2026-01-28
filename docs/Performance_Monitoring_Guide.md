# AINPC性能监控指南

## 🎯 快速监控（游戏内）

### 1. 基础FPS和帧时间
在游戏中按 `~` 打开控制台，输入：

```
stat fps          # 显示FPS
stat unit         # 显示帧时间分解
                  # - Frame: 总帧时间（目标<16.67ms for 60fps）
                  # - Game:  游戏逻辑线程
                  # - Draw:  渲染准备
                  # - GPU:   GPU渲染时间
```

### 2. AI系统性能
```
stat ai           # AI系统详细信息
stat game         # 游戏线程详细统计
                  # 包含我们自定义的 "Cognition - Lazy Fetch"
```

### 3. 查看Lazy Fetch性能
```
stat game
# 在列表中找到：
# - Cognition - Lazy Fetch: X.XXms
```

**预期值**：
- 单次调用：< 0.01ms
- 即使100个NPC：< 1ms/秒

---

## 📊 详细分析（Unreal Insights）

### 启动方法1：命令行参数
```powershell
# 在启动游戏时添加
-trace=cpu,frame,log,bookmark
```

### 启动方法2：编辑器内
1. 打开 **Session Frontend**（Ctrl + Alt + F11）
2. 点击 **Profiler** 标签
3. 点击 **Start Capture**
4. 运行游戏测试
5. 点击 **Stop Capture**
6. 查看结果

### 启动方法3：独立工具
```powershell
# 启动Unreal Insights
D:\UnrealEngine5\UE_5.5\Engine\Binaries\Win64\UnrealInsights.exe

# 在游戏中连接
stat startfile
# ... 运行测试场景 ...
stat stopfile

# 在Insights中打开生成的.utrace文件
```

---

## 🔍 关键指标

### CPU性能
| 指标 | 好 | 警告 | 严重 |
|------|------|------|------|
| **Frame Time** | < 16ms | 16-33ms | > 33ms |
| **Game Thread** | < 10ms | 10-20ms | > 20ms |
| **Lazy Fetch/Frame** | < 0.1ms | 0.1-1ms | > 1ms |

### 内存
```
stat memory       # 内存使用
stat streaming    # 资源流送
```

### LLM相关
```
# 在Output Log中搜索：
"[Cognition]"     # 查看LLM请求频率
"Rate Limiting"   # 查看限流情况
```

---

## 🎮 测试场景

### 压力测试1：大量NPC
1. 创建50+ camp
2. 运行 `stat game`
3. 观察 "Cognition - Lazy Fetch" 的总时间
4. 预期：< 1ms/frame

### 压力测试2：战斗场景
1. 让10个NPC同时战斗
2. 运行 `stat ai`
3. 观察AI更新频率
4. 预期：LLM请求被限流到1.5秒/次

### 压力测试3：玩家附近
1. 站在多个camp中间
2. 观察FPS是否下降
3. 用 `stat unit` 确定瓶颈（Game/Draw/GPU）

---

## 💡 性能优化建议

### 如果Game Thread过高（>15ms）
1. **检查LLM请求频率**：
   ```
   # 增加冷却时间
   float Cooldown = bIsHighPriority ? 2.0f : 8.0f;
   ```

2. **启用距离剔除**：
   ```cpp
   float DistToPlayer = FVector::Dist(GetActorLocation(), PlayerLocation);
   if (DistToPlayer > 5000.0f) return; // 跳过远处NPC
   ```

3. **禁用Mindless NPC的LLM**：
   ```cpp
   bEnableReasoning = false; // 僵尸/动物不需要LLM
   ```

### 如果GPU过高（>16ms）
- 不是AI系统的问题
- 检查光照/阴影/后处理

### 如果Draw过高（>5ms）
- 减少视野内NPC数量
- 使用LOD系统

---

## 📝 性能日志示例

### 正常情况（44个camps，132个NPCs）：
```
Frame: 12.5ms (80fps)
├─ Game:  8.2ms
│  ├─ AI Update: 3.1ms
│  ├─ Cognition - Lazy Fetch: 0.08ms  ← 完全可以忽略！
│  └─ Physics: 2.4ms
├─ Draw:  2.8ms
└─ GPU:   9.4ms
```

### 异常情况（需要优化）：
```
Frame: 45.2ms (22fps)  ← 卡了！
├─ Game:  38.1ms       ← 瓶颈在这里
│  ├─ AI Update: 32.5ms  ← AI太慢
│  │  └─ LLM Requests: 28.2ms  ← 太多LLM请求
│  └─ Cognition - Lazy Fetch: 0.15ms  ← 依然很小
```

---

## ✅ 总结

**Lazy Fetch的性能影响几乎可以忽略**：
- 单次调用：< 0.01ms
- 100个NPC：< 1ms/frame
- 占总帧时间：< 0.1%

**真正的性能瓶颈**：
1. LLM网络请求（100-500ms）
2. JSON解析（1-5ms）
3. 感知系统（如果没有优化）

**当前系统已经很高效**，因为：
✅ Rate Limiting（1.5-4秒冷却）
✅ Mindless NPC模式
✅ Context LOD系统
