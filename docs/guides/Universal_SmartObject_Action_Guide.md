# Universal Smart Object Action Implementation Guide

## Overview

We have implemented a **Data-Driven Universal Action** system. Instead of creating separate C++ classes for "Eating", "Sleeping", "Drinking", etc., we use a single C++ class (`UAction_SmartObject`) and configure its behavior via the `DataTable`.

## Core Components

1.  **UAction_SmartObject (C++)**:
    *   **Logic**: Finds a Smart Object with a specific Tag -> Moves to it -> Interacts -> Restores a specific Stat based on Tag.
    *   **Location**: `Source/AINPC/Private/Actions/Action_SmartObject.cpp`

2.  **USensoryComponent (C++)**:
    *   **Function**: `FindBestSmartObject(FGameplayTag ActivityTag)`
    *   **Logic**: Scans recently perceived actors (and fallback to nearby actors) for one with a matching `SocialTag` in its `SmartObjectComponent`.

3.  **FUtilityActionConfig (Struct)**:
    *   New Field: `SmartObjectTag` (FGameplayTag).

## How to Configure (DataTable)

To implement "Eat" and "Sleep" behaviors, you do **not** write C++ code. You add **Rows** to your `DT_UtilityActions`.

### Example 1: Creating "Eat" Action
*   **Row Name**: `Action_Eat`
*   **Action Class**: `UAction_SmartObject`
*   **Base Reward**: `1.0` (or as needed)
*   **Considerations**:
    *   **Input**: `Hunger`
    *   **Curve**: `Logistic` (or Linear)
    *   **Type**: `Motivation`
    *   *(Optional Context)*: `HasFoodNearby` -> `Context`
*   **Smart Object Config**:
    *   **SmartObjectTag**: `Activity.Eat`

### Example 2: Creating "Sleep" Action
*   **Row Name**: `Action_Sleep`
*   **Action Class**: `UAction_SmartObject`
*   **Base Reward**: `1.5` (Sleep is important!)
*   **Considerations**:
    *   **Input**: `Energy` (Fatigue)
    *   **Curve**: `InverseQuadratic` (Becomes high priority quickly when tired)
    *   **Type**: `Motivation`
    *   *(Optional Context)*: `HasBedNearby` -> `Context`
*   **Smart Object Config**:
    *   **SmartObjectTag**: `Activity.Rest`

## Stats Restoration Logic
The C++ class currently handles restoration hardcoded based on tags (for safety):
*   `Activity.Eat` -> Reduces `Hunger`.
*   `Activity.Rest` -> Reduces `Energy` (Fatigue).

## Adding New Smart Objects
1.  Place an Actor (Prop).
2.  Add `SmartObjectComponent`.
3.  In Details Panel -> **Semantics** -> **Social Tag**: Set to `Activity.Eat` or `Activity.Rest`.

## Best Practices: Tuning Base Rewards ⚖️

To ensure natural behavior, use a **Priority Pyramid** when setting `Base Reward`. This ensures urgent needs override casual ones.

### 1. 🚨 Survival (1.5 - 2.0)
*   **Flee**: `2.0` (Immediate threat response)
*   **Eat / Sleep**: `1.5 - 1.8` (Prevents starving while chatting)
*   *Tip: Set these higher than Social actions so needs break conversations.*

### 2. 💬 Social & Work (1.0)
*   **Talk / Work**: `1.0` (Standard priority)
*   *only performed when survival needs are low.*

### 3. 🍂 Idle (0.1 - 0.5)
*   **Idle / Wander**: `0.3` (Fallback)
*   *Executed only when nothing else is important.*

