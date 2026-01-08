# AI Perception 架构说明 / AI Perception Architecture

## 核心概念 / Core Concepts

AI Perception 系统有两个角色：

### 1. Observer（观察者）- Controller 端
- **组件**：`AIPerceptionComponent`
- **位置**：在 `UtilityAIController` 上
- **作用**：让 AI 能够感知周围的 Actor
- **类比**：眼睛和耳朵

### 2. Stimuli Source（刺激源）- Pawn 端
- **组件**：`AIPerceptionStimuliSourceComponent`
- **位置**：在 `BP_Zombie`（Pawn）上
- **作用**：让这个 Pawn 能够被其他 AI 感知到
- **类比**：身体的可见性和声音

---

## 为什么需要两个组件？

### 类比：现实世界

```
你（Observer）：
  └── 眼睛 (AIPerceptionComponent) → 能看到别人

桌子（Stimuli Source）：
  └── 反射光线 (AIPerceptionStimuliSource) → 能被别人看到
```

**如果桌子不反射光线，即使你有眼睛也看不到它！**

### 在 Unreal 中

```
Controller (UtilityAIController):
  └── AIPerceptionComponent
      ├── SightConfig (视觉配置)
      ├── HearingConfig (听觉配置)
      └── "我能感知周围的 Actor"

Pawn (BP_Zombie):
  └── AIPerceptionStimuliSource
      ├── Register for Sight (注册为视觉刺激源)
      ├── Register for Hearing (注册为听觉刺激源)
      └── "其他 AI 能感知到我"
```

---

## 当前状态 / Current Status

### ✅ 已配置（Controller 端）

在 `UtilityAIController.cpp` 中：

```cpp
// 创建 AIPerception
AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

// 配置视觉
SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
SightConfig->SightRadius = 1500.0f;
SightConfig->DetectionByAffiliation.bDetectEnemies = true;
SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
AIPerception->ConfigureSense(*SightConfig);

// 配置听觉
HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
HearingConfig->HearingRange = 2500.0f;
HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
AIPerception->ConfigureSense(*HearingConfig);
```

**结果**：✅ AI 有"眼睛"和"耳朵"，能够感知周围

---

### ❌ 缺少配置（Pawn 端）

在 `BP_Zombie` 中：

**当前状态**：❌ 没有 `AIPerceptionStimuliSource` 组件

**结果**：❌ 这个 Pawn 不会被其他 AI 感知到（即使其他 AI 有 AIPerceptionComponent）

---

## 解决方案 / Solution

### 方法 1: 在 Blueprint 中添加（推荐）

1. 打开 `BP_Zombie`
2. 点击 **Add Component**
3. 搜索 **AI Perception Stimuli Source**
4. 添加后，在 Details 面板中配置：
   ```
   AI Perception Stimuli Source:
   ├── Auto Register as Source: ✅
   └── Register as Source for Senses:
       ├── AISense_Sight: ✅
       └── AISense_Hearing: ✅
   ```

### 方法 2: 在 C++ 中自动添加

创建一个 `AICharacterBase` 类，自动为所有 NPC 添加这个组件：

```cpp
// AICharacterBase.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AICharacterBase.generated.h"

class UAIPerceptionStimuliSourceComponent;

UCLASS()
class AINPC_API AAICharacterBase : public ACharacter
{
    GENERATED_BODY()
    
public:
    AAICharacterBase();
    
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionStimuliSourceComponent* StimuliSource;
};

// AICharacterBase.cpp
#include "AICharacterBase.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

AAICharacterBase::AAICharacterBase()
{
    // 创建 Stimuli Source 组件
    StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
    
    // 注册为视觉和听觉的刺激源
    StimuliSource->RegisterForSense(TSubclassOf<UAISense_Sight>());
    StimuliSource->RegisterForSense(TSubclassOf<UAISense_Hearing>());
    StimuliSource->RegisterWithPerceptionSystem();
}
```

然后让 `BP_Zombie` 继承 `AICharacterBase`。

---

## 数据流 / Data Flow

### 完整的感知流程

```
场景中有两个 NPC：Zombie A 和 Zombie B

1. Zombie B 的 Pawn 有 StimuliSource
   └── 向 AI Perception System 注册："我在这里，位置是 (X, Y, Z)"

2. Zombie A 的 Controller 有 AIPerceptionComponent
   └── 每帧查询 AI Perception System："我的视野范围内有什么？"

3. AI Perception System 计算
   └── "Zombie B 在 Zombie A 的视野范围内，且没有遮挡"

4. AIPerceptionComponent 触发事件
   └── OnTargetPerceptionUpdated.Broadcast(Zombie B, Stimulus)

5. SensoryComponent 接收事件
   └── HandleTargetPerceived(Zombie B, Stimulus)

6. SensoryComponent 翻译成文字
   └── "I saw BP_Zombie_C_1"

7. 广播给 CognitionComponent
   └── ProcessStimulus("I saw BP_Zombie_C_1")

8. LLM 处理
   └── 生成情绪和行为反应
```

---

## 调试技巧 / Debugging Tips

### 1. 可视化感知范围

运行游戏后，按 `'` 键打开控制台，输入：
```
ShowDebug AI
```

你应该能看到：
- **绿色锥体**：视觉范围
- **黄色圆圈**：听觉范围

### 2. 检查日志

如果看到这个警告：
```
LogAIPerception: Warning: No stimuli source registered for BP_Zombie_C_0
```

说明 Pawn 没有 `AIPerceptionStimuliSource` 组件。

### 3. 检查组件是否正确注册

在 Blueprint 中，选中 `AIPerceptionStimuliSource` 组件，查看 Details 面板：
- `Auto Register as Source` 必须勾选 ✅
- `Register as Source for Senses` 必须包含 `AISense_Sight` ✅

---

## 总结 / Summary

| 组件 | 位置 | 作用 | 状态 |
|------|------|------|------|
| `AIPerceptionComponent` | Controller | 能感知别人 | ✅ 已配置 |
| `AIPerceptionStimuliSource` | Pawn | 能被别人感知 | ❌ 需要添加 |

**结论**：你的 Controller 已经有"眼睛"了，但是 Pawn 需要"可见性"才能被其他 AI 看到！

