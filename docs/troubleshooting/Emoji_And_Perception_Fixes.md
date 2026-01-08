# 问题修复总结 / Problem Fixes Summary

## 问题 1: Emoji 不显示 / Emoji Not Displaying

### 原因 / Root Cause

`EmotionDisplayComponent::BeginPlay()` 在 Controller Possess Pawn 之前就执行了，导致：
- `AIController->GetPawn()` 返回 `nullptr`
- Widget 组件无法创建和附加到 Pawn

### 解决方案 / Solution

**延迟初始化 Widget 组件**，使用 Timer 延迟 0.1 秒后再创建：

```cpp
void UEmotionDisplayComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 延迟创建 Widget 组件，确保 Pawn 已经被 Possess
    if (GetWorld())
    {
        FTimerHandle DelayHandle;
        GetWorld()->GetTimerManager().SetTimer(
            DelayHandle,
            [this]()
            {
                CreateWidgetComponents();
                BindToCognitionEvents();
            },
            0.1f,  // 延迟 0.1 秒
            false
        );
    }
}
```

### 验证步骤 / Verification Steps

1. 编译并运行游戏
2. 查看日志，应该看到：
   ```
   [EmotionDisplay] BeginPlay called
   [EmotionDisplay] Scheduled delayed initialization
   [EmotionDisplay] Delayed initialization starting...
   [EmotionDisplay] ✅ AIController found: BP_UtilityAIController_C_0
   [EmotionDisplay] ✅ Controlled Pawn found: BP_Zombie_C_0
   [EmotionDisplay] ✅ Emoji Widget Component created with class: WBP_EmojiDisplay
   [EmotionDisplay] ✅ Successfully bound to CognitionComponent events
   ```

---

## 问题 2: NPC 之间无法看到彼此 / NPCs Cannot See Each Other

### 原因 / Root Cause

AI Perception 需要 Pawn 注册为 **Stimuli Source（刺激源）** 才能被其他 AI 感知到。

虽然 `UtilityAIController` 已经配置了：
```cpp
SightConfig->DetectionByAffiliation.bDetectEnemies = true;
SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
```

但是 **Pawn 本身没有注册为可被感知的对象**。

### 解决方案 / Solution

有两种方法：

#### 方法 A: 在 Blueprint 中添加 `AIPerceptionStimuliSource` 组件（推荐）

1. 打开 `BP_Zombie`（或其他 NPC Blueprint）
2. 点击 **Add Component**
3. 搜索并添加 **AI Perception Stimuli Source**
4. 在 Details 面板中配置：
   ```
   AI Perception Stimuli Source:
   ├── Auto Register as Source: ✅ (勾选)
   └── Register as Source for Senses:
       ├── AI Sight: ✅ (勾选)
       └── AI Hearing: ✅ (勾选)
   ```
5. 编译并保存

#### 方法 B: 在 C++ 中自动注册（更自动化）

修改 `AICharacterBase` 或创建一个新的基类，在 `BeginPlay` 中自动注册：

```cpp
// AICharacterBase.h
#include "Perception/AIPerceptionStimuliSourceComponent.h"

UCLASS()
class AINPC_API AAICharacterBase : public ACharacter
{
    GENERATED_BODY()
    
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionStimuliSourceComponent* StimuliSource;
    
public:
    AAICharacterBase();
};

// AICharacterBase.cpp
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

### 验证步骤 / Verification Steps

1. 在场景中放置两个 NPC（例如两个 Zombie）
2. 运行游戏
3. 查看日志，应该看到：
   ```
   [Sensory] I saw BP_Zombie_C_1
   [Cognition] Processing stimulus: I saw BP_Zombie_C_1
   ```

### 调试技巧 / Debugging Tips

如果还是看不到，检查以下内容：

1. **检查 Sight Config 的范围**：
   ```cpp
   SightConfig->SightRadius = 1500.0f;  // 确保足够大
   ```

2. **在编辑器中可视化感知范围**：
   - 运行游戏
   - 按 `'`（单引号键）打开控制台
   - 输入：`ShowDebug AI`
   - 你应该能看到绿色的视觉锥体

3. **检查日志**：
   ```
   LogAIPerception: Warning: No stimuli source registered for [ActorName]
   ```
   如果看到这个警告，说明 Pawn 没有注册为刺激源。

---

## 总结 / Summary

### Emoji 显示问题
- ✅ **已修复**：添加了延迟初始化机制
- ✅ **已添加**：详细的调试日志
- ⏳ **待验证**：编译完成后测试

### NPC 感知问题
- ❌ **未修复**：需要添加 `AIPerceptionStimuliSource` 组件
- 📝 **推荐方案**：在 Blueprint 中手动添加（方法 A）
- 🔧 **长期方案**：创建 `AICharacterBase` 类自动处理（方法 B）

---

## 下一步 / Next Steps

1. ✅ 等待编译完成
2. ✅ 测试 Emoji 显示是否正常
3. ⚠️ 在 NPC Blueprint 中添加 `AIPerceptionStimuliSource` 组件
4. ✅ 测试 NPC 之间是否能互相感知

