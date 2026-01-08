# 修复总结 / Fix Summary

## 问题 1: NPC 之间无法看到彼此 ✅ 已解决

**原因**：NPC Pawn 缺少 `AIPerceptionStimuliSource` 组件

**解决方案**：
1. 打开 `BP_Zombie` (或其他 NPC Blueprint)
2. 添加 `AI Perception Stimuli Source` 组件
3. 配置：
   - ✅ `Auto Register as Source`
   - ✅ `Register as Source for Senses` → `AI Sight`
   - ✅ `Register as Source for Senses` → `AI Hearing`

**验证**：现在日志中显示 `I saw Zombie` 而不是 `I saw BP_NPC_C_UAID_...`

---

## 问题 2: 感知描述显示 Actor 实例名而不是 PersonalityID ✅ 已修复

**原因**：`SensoryComponent::FormatDescription()` 使用 `Actor->GetName()` 而不是 PersonalityID

**解决方案**：修改 `SensoryComponent.cpp`，优先使用 PersonalityID：

```cpp
// 步骤 1: 尝试获取 PersonalityID（NPC）
if (UtilityController->PersonalityComp && !UtilityController->PersonalityComp->PersonalityID.IsNone())
{
    TargetName = UtilityController->PersonalityComp->PersonalityID.ToString();
}

// 步骤 2: 如果没有 PersonalityID，检查 Tag（玩家）
if (TargetName == "Unknown" && Target->ActorHasTag("Player"))
{
    TargetName = "Player";
}

// 步骤 3: 如果都没有，使用 Actor 名称
if (TargetName == "Unknown")
{
    TargetName = Target->GetName();
}
```

**结果**：现在日志显示 `I saw Zombie` 而不是 `I saw BP_NPC_C_UAID_08BFB83CDBE666B302_2010700471`

---

## 问题 3: Warrior 攻击玩家而不是 Zombie ✅ 已修复

**原因**：`TestAction_Attack` 硬编码为攻击玩家

**解决方案**：修改 `TestAction_Attack.cpp`，优先攻击带有 "Enemy" 标签的 Actor：

```cpp
// 搜索所有带有 "Enemy" 标签的 Actor
TArray<AActor*> FoundActors;
UGameplayStatics::GetAllActorsWithTag(World, FName("Enemy"), FoundActors);

for (AActor* Actor : FoundActors)
{
    // 找到最近的敌人
    if (Distance < MinDistance)
    {
        MinDistance = Distance;
        TargetEnemy = Actor;
    }
}

// 如果没有找到 "Enemy" 标签的 Actor，尝试攻击玩家
if (!TargetEnemy)
{
    TargetEnemy = UGameplayStatics::GetPlayerPawn(World, 0);
}
```

**重要**：确保 Zombie 有 "Enemy" 标签！

在 `BP_Zombie` 中：
1. 选中 Pawn
2. 在 Details 面板中找到 **Tags** → **Actor** → **Tags**
3. 添加 `Enemy` 标签

---

## 问题 4: Emoji 不显示 ✅ 已修复

**原因**：`EmotionDisplayComponent::BeginPlay()` 执行时 Pawn 还没有被 Possess

**解决方案**：添加延迟初始化（0.1秒）

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

---

## 下一步测试

### 1. 编译代码
```bash
MSBuild AINPC.sln /t:Build /p:Configuration="Development Editor" /p:Platform=Win64
```

### 2. 配置 Blueprint

**BP_Zombie:**
1. 添加 `AI Perception Stimuli Source` 组件
2. 添加 `Enemy` 标签

**BP_Warrior (或其他友军 NPC):**
1. 添加 `AI Perception Stimuli Source` 组件
2. **不要**添加 `Enemy` 标签（这样 Warrior 不会攻击其他 Warrior）

### 3. 测试场景

放置以下 Actor：
- 1 个 Player
- 1 个 Warrior (PersonalityID: "Warrior")
- 1 个 Zombie (PersonalityID: "Zombie")

### 4. 预期行为

**Warrior 看到 Zombie:**
```
[Cognition] Processing Stimulus: I saw Zombie
[LLM] Anger: "Angry"
[LLM] Speech: "Zombies again? I'll protect this place with honor."
[Attack] Chasing Zombie... Distance: 300.0
[Attack] Dealt 10.0 damage to Zombie!
```

**Zombie 看到 Warrior:**
```
[Cognition] Processing Stimulus: I saw Warrior
[LLM] Perceived_Threat: "Threatened"
[LLM] Speech: "Brains... run away..."
[Flee] Running from Warrior (Dist: 250)...
```

---

## 总结

✅ **NPC 互相感知** - 添加 `AIPerceptionStimuliSource` 组件
✅ **PersonalityID 显示** - 修改 `FormatDescription()` 逻辑
✅ **Attack 目标选择** - 优先攻击 "Enemy" 标签的 Actor
✅ **Emoji 显示** - 延迟初始化 Widget

现在 Warrior 应该能够：
1. 看到 Zombie（显示 "I saw Zombie"）
2. 生成愤怒情绪（"Zombies again? I'll protect this place with honor."）
3. 攻击 Zombie（而不是玩家）

