# AINPC 项目架构注意事项

## 🏗️ 核心架构约定

### 1. Component 所有权位置 ⚠️ **关键**
```
Controller (AUtilityAIController):
  ├── PersonalityComponent ✅ 在这里！
  ├── CognitionComponent
  ├── MemoryComponent
  ├── SensoryComponent
  ├── GoalComponent
  ├── UtilityAIComponent
  └── MentalState (UNPCMentalState*)

Pawn:
  ├── EmotionDisplayComponent
  └── NPCDefinitionComponent
```

**关键点**: 在任何需要访问 `PersonalityComponent` 的地方（如 `CalculateScore`），**必须先从 Controller 查找，再 fallback 到 Pawn**！

```cpp
// ✅ 正确做法
if (AUtilityAIController* UtilityController = Cast<AUtilityAIController>(Controller))
{
    PersonalityComp = UtilityController->PersonalityComp;
}

// ❌ 错误做法
if (APawn* BotPawn = Controller->GetPawn())
{
    PersonalityComp = BotPawn->FindComponentByClass<UPersonalityComponent>(); // 永远找不到！
}
```

---

### 2. DataTable vs JSON
- ✅ **使用 DataTable 编辑器直接编辑配置**
- ❌ **不是** 从 JSON 文件直接解析
- JSON 文件只是导出参考

**重要**: Unreal DataTable **可以直接编辑 `TMap<Enum, T>` 类型**，不需要额外的 workaround 字段！

---

### 6. Mental State 变量管辖权
```
变量            | 主控方   | 说明
----------------|----------|---------------------------
Hunger          | ENGINE   | MetabolismComponent 控制
Fatigue         | ENGINE   | MetabolismComponent 控制
Perceived_Threat| ENGINE ⚠️| 游戏系统有覆盖权！敌人死亡→强制归零
Loneliness      | HYBRID   | 游戏计时器 + LLM 社交检测
Indignity       | LLM      | 只有 LLM 能感知尊严受损
Boredom         | LLM      | 只有 LLM 能感知无聊
Intention       | LLM      | LLM 主观决策：Attack/Flee/Idle/Talk
```

**关键**：虽然 LLM 可以输出 `Perceived_Threat`，但当敌人死亡时，
游戏系统会**强制覆盖**为 0，防止"僵尸已死但仍害怕"的问题。

---

### 3. Gameplay Tags 命名空间
```cpp
// ✅ 正确 - 使用统一命名空间
#include "Social/SocialGameplayTags.h"
AINPCTags::Status_Dead
AINPCTags::Directive_Survival
AINPCTags::Directive_Social

// ❌ 错误 - 已废弃
FAINPCNativeTags::Get().XXX  // 已删除
FGameplayTag::RequestGameplayTag("...")  // 避免使用
```

---

### 4. Mental State 变量名
```cpp
// ✅ 正确 - 使用常量
#include "UtilityAI/MentalStateNames.h"
MentalStateNames::Hunger
MentalStateNames::Perceived_Threat

// ❌ 错误 - 硬编码字符串
"Hunger"  // 容易拼写错误
```

---

### 5. 常量定义位置
```cpp
// 游戏全局常量 (如 Directive 加成) → UtilityActionBase.h (static const)
static const float DirectiveMatchMultiplier = 1.5f;

// 配置参数 (如 Modifier 范围) → 直接硬编码或用 UPROPERTY(Config)
```

---

## 🐛 常见陷阱

### 陷阱 1: 忘记 PersonalityComponent 在 Controller 上
**症状**: PersonalityComp 总是 NULL  
**解决**: 先从 `UtilityController->PersonalityComp` 获取

### 陷阱 2: 以为 JSON 会自动解析 TMap
**症状**: DataTable 中的 TMap 字段显示为空  
**解决**: 在 DataTable 编辑器中直接编辑 TMap（支持的！）

### 陷阱 3: 使用错误的 Gameplay Tag 命名空间
**症状**: 编译错误 "FAINPCNativeTags not found"  
**解决**: 使用 `AINPCTags::XXX` 替代

---

## 📁 关键文件路径

### 配置文件
- **Actions**: `Content/AINPC/Datatable/DT_NPC_Actions1` (DataTable Asset)
- **Personalities**: `Content/AINPC/Datatable/DT_Personalities`
- **Professions**: `Content/AINPC/Datatable/DT_Professions`

### 核心代码
- **Action 基类**: `Source/AINPC/Public/Base/UtilityActionBase.h`
- **Controller**: `Source/AINPC/Controller/UtilityAIController.h`
- **Tags**: `Source/AINPC/Public/Social/SocialGameplayTags.h`
- **Constants**: `Source/AINPC/Public/UtilityAI/MentalStateNames.h`

---

## 🎯 设计哲学

### Personality Action Modifier (PAM)
- **正相关** (Factor > 0): Trait 越高，Action 分数越高
- **负相关** (Factor < 0): Trait 越低，Action 分数越高
- **范围**: 0.5x - 2.0x (可调整 `1.5f` 系数)

**示例**:
```
Attack: PersonalityInfluence { Neuroticism: -1.0 }
  → Brave (N=0.0) 获得 2.0x bonus
  → Cautious (N=1.0) 获得 0.5x penalty

Flee: PersonalityInfluence { Neuroticism: 1.0 }
  → Brave (N=0.0) 获得 0.5x penalty
  → Cautious (N=1.0) 获得 2.0x bonus
```

---

## ✅ 新功能添加清单

添加新 Utility Action 时：
1. [ ] 在 DataTable 中添加行，配置 Considerations
2. [ ] 如果需要性格影响，配置 `PersonalityInfluence` (TMap)
3. [ ] 设置 `DirectiveTag` (如 Directive.Survival)
4. [ ] 设置 `IntentionTag` (如 Intention.Attack，用于 LLM 匹配)
5. [ ] 如果是 SmartObject，配置 `SmartObjectTag` + Animation

添加新 Mental State 变量时：
1. [ ] 在 `MentalStateTypes.h` 的 `MENTAL_STATE_FIELDS` 宏中添加
2. [ ] 在 `MentalStateNames.h` 中添加对应常量
3. [ ] 在 `PersonalityComponent::GetWeightForVariable` 中添加权重映射
4. [ ] 如果需要 LLM 输出，更新 `LLMCommunicator` 的 Prompt

---

## 🚨 改动代码前必读

1. **查找 Component**: 优先从 Controller 查找，再 Pawn，最后 Owner
2. **使用常量**: 绝不硬编码字符串或数字
3. **检查 NULL**: 所有 `FindComponentByClass` 必须检查返回值
4. **日志分类**: 使用 `BRAIN_LOG`, `UTILITY_LOG` 等专用宏
5. **测试两种性格**: Brave + Cautious 必须表现出明显差异

---

**最后更新**: 2026-01-21  
**维护者**: AI + User
