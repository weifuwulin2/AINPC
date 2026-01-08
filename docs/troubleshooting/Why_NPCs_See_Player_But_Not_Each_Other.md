# 为什么 NPC 能看到玩家，但看不到彼此？

## 你的疑问

> "为什么玩家不需要放这个（AIPerceptionStimuliSource）NPC 就可以看到玩家呢？"

这是个非常好的问题！让我解释一下可能的原因。

---

## 可能的原因

### 1. 玩家 Blueprint 已经配置了 `AIPerceptionStimuliSource`

最可能的情况是：你的玩家 Blueprint（例如 `BP_ThirdPersonCharacter` 或 `BP_CombatCharacter`）**已经添加了** `AIPerceptionStimuliSource` 组件。

**验证方法**：
1. 打开你的玩家 Blueprint
2. 查看 Components 面板
3. 看看是否有 `AIPerceptionStimuliSource` 组件

如果有，那就解释了为什么 NPC 能看到玩家！

---

### 2. 你使用了 `ActorHasTag("Player")` 的特殊处理

在 `SensoryComponent.cpp` 中，我看到了这段代码：

```cpp
FString USensoryComponent::FormatDescription(FString Verb, AActor* Target, FString ExtraInfo)
{
    FString TargetName = Target ? Target->GetName() : "Unknown";
    if (Target && Target->ActorHasTag("Player")) TargetName = "Player";  // ← 这里！
    
    if (ExtraInfo.IsEmpty())
        return FString::Printf(TEXT("I %s %s"), *Verb, *TargetName);
    else
        return FString::Printf(TEXT("I %s %s %s"), *Verb, *TargetName, *ExtraInfo);
}
```

这说明：
- 如果 Actor 有 `"Player"` Tag，会被识别为玩家
- 但这**不影响 AI Perception 的感知**，只是改变了描述文本

所以这不是原因。

---

### 3. 玩家使用了不同的 Pawn 类

如果你的玩家使用的是 `CombatCharacter` 或其他特殊的 Pawn 类，可能在某个地方（Blueprint 或 C++）已经配置了感知相关的设置。

---

## 实验验证

让我们做个简单的实验来验证：

### 实验 1: 检查玩家 Blueprint

1. 打开你的玩家 Blueprint（例如 `BP_ThirdPersonCharacter`）
2. 查看 Components 面板
3. 截图发给我，我帮你看看

### 实验 2: 临时移除玩家的刺激源

如果玩家 Blueprint 有 `AIPerceptionStimuliSource` 组件：
1. 取消勾选 `Auto Register as Source`
2. 运行游戏
3. 看看 NPC 是否还能看到玩家

**预期结果**：
- 如果 NPC 看不到玩家了 → 证明玩家确实有 `AIPerceptionStimuliSource`
- 如果 NPC 还能看到玩家 → 说明有其他机制在起作用

---

## 最可能的真相

根据 Unreal 官方文档和我的经验，**最可能的情况是**：

```
你的玩家 Blueprint 已经配置了 AIPerceptionStimuliSource 组件
                    ↓
            NPC 能看到玩家 ✅
                    
你的 NPC Blueprint 没有配置 AIPerceptionStimuliSource 组件
                    ↓
            NPC 看不到彼此 ❌
```

---

## 解决方案

### 方案 1: 复制玩家的配置到 NPC

1. 打开玩家 Blueprint
2. 查看 `AIPerceptionStimuliSource` 组件的配置
3. 在 NPC Blueprint 中添加相同的配置

### 方案 2: 在 C++ 中统一处理

创建一个基类，自动为所有角色添加 `AIPerceptionStimuliSource`：

```cpp
// PerceivableCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PerceivableCharacter.generated.h"

class UAIPerceptionStimuliSourceComponent;

UCLASS()
class AINPC_API APerceivableCharacter : public ACharacter
{
    GENERATED_BODY()
    
public:
    APerceivableCharacter();
    
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionStimuliSourceComponent* StimuliSource;
};

// PerceivableCharacter.cpp
#include "PerceivableCharacter.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

APerceivableCharacter::APerceivableCharacter()
{
    // 创建 Stimuli Source 组件
    StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
    
    // 注册为视觉和听觉的刺激源
    StimuliSource->RegisterForSense(TSubclassOf<UAISense_Sight>());
    StimuliSource->RegisterForSense(TSubclassOf<UAISense_Hearing>());
    StimuliSource->RegisterWithPerceptionSystem();
}
```

然后让玩家和 NPC 都继承这个基类。

---

## 调试技巧

### 1. 查看日志

运行游戏时，查看日志中是否有：

```
LogAIPerception: Warning: No stimuli source registered for BP_Zombie_C_0
```

如果有这个警告，说明 NPC 确实没有注册为刺激源。

### 2. 可视化感知范围

按 `'` 键打开控制台，输入：
```
ShowDebug AI
```

你应该能看到：
- **绿色锥体**：NPC 的视觉范围
- **绿色点**：被感知到的 Actor

如果玩家在视觉范围内但没有绿色点，说明玩家没有注册为刺激源。

---

## 总结

**最可能的情况**：
- ✅ 玩家 Blueprint 有 `AIPerceptionStimuliSource` → NPC 能看到玩家
- ❌ NPC Blueprint 没有 `AIPerceptionStimuliSource` → NPC 看不到彼此

**解决方案**：
- 在 NPC Blueprint 中添加 `AIPerceptionStimuliSource` 组件
- 配置和玩家一样的设置

**下一步**：
1. 检查你的玩家 Blueprint，看看是否有 `AIPerceptionStimuliSource`
2. 如果有，复制相同的配置到 NPC Blueprint
3. 测试 NPC 是否能互相看到

需要我帮你检查具体的 Blueprint 配置吗？

