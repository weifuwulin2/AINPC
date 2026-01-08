# AI Character Setup Guide / AI 角色设置指南

## 概述 / Overview

所有使用 UtilityAI 的 NPC 都应该继承 `AICharacterBase` 类，这样可以在 Blueprint 中直接设置 `PersonalityID`。

All NPCs using UtilityAI should inherit from `AICharacterBase` class, allowing you to set `PersonalityID` directly in Blueprint.

## 设置步骤 / Setup Steps

### Step 1: 让你的 NPC Blueprint 继承 AICharacterBase

#### 方法 A: 创建新的 NPC Blueprint

1. 在 Content Browser 中右键
2. 选择 **Blueprint Class**
3. 选择 **AICharacterBase** 作为父类 ← 重要！
4. 命名为 `BP_Zombie`（或其他名字）

#### 方法 B: 修改现有的 NPC Blueprint

1. 打开现有的 `BP_Zombie`
2. 点击工具栏的 **File** → **Reparent Blueprint**
3. 选择 `AICharacterBase` 作为新的父类
4. 保存

### Step 2: 设置 PersonalityID

1. 打开 `BP_Zombie`
2. 点击工具栏的 **Class Defaults** 按钮
3. 在 Details 面板中找到 **AI | Personality** 分类
4. 设置：
   ```
   AI | Personality:
   ├── Personality ID: Zombie  ← 设置这个！
   └── Auto Set Personality: ✅ (勾选)
   ```

5. 在 **Pawn** 分类中设置：
   ```
   Pawn:
   └── AI Controller Class: BP_UtilityAIController
   ```

6. 编译并保存

### Step 3: 对每种 NPC 类型重复

**BP_Zombie:**
```
Class Defaults:
├── AI Controller Class: BP_UtilityAIController
└── Personality ID: Zombie
```

**BP_Merchant:**
```
Class Defaults:
├── AI Controller Class: BP_UtilityAIController
└── Personality ID: Merchant
```

**BP_Guard:**
```
Class Defaults:
├── AI Controller Class: BP_UtilityAIController
└── Personality ID: Guard
```

---

## 工作原理 / How It Works

### 自动传递流程 / Automatic Transfer Flow

```
1. NPC Spawns (例如 BP_Zombie)
    ↓
2. Unreal 自动创建 UtilityAIController 实例
    ↓
3. Controller Possess Pawn
    ↓
4. Pawn's BeginPlay 触发
    ↓
5. AICharacterBase::BeginPlay() 执行
    ↓
6. 读取 self.PersonalityID (例如 "Zombie")
    ↓
7. 获取 AI Controller 的 PersonalityComp
    ↓
8. 设置 PersonalityComp->PersonalityID = "Zombie"
    ↓
9. 调用 RecalculateWeights() 重新加载配置
    ↓
10. 完成！NPC 现在使用 "Zombie" 的性格配置
```

### C++ 代码（自动执行）

```cpp
void AAICharacterBase::BeginPlay()
{
    Super::BeginPlay();
    
    if (bAutoSetPersonality)
    {
        AUtilityAIController* AIController = Cast<AUtilityAIController>(GetController());
        if (AIController && AIController->PersonalityComp)
        {
            AIController->PersonalityComp->PersonalityID = PersonalityID;
            AIController->PersonalityComp->RecalculateWeights();
        }
    }
}
```

---

## 优势 / Advantages

### ✅ 相比手动在 Blueprint 中写代码

**之前（需要手动写蓝图代码）：**
```blueprint
Event BeginPlay
    ↓
Get AI Controller
    ↓
Cast to UtilityAIController
    ↓
Get PersonalityComp
    ↓
Set PersonalityID = "Zombie"
```

**现在（只需设置变量）：**
```
Class Defaults:
└── Personality ID: Zombie  ← 只需这一步！
```

### ✅ 好处

- ✅ **更简单** - 只需在 Class Defaults 中设置一个变量
- ✅ **更清晰** - 不需要写蓝图代码
- ✅ **更安全** - C++ 代码自动处理，不会出错
- ✅ **更易维护** - 所有 NPC 使用相同的逻辑

---

## 验证设置 / Verify Setup

### 检查继承关系

在 Blueprint 编辑器中：
1. 打开 `BP_Zombie`
2. 查看右上角的 **Parent Class**
3. 应该显示：`Parent Class: AICharacterBase`

### 检查日志

运行游戏后，在 Output Log 中搜索：
```
[AICharacter]
```

你应该看到：
```
[AICharacter] Set PersonalityID 'Zombie' for BP_Zombie_C_0
[AICharacter] Set PersonalityID 'Merchant' for BP_Merchant_C_0
```

---

## 常见问题 / Troubleshooting

### Q: 找不到 AICharacterBase 类？

**A:** 编译 C++ 代码：
1. 关闭 Unreal Editor
2. 在 Visual Studio 中编译项目
3. 重新打开 Unreal Editor

### Q: Reparent Blueprint 后出现错误？

**A:** 这是正常的，因为父类变了。解决方法：
1. 保存 Blueprint
2. 关闭并重新打开 Blueprint
3. 如果还有错误，检查是否有使用了旧父类特有的功能

### Q: PersonalityID 没有生效？

**A:** 检查：
1. ✅ `Auto Set Personality` 是否勾选
2. ✅ `AI Controller Class` 是否设置为 `BP_UtilityAIController`
3. ✅ `DT_PersonalityConfig` 中是否有对应的 Row Name
4. ✅ 查看日志是否有 `[AICharacter]` 相关消息

---

## 完整配置清单 / Complete Checklist

- [ ] 编译 C++ 代码（添加了 AICharacterBase）
- [ ] 创建或修改 NPC Blueprint，继承 `AICharacterBase`
- [ ] 在 Class Defaults 中设置 `Personality ID`
- [ ] 在 Class Defaults 中设置 `AI Controller Class` = `BP_UtilityAIController`
- [ ] 确保 `Auto Set Personality` 勾选
- [ ] 在 `DT_PersonalityConfig` 中添加对应的配置
- [ ] 在 `BP_UtilityAIController` 中设置 `Personality Table` = `DT_PersonalityConfig`
- [ ] 测试：运行游戏，检查日志

---

## 示例配置 / Example Configuration

### DT_PersonalityConfig

| Row Name | RoleDescription | BehavioralGuidelines | Openness | Agreeableness |
|----------|----------------|---------------------|----------|---------------|
| Zombie | You are a hostile zombie... | Attack humans, flee when weak | 0.1 | 0.0 |
| Merchant | You are a friendly merchant... | Greet customers, sell items | 0.7 | 0.9 |
| Guard | You are a town guard... | Patrol, protect citizens | 0.5 | 0.6 |

### BP_Zombie (Class Defaults)

```
AI | Personality:
├── Personality ID: Zombie
└── Auto Set Personality: ✅

Pawn:
└── AI Controller Class: BP_UtilityAIController
```

### BP_Merchant (Class Defaults)

```
AI | Personality:
├── Personality ID: Merchant
└── Auto Set Personality: ✅

Pawn:
└── AI Controller Class: BP_UtilityAIController
```

---

**现在你可以在 Class Defaults 中直接设置 PersonalityID 了！** ✨

