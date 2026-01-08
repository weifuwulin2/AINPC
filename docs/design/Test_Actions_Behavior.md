# Test Actions Behavior & Logic
# 测试动作行为与逻辑

This document details the implementation logic of the core Test Actions (`Test_Idle`, `Test_Attack`, `Test_Flee`) used to validate the Utility AI and Role-Playing frameworks.
本文档详细介绍了用于验证 Utility AI 和角色扮演框架的核心测试动作（`Test_Idle`, `Test_Attack`, `Test_Flee`）的实现逻辑。

## 1. Test_Idle (待机)

*   **Logic**:
    *   Wanders randomly within a small radius.
    *   Acts as a **fallback action** when no other action has a high score.
    *   在小半径内随机漫游。
    *   当没有其他动作得分高时，作为 **保底动作**。
*   **Scoring (Config)**:
    *   **BaseReward**: Should be low (e.g., `0.1` to `0.5`).
    *   **Inertia**: Low.
    *   *Previous Issue*: BaseReward was too high (`2.0`), causing NPCs to ignore threats. Fixed by verifying configuration.
    *   *之前的问题*: BaseReward 太高 (`2.0`)，导致 NPC 无视威胁。已通过验证配置修复。

## 2. Test_Attack (攻击)

*   **Logic**:
    *   **Target Selection**: Scans for the **nearest Character** (excluding self).
    *   **Tag Filtering**: None (Zombies attack anyone).
    *   **Move & Attack**: Moves to range -> Deals Damage via `UGameplayStatics::ApplyDamage`.
    *   **目标选择**: 扫描 **最近的角色**（排除自己）。
    *   **标签过滤**: 无（僵尸攻击任何人）。
    *   **移动与攻击**: 移动到范围 -> 造成伤害。
*   **Scoring**:
    *   Drivien by `Anger` (Motivation) and `TargetHealth` (Context).
*   **Behavior**:
    *   Used to test "Zombie" or "Warrior" personas.
    *   Zombies will attack other NPCs if they are closer than the player.
    *   僵尸会攻击其他 NPC，如果它们比玩家更近。

## 3. Test_Flee (逃跑)

*   **Logic (Enhanced)**:
    *   **Target Selection (Smart)**:
        1.  **Priority**: Looks for actors with the **Tag "Enemy"** (e.g., Zombies).
        2.  **Fallback**: If no "Enemy" found, looks for Player (Controller 0).
        3.  *Benefit*: Prevents NPCs from scaring each other (Friendly NPCs are ignored).
        4.  *优势*: 防止 NPC 互相惊吓（友方 NPC 被忽略）。
    *   **Movement**:
        1.  Calculates vector away from threat: `-(ThreatLocation - MyLocation)`.
        2.  Throttles `MoveToLocation` calls to every 0.25s for performance and smoothness.
        3.  計算远离威胁的向量。
        4.  每 0.25 秒更新移动目标，以获得平滑表现。
    *   **Completion Condition (Continuous)**:
        1.  **Never Complete**: The action runs indefinitely as long as Utility AI selects it.
        2.  **No Safe Distance Stop**: It keeps running even if far away, assuming the "Fear" mental state is still high.
        3.  **Utility Switching**: It only stops when the `Perceived_Threat` (and thus `Fear`) decays naturally in the Mental State, causing the Action Score to drop below Idle.
        4.  **永不完成**: 只要 Utility AI 发送它，动作就会无限期运行。
        5.  **无安全距离停止**: 即使跑得很远，只要“恐惧”心理状态仍然很高，它就会继续跑。
        6.  **Utility 切换**: 只有当心理状态中的 `Perceived_Threat`（以及 `Fear`）自然衰减，导致动作得分低于 Idle 时，它才会停止。

## 4. Key Improvements Summary / 关键改进总结

1.  **Jurisdiction Enforcement**: `CognitionComponent` now strictly ignores LLM attempts to modify `Hunger` or `Energy`.
2.  **Continuous Fleeing**: Flee action no longer stops abruptly at a fixed distance; it respects the emotional state duration.
3.  **Vector Correction**: Verified flee vector is correctly pointing *away* from the threat.
4.  **Tag-Based Hostility**: Implemented simple IFF (Identification Friend or Foe) for Fleeing using Actor Tags.
