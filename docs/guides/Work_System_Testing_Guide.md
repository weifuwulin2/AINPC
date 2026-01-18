# Profession System Testing Guide

## 🎯 Objective
Verify that NPCs correctly perform work actions based on their assigned **Profession** and **Schedule**.

## 🛠️ Prerequisites
- **TimeManager** must be active.
- **UtilityAI** system must be compiled with recent fixes (Profession Filtering).
- **DT_UtilityActions** must contain Work actions (e.g., `Work_Farm`, `Work_Guard`).

## 🧪 Test Setup

### 1. Create Work SmartObjects
Create a simple Actor (or use existing SmartObjects) to serve as work stations.
*   **Method A (Blueprint)**: Add a `SmartObjectComponent`. Set its `SocialTag` in Details Panel.
*   **Method B (Tagging)**: Add a GameplayTag to the Actor itself (if your Sensory reads Actor Tags). *Recommendation: Use Method A.*

| Profession | Required Tag (SocialTag) | Example Object |
| :--- | :--- | :--- |
| **Farmer** | `Interaction.Work.Farm` | A patch of dirt or a plant |
| **Miner** | `Interaction.Work.Mine` | A rock or crystal |
| **Guard_Day** | `Interaction.Work.Guard` | A patrol point or guard post |
| **Merchant** | `Interaction.Work.Trade` | A market stall |
| **Scholar** | `Interaction.Work.Study` | A bookshelf or desk |
| **Zealot** | `Interaction.Work.Pray` | An altar or statue |

### 2. Configure the Test NPC
Select your test NPC in the level (e.g., `BP_NPC_Base`).
*   **ProfessionID**: Set to `Farmer` (or your target profession).
*   **Schedule**: Ensure `DT_Professions` has a schedule that includes `Directive.Work` for the current time roughly.
    *   *Default Farmer Schedule*: Usually 08:00 - 18:00.

### 3. Set the Time
Ensure `BP_TimeManager` (or GameState) is set to a working hour.
*   **Console Command**: `SetTime 10` (if implemented)
*   OR set `StartTime` in `TimeManager` default properties to `10.0`.

## 🏃‍♂️ Execution Steps

1.  **Play in Editor (PIE)**.
2.  **Observe Log**: Look for `[Goal] New Goal: Work` or similar logs from `GoalComponent`.
    *   If you see `New Goal: Idle`, check the Time or Schedule.
3.  **Observe Utility AI**: Look for `[UtilityAI] Best Action: Work_Farm`.
    *   If failing, check if `FindBestSmartObject` found your Farm object.
4.  **Observe Behavior**: NPC should walk to the object and play the interaction (animation).

## 🚑 Troubleshooting

| Issue | Check |
| :--- | :--- |
| **NPC stays Idle** | Is `GoalComponent` outputting `Directive.Work`? Check Time. Check `DT_Professions` schedule. |
| **NPC wanders aimedlessly** | Is `FindBestSmartObject` failing? Check if your SmartObject has the correct **GameplayTag**. Check distance (< 150m). |
| **NPC does wrong job** | Check `DT_UtilityActions`. Does `Work_Mine` accidentally have `RequiredProfessionID: Farmer`? (Should be Miner). |
| **Action Score is 0** | Check `Context: GoalDirectiveMatch`. If Goal says `Survival`, Work might be suppressed. Feed the NPC first! |

## ✅ Success Criteria
*   [ ] NPC identifies the correct work station.
*   [ ] NPC performs the work animation.
*   [ ] NPC stops working when Schedule changes (e.g., Night time) or Survival needs invoke (Starving).
