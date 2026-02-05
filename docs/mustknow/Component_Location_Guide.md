# Component Location Guide: Controller vs Pawn

> **MUST KNOW** - 本项目中反复出现的 Bug 根源。每次写 `FindComponentByClass` 之前请先查阅此表。

## 组件分布总览

| Component | 所在位置 | 创建方式 | 备注 |
|-----------|---------|---------|------|
| **AIPerceptionComponent** | Controller | `UtilityAIController` 构造函数 | UE 标准做法 |
| **SensoryComponent** | Controller | `UtilityAIController` 构造函数 | 语义层，包装 AIPerception |
| **MemoryComponent** | Controller | `UtilityAIController` 构造函数 | |
| **CognitionComponent** | Controller | `UtilityAIController` 构造函数 | 含 Interpolator |
| **UtilityAIComponent** | Controller | `UtilityAIController` 构造函数 | |
| **PersonalityComponent** | Controller | `UtilityAIController` 构造函数 | OCEAN + Maslow Weights |
| **EmotionDisplayComponent** | Controller | `UtilityAIController` 构造函数 | |
| **MetabolismComponent** | Controller | `UtilityAIController` 构造函数 | |
| **GoalComponent** | Controller | `UtilityAIController` 构造函数 | |
| **FactionReputationComponent** | **Pawn** | **蓝图 (Blueprint)** | **不在 Controller 上!** |
| **NPCDefinitionComponent** | **Pawn** | **蓝图 (Blueprint)** | **不在 Controller 上!** |
| **MonsterComponent** | **Pawn** | **蓝图 (Blueprint)** | **不在 Controller 上!** |
| **CharacterMovementComponent** | **Pawn** | UE 自动创建 | Character 自带 |
| **CapsuleComponent / Mesh** | **Pawn** | UE 自动创建 | Character 自带 |

## 规则

```
Controller 上的组件 → 用 Controller->FindComponentByClass<T>()
Pawn 上的组件      → 用 Pawn->FindComponentByClass<T>()  (或 Controller->GetPawn()->...)
```

**绝对不要用 `Controller->FindComponentByClass` 去找 Pawn 上的组件，反之亦然。**

## 已知因此产生的 Bug

### Bug #1: HasEnemyNearby 跳过 Attitude 检查 (2026-02-05)
- **文件**: `UtilityActionBase.cpp` — `GetConsiderationValue()` — `HasEnemyNearby` case
- **错误写法**: `Controller->FindComponentByClass<UFactionReputationComponent>()`
- **正确写法**: `BotPawn->FindComponentByClass<UFactionReputationComponent>()`
- **后果**: FactionComp 永远是 nullptr → TargetSelectionSubsystem 跳过整个 Attitude 检查 → 友好的 Player 被判定为合法战斗目标 → Orc 攻击友好 Player
- **症状**: Plot 模式下 NPC 无视 Faction 关系攻击玩家

## 安全查找模式

当你不确定组件在 Controller 还是 Pawn 上时，使用以下模式：

```cpp
// 从 Controller 出发，安全查找任意组件
template<typename T>
T* FindComponentSafe(AAIController* Controller)
{
    if (!Controller) return nullptr;

    // 1. 先查 Controller
    if (T* Comp = Controller->FindComponentByClass<T>())
        return Comp;

    // 2. 再查 Pawn
    if (APawn* Pawn = Controller->GetPawn())
        return Pawn->FindComponentByClass<T>();

    return nullptr;
}
```

## 特殊情况：Player 角色

Player 角色 (`AINPCCharacter` / `CombatCharacter`) 没有以下组件：
- `FactionReputationComponent` — 没有
- `NPCDefinitionComponent` — 没有
- `PersonalityComponent` — 没有

Player 的 Faction 通过 `FactionHelpers::GetFactionID()` 的 fallback 链解析：
1. FactionReputationComponent → **无**
2. `Faction.*` Tag → **无**
3. NPCDefinitionComponent → **无**
4. `Player` Tag → **CombatCharacter 有 / AINPCCharacter 无**
5. Default → `"Neutral"`

> **注意**: `AINPCCharacter`（ThirdPerson 变体）没有 `Player` Tag，会 fallback 到 `"Neutral"` 而不是 `"Player"`。如果需要 Faction 表中定义的 Orc→Player 关系生效，请确保角色蓝图添加了 `Player` Tag。

## Checklist: 写新代码前

- [ ] 我要找的组件在 Controller 上还是 Pawn 上？（查上面的表）
- [ ] 我当前持有的引用是 Controller 还是 Pawn？
- [ ] 如果是跨系统调用（如 TargetSelectionSubsystem），传入的 FactionComp 参数是从哪找的？
- [ ] Player 角色有没有这个组件？如果没有，null 分支是否安全？
