# 兽人解放战 - 实施指南

## 📋 场景设计

### 场景1️⃣：庄园压迫（Manor_Oppression）
**ID**: `Scene_Manor_Oppression`  
**状态**: 初始状态  
**位置**: 庄园（NarrativeSceneAnchor）

**角色配置**：
- **领主** × 1 (Human, Personality: `Tyrant_Lord`)
  - 行为：享受美食、发表演讲
  - 对白：基于PersonalityComponent + Plot Context生成
  
- **护卫** × 3 (Human, Personality: 随机`Brave_Guard` / `Coward_Guard`)
  - 勇敢护卫：守门、巡逻
  - 胆小护卫：跟随领主
  
- **兽人奴隶** × 5 (Orc, Personality: `Oppressed_Slave`)
  - 行为：在铁笼里
  - 状态：虚弱、恐惧

**Plot Outline**:
```
"The cruel Human Lord rules over this manor with an iron fist. 
Orc slaves are imprisoned in cages, suffering under brutal oppression. 
Guards patrol the grounds, ready to crush any sign of rebellion."
```

---

### 场景2️⃣：庄园解放（Manor_Liberation）
**ID**: `Scene_Manor_Liberation`  
**状态**: 玩家帮助兽人后切换  
**位置**: 同一个庄园

**角色配置**：
- **兽人领袖** × 1 (Orc, Personality: `Grateful_Leader`)
  - 行为：管理庄园、感谢玩家
  - 对白："You are our savior! This manor is now our home."

- **兽人战士** × 3 (Orc, Personality: `Loyal_Warrior`)
  - 行为：守卫庄园、采集食物

**Plot Outline**:
```
"The tyrant lord has been overthrown! The Orcs have claimed this manor as their own. 
They live peacefully here, grateful to their liberator."
```

---

### 场景3️⃣：破败据点（Rebel_Camp）
**ID**: `Scene_Rebel_Camp`  
**状态**: 始终存在（野外）  
**位置**: 野外NarrativeSceneAnchor

**角色配置**：
- **兽人反抗者** × 2 (Orc, Personality: `Suspicious_Rebel`)
  - 行为：巡逻、采集食物
  - 对白："Who are you? Are you one of the Lord's dogs?!"

**Plot Outline**:
```
"Escaped Orc slaves have established a ramshackle camp in the wilderness. 
They distrust strangers and prepare to raid the manor."
```

---

## 🔧 实施步骤

### 步骤1：配置NPC模板（DT_NPCDefinitions.json）

添加以下NPC模板：

```json
{
    "Name": "Narrative_Human_Lord",
    "PersonalityID": "Tyrant_Lord",
    "ProfessionID": "Noble",
    "FactionID": "Human",
    "PawnClass": "/Game/Blueprints/BP_HumanNPC.BP_HumanNPC_C"
},
{
    "Name": "Narrative_Human_Guard_Brave",
    "PersonalityID": "Brave_Guard",
    "ProfessionID": "Soldier",
    "FactionID": "Human",
    "PawnClass": "/Game/Blueprints/BP_HumanNPC.BP_HumanNPC_C"
},
{
    "Name": "Narrative_Human_Guard_Coward",
    "PersonalityID": "Coward_Guard",
    "ProfessionID": "Soldier",
    "FactionID": "Human",
    "PawnClass": "/Game/Blueprints/BP_HumanNPC.BP_HumanNPC_C"
},
{
    "Name": "Narrative_Orc_Slave",
    "PersonalityID": "Oppressed_Slave",
    "ProfessionID": "Laborer",
    "FactionID": "Orc",
    "PawnClass": "/Game/Blueprints/BP_OrcNPC.BP_OrcNPC_C"
},
{
    "Name": "Narrative_Orc_Leader",
    "PersonalityID": "Grateful_Leader",
    "ProfessionID": "Chieftain",
    "FactionID": "Orc",
    "PawnClass": "/Game/Blueprints/BP_OrcNPC.BP_OrcNPC_C"
},
{
    "Name": "Narrative_Orc_Rebel",
    "PersonalityID": "Suspicious_Rebel",
    "ProfessionID": "Warrior",
    "FactionID": "Orc",
    "PawnClass": "/Game/Blueprints/BP_OrcNPC.BP_OrcNPC_C"
}
```

---

### 步骤2：配置Personality（DT_Personalities.json）

添加以下Personality：

```json
{
    "PersonalityID": "Tyrant_Lord",
    "RoleDescription": "You are a cruel and arrogant noble who despises non-humans.",
    "BehavioralGuidelines": "Show contempt for Orcs. Demand absolute obedience. Beg for mercy if threatened.",
    "OCEAN": {
        "Openness": 0.3,
        "Conscientiousness": 0.6,
        "Extraversion": 0.7,
        "Agreeableness": 0.1,
        "Neuroticism": 0.5
    }
},
{
    "PersonalityID": "Brave_Guard",
    "RoleDescription": "You are a loyal guard who will fight to the death for your lord.",
    "BehavioralGuidelines": "Attack intruders. Protect the lord. Show no mercy.",
    "OCEAN": {
        "Openness": 0.4,
        "Conscientiousness": 0.8,
        "Extraversion": 0.5,
        "Agreeableness": 0.3,
        "Neuroticism": 0.2
    }
},
{
    "PersonalityID": "Coward_Guard",
    "RoleDescription": "You are a cowardly guard who values your own life above all.",
    "BehavioralGuidelines": "Flee if threatened. Beg for mercy. Betray your lord if necessary.",
    "OCEAN": {
        "Openness": 0.4,
        "Conscientiousness": 0.4,
        "Extraversion": 0.3,
        "Agreeableness": 0.5,
        "Neuroticism": 0.8
    }
},
{
    "PersonalityID": "Oppressed_Slave",
    "RoleDescription": "You are a weak and frightened Orc slave, desperate for freedom.",
    "BehavioralGuidelines": "Show fear of humans. Beg for help. Express gratitude to liberators.",
    "OCEAN": {
        "Openness": 0.5,
        "Conscientiousness": 0.6,
        "Extraversion": 0.3,
        "Agreeableness": 0.7,
        "Neuroticism": 0.9
    }
},
{
    "PersonalityID": "Grateful_Leader",
    "RoleDescription": "You are an Orc leader who is deeply grateful to the player for liberation.",
    "BehavioralGuidelines": "Show gratitude. Offer gifts. Protect allies.",
    "OCEAN": {
        "Openness": 0.7,
        "Conscientiousness": 0.7,
        "Extraversion": 0.6,
        "Agreeableness": 0.8,
        "Neuroticism": 0.3
    }
},
{
    "PersonalityID": "Suspicious_Rebel",
    "RoleDescription": "You are a suspicious Orc rebel who distrusts all strangers.",
    "BehavioralGuidelines": "Question strangers. Attack if threatened. Seek revenge against humans.",
    "OCEAN": {
        "Openness": 0.4,
        "Conscientiousness": 0.6,
        "Extraversion": 0.4,
        "Agreeableness": 0.3,
        "Neuroticism": 0.6
    }
}
```

---

### 步骤3：配置场景（DT_NarrativeScenes.json）

更新你的`DT_NarrativeScenes.json`，将`Scene_OrcRescue`改为更详细的版本，并添加其他场景。

---

### 步骤4：场景切换逻辑（Blueprint或C++）

**方案A：Blueprint实现（简单）**

在`BP_GameMode`或`BP_PlayerController`中：

```
Event: OnActorKilled(Actor KilledActor)
├─ Check: Is KilledActor tagged as "Lord"?
│   └─ Yes → Call NarrativeSquadSubsystem::SwitchScene
│       ├─ Destroy "Scene_Manor_Oppression"
│       └─ Spawn "Scene_Manor_Liberation"
```

**方案B：C++实现（推荐）**

创建一个新的Subsystem：

```cpp
// StoryEventSubsystem.h
class UStoryEventSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
    
public:
    // 监听NPC死亡
    UFUNCTION()
    void OnNPCDied(AActor* DeadNPC);
    
    // 切换场景
    void TransitionToLiberation();
};
```

---

## 🎯 核心功能实现

### 功能1：领主求饶

**实现方式**：基于`HealthComponent`触发对话

```cpp
// 在HealthComponent中
if (Health < MaxHealth * 0.2f) // 低于20%血量
{
    // 触发求饶对话
    if (CognitionComponent)
    {
        CognitionComponent->ProcessStimulus(
            "I'm about to die! I must beg for mercy and offer money!"
        );
    }
}
```

**LLM会自动生成**："等……等一下！这位好汉！有什么话好商量，你要多少金币我都给！"

---

### 功能2：护卫逃跑

**实现方式**：使用现有的`UtilityAI` + `Action_Flee`

在`DT_Professions.json`中为`Coward_Guard`:
```json
{
    "ProfessionID": "Coward_Guard_Profession",
    "Schedule": [
        {
            "StartTime": 0.0,
            "TaskTag": "Directive.Survival" // 总是处于生存模式
        }
    ]
}
```

---

### 功能3：奴隶感谢玩家

**实现方式**：基于`FactionReputationComponent`

当领主死亡时：
```cpp
// 提升玩家在Orc阵营的声望
FactionReputationComponent->ModifyFactionReputation("Orc", +50.0f);

// 兽人奴隶的CognitionComponent会感知到
// LLM自动生成感谢对话
```

---

## ✅ 验证清单

实施后测试：

- [ ] 场景1：庄园压迫场景正确生成
- [ ] 领主在血量低时说"求饶"对话
- [ ] 勇敢护卫攻击玩家
- [ ] 胆小护卫逃跑
- [ ] 领主死亡后场景切换到"庄园解放"
- [ ] 兽人奴隶变成兽人领袖/战士
- [ ] 兽人NPC对玩家友好
- [ ] 破败据点中的反抗者对玩家警惕

---

## 📊 工作量估算

| 任务 | 时间 | 难度 |
|------|------|------|
| 配置NPC模板 | 1小时 | ⭐️ |
| 配置Personality | 2小时 | ⭐️ |
| 配置场景 | 2小时 | ⭐️⭐️ |
| 场景切换逻辑 | 3小时 | ⭐️⭐️⭐️ |
| 测试和调优 | 4小时 | ⭐️⭐️ |
| **总计** | **12小时** | **1-2天** |

---

## 🚀 下一步

1. **现在**：我可以帮你生成完整的JSON配置文件
2. **然后**：实现场景切换逻辑
3. **最后**：测试和调优

**要我现在帮你生成完整的配置文件吗？** 🎮
