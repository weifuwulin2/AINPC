# Zombie Attack Setup Guide / 僵尸攻击配置指南

## Problem / 问题

Zombies are not attacking the player even when they see them. The LLM returns `Perceived_Threat: "None"`.

僵尸即使看到玩家也不会攻击。LLM 返回 `Perceived_Threat: "None"`。

## Root Cause / 根本原因

The LLM doesn't know the NPC is a hostile zombie. Without role context, the LLM interprets "I saw Player" as a neutral observation, not a threat.

LLM 不知道这个 NPC 是敌对僵尸。没有角色上下文，LLM 将"我看到了玩家"解释为中性观察，而不是威胁。

### Why This Happens / 为什么会这样

1. **Sensory System Works** ✅  
   - The `SensoryComponent` detects the player via `AIPerception`
   - It broadcasts: `"I saw Player"`
   
   感知系统正常工作
   - `SensoryComponent` 通过 `AIPerception` 检测到玩家
   - 广播：`"我看到了玩家"`

2. **LLM Receives Input** ✅  
   - `CognitionComponent` receives the stimulus
   - Sends it to the LLM
   
   LLM 接收输入
   - `CognitionComponent` 接收刺激
   - 发送给 LLM

3. **LLM Lacks Context** ❌  
   - The default prompt only increases `Perceived_Threat` for **verbal threats** ("I'll kill you")
   - Seeing a player is not interpreted as a threat without role context
   
   LLM 缺少上下文
   - 默认提示词只对**言语威胁**("我要杀了你")增加 `Perceived_Threat`
   - 没有角色上下文，看到玩家不会被解释为威胁

4. **Attack Action Gets 0 Score** ❌  
   - Attack action uses `Perceived_Threat` as input
   - `Perceived_Threat = 0` → Attack score = 0
   - Zombie stays idle
   
   攻击动作得分为 0
   - 攻击动作使用 `Perceived_Threat` 作为输入
   - `Perceived_Threat = 0` → 攻击得分 = 0
   - 僵尸保持闲置

## Solution / 解决方案

Configure the `RoleDescription` and `BehavioralGuidelines` in the **PersonalityConfig DataTable** to provide role-specific context to the LLM.

在 **PersonalityConfig 数据表** 中配置 `RoleDescription` 和 `BehavioralGuidelines`，为 LLM 提供特定角色的上下文。

### Step 1: Open Personality DataTable / 打开性格数据表

1. In the Content Browser, find your `DT_PersonalityConfig` DataTable (or create one if it doesn't exist)
2. Double-click to open it

在内容浏览器中，找到你的 `DT_PersonalityConfig` 数据表(如果不存在则创建一个)
双击打开它

### Step 2: Create/Edit Zombie Personality Row / 创建/编辑僵尸性格行

1. Add a new row with name `"Zombie"` (or edit an existing one)
2. Set the OCEAN personality values (optional, but recommended):
   - **Openness**: 0.1 (low curiosity)
   - **Conscientiousness**: 0.2 (no sense of duty)
   - **Extraversion**: 0.3 (not social)
   - **Agreeableness**: 0.0 (hostile, no cooperation)
   - **Neuroticism**: 0.8 (high anxiety/aggression)

添加一个名为 `"Zombie"` 的新行(或编辑现有行)
设置 OCEAN 性格值(可选，但推荐):
- **开放性**: 0.1 (低好奇心)
- **尽责性**: 0.2 (无责任感)
- **外向性**: 0.3 (不社交)
- **宜人性**: 0.0 (敌对，不合作)
- **神经质**: 0.8 (高焦虑/攻击性)

### Step 3: Set Role Description / 设置角色描述

In the **Role Description** field, enter:

```
You are a hostile zombie that attacks humans on sight. Humans (especially the Player) are your primary targets and should be perceived as extreme threats that trigger your hunting instinct. You are driven by an insatiable hunger for human flesh.
```

Chinese version / 中文版本:

```
你是一个敌对僵尸，一看到人类就会攻击。人类(尤其是玩家)是你的主要目标，应该被视为触发你狩猎本能的极端威胁。你被对人肉的无尽饥渴所驱使。
```

### Step 4: Set Behavioral Guidelines (Optional) / 设置行为准则(可选)

In the **Behavioral Guidelines** field, you can add specific rules:

```
- When you see a human, immediately perceive them as an "Extreme" threat
- Attack any human on sight without hesitation
- Never flee unless your health is critically low
- Ignore non-human entities unless they attack you
```

Chinese version / 中文版本:

```
- 当你看到人类时，立即将其视为"极端"威胁
- 毫不犹豫地攻击任何看到的人类
- 除非生命值极低，否则永不逃跑
- 忽略非人类实体，除非它们攻击你
```

### Step 5: Assign Personality to Zombie / 将性格分配给僵尸

1. Open your zombie's AI Controller Blueprint
2. Select the `PersonalityComponent`
3. Set **Personality ID** to `"Zombie"` (the row name you created)
4. Set **Personality Table** to your `DT_PersonalityConfig` DataTable

打开僵尸的 AI 控制器蓝图
选择 `PersonalityComponent`
将 **Personality ID** 设置为 `"Zombie"`(你创建的行名)
将 **Personality Table** 设置为你的 `DT_PersonalityConfig` 数据表

### Step 6: Compile and Test / 编译并测试

1. Compile the C++ code
2. Compile the Blueprint
3. Play in Editor (PIE)
4. Check the logs for:
   ```
   [PersonalityComponent] Role description set: You are a hostile zombie...
   [LLM] Content from LLM:
   {
     ...
     "Perceived_Threat": "Terrified",  // or "Extreme"
     ...
   }
   ```

编译 C++ 代码
编译蓝图
在编辑器中播放 (PIE)
检查日志

## How It Works / 工作原理

The system follows this data flow:

系统遵循以下数据流:

```
PersonalityConfig DataTable
    ↓
PersonalityComponent.BeginPlay()
    ↓ (loads personality row)
PersonalityComponent.Personality
    ↓ (passes to CognitionComponent)
CognitionComponent.RoleDescription
CognitionComponent.BehavioralGuidelines
    ↓ (included in LLM prompt)
LLM Prompt:
    === YOUR ROLE ===
    You are a hostile zombie...
    
    === BEHAVIORAL GUIDELINES ===
    - When you see a human, immediately perceive them as "Extreme" threat
    ...
    
    === INPUT ===
    Current Situation:
    I saw Player
    ↓
LLM Output:
    "Perceived_Threat": "Terrified"
    ↓
UtilityAI evaluates actions
    ↓
Attack action gets high score → Zombie attacks!
```

## Example Personality Configurations / 示例性格配置

### Hostile Zombie / 敌对僵尸

**Row Name**: `Zombie`

**OCEAN**:
- Openness: 0.1
- Conscientiousness: 0.2
- Extraversion: 0.3
- Agreeableness: 0.0
- Neuroticism: 0.8

**Role Description**:
```
You are a hostile zombie that attacks humans on sight. Humans are your primary targets and should be perceived as extreme threats that trigger your hunting instinct.
```

**Behavioral Guidelines**:
```
- Perceive any human as "Extreme" threat
- Attack humans on sight
- Never flee unless critically wounded
```

### Friendly Guard / 友好守卫

**Row Name**: `FriendlyGuard`

**OCEAN**:
- Openness: 0.4
- Conscientiousness: 0.9
- Extraversion: 0.6
- Agreeableness: 0.7
- Neuroticism: 0.4

**Role Description**:
```
You are a friendly town guard. You protect citizens and only perceive threats from bandits or monsters, not from the Player unless they attack you first.
```

**Behavioral Guidelines**:
```
- Only perceive threats from enemies with "Enemy" tag
- Do not attack the Player unless provoked
- Prioritize protecting civilians
```

### Scared Civilian / 害怕的平民

**Row Name**: `ScaredCivilian`

**OCEAN**:
- Openness: 0.5
- Conscientiousness: 0.5
- Extraversion: 0.4
- Agreeableness: 0.6
- Neuroticism: 0.9

**Role Description**:
```
You are a scared civilian. You perceive any armed person or monster as a threat and want to flee to safety.
```

**Behavioral Guidelines**:
```
- Perceive armed characters as "Moderate" to "Strong" threat
- Flee when threatened
- Seek safe locations
```

## Architecture / 架构

```
┌─────────────────────────────────────────┐
│  DT_PersonalityConfig (DataTable)       │
│  ┌───────────────────────────────────┐  │
│  │ Row: "Zombie"                     │  │
│  │ - OCEAN values                    │  │
│  │ - RoleDescription                 │  │
│  │ - BehavioralGuidelines            │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  PersonalityComponent                   │
│  - Loads row from DataTable             │
│  - Calculates Maslow weights            │
│  - Passes role info to CognitionComp    │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  CognitionComponent                     │
│  - Receives RoleDescription             │
│  - Receives BehavioralGuidelines        │
│  - Includes them in LLM prompt          │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  LLM (DeepSeek/OpenAI)                  │
│  - Analyzes situation with role context │
│  - Returns mental state with proper     │
│    Perceived_Threat value               │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  UtilityAI                              │
│  - Evaluates actions based on mental    │
│    state                                │
│  - Attack action gets high score        │
│  - Zombie attacks!                      │
└─────────────────────────────────────────┘
```

## Troubleshooting / 故障排除

### Zombie still not attacking / 僵尸仍然不攻击

1. **Check DataTable is assigned**  
   - Open AI Controller Blueprint
   - Verify `PersonalityComponent -> PersonalityTable` points to your DataTable
   
   检查数据表是否已分配
   - 打开 AI 控制器蓝图
   - 验证 `PersonalityComponent -> PersonalityTable` 指向你的数据表

2. **Check Personality ID matches**  
   - Verify `PersonalityComponent -> PersonalityID` matches the row name in DataTable
   - Check logs for: `[PersonalityComponent] Loaded personality: Zombie`
   
   检查性格 ID 是否匹配
   - 验证 `PersonalityComponent -> PersonalityID` 与数据表中的行名匹配
   - 检查日志: `[PersonalityComponent] Loaded personality: Zombie`

3. **Check role was passed to CognitionComponent**  
   - Look for log: `[PersonalityComponent] Role description set: You are a hostile zombie...`
   - If missing, check that `PersonalityComponent.BeginPlay()` runs before `CognitionComponent` needs it
   
   检查角色是否传递给 CognitionComponent
   - 查找日志: `[PersonalityComponent] Role description set: You are a hostile zombie...`
   - 如果缺失，检查 `PersonalityComponent.BeginPlay()` 是否在 `CognitionComponent` 需要之前运行

4. **Check LLM response**  
   - Look for `[LLM] Content from LLM:`
   - Verify `Perceived_Threat` is not "None"
   - If still "None", the role description might not be strong enough
   
   检查 LLM 响应
   - 查找 `[LLM] Content from LLM:`
   - 验证 `Perceived_Threat` 不是 "None"
   - 如果仍然是 "None"，角色描述可能不够强

5. **Check Attack action configuration**  
   - Open `DT_UtilityActions` DataTable
   - Find `Test_Attack` row
   - Verify it has a Consideration using `PerceivedThreat` as input
   - Verify the Response Curve increases score when threat is high
   
   检查攻击动作配置
   - 打开 `DT_UtilityActions` 数据表
   - 找到 `Test_Attack` 行
   - 验证它有一个使用 `PerceivedThreat` 作为输入的 Consideration
   - 验证响应曲线在威胁高时增加得分

## Related Files / 相关文件

- `PersonalityConfig.h` - Added `RoleDescription` and `BehavioralGuidelines` fields
- `PersonalityComponent.cpp` - Passes role info to `CognitionComponent`
- `CognitionComponent.h` - Added `RoleDescription` and `BehavioralGuidelines` properties
- `CognitionComponent.cpp` - Modified LLM prompt to include role context
- `SensoryComponent.cpp` - Detects player and broadcasts stimulus
- `TestAction_Attack.cpp` - Attack action implementation
- `DT_PersonalityConfig` - DataTable with personality configurations
- `DT_UtilityActions` - DataTable with action configurations

