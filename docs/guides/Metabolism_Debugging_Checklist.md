# Metabolism 调试检查清单

## 🔍 问题排查步骤

### 1. 检查 Metabolism 是否在运行
**打开 Output Log，搜索 "Metabolism"**

预期看到：
```
[UtilityAIController_X] Metabolism ENABLED (Faction: Human)
[Metabolism] Hunger: 0.15...
[Metabolism] Energy: 0.08...
```

❌ **如果没看到**：
- Metabolism 可能被禁用了
- PersonalityComponent 的 Faction 可能是 Monster

### 2. 检查 Hunger/Energy 是否在增长
**搜索 "Hunger" 或 "Energy"**

预期看到数值逐渐增长：
```
[Metabolism] Hunger: 0.001
[Metabolism] Hunger: 0.005
[Metabolism] Hunger: 0.010
```

❌ **如果数值不变或没有日志**：
- MetabolismComponent 的 Tick 可能被禁用
- HungerRate/EnergyRate 可能设置为 0

### 3. 检查 UtilityAI 是否评估了 Action_Eat
**搜索 "Action_Eat" 或 "UtilityAI"**

预期看到：
```
[UtilityAI] Evaluating actions...
[UtilityAI] Action_Eat score: 0.05
[UtilityAI] Action_Idle score: 1.2
[UtilityAI] Best Action: Test_Idle
```

❌ **如果没看到 Action_Eat**：
- DataTable 中可能没有配置 Action_Eat
- 或者 ActionClass 配置错误

### 4. 检查 HasFoodNearby 是否返回 1.0
**在 UtilityActionBase.cpp 的 GetConsiderationValue 中添加临时日志**

找到 `case EUtilityInputType::HasFoodNearby:` 这一段，确认它是否找到了食物。

### 5. 检查 Smart Object 是否正确配置
**在场景中选中食物对象，检查：**
- ✅ 有 SmartObjectComponent
- ✅ Semantics.SocialTag = `Activity.Eat`
- ✅ RestoreValue > 0

## 🛠️ 快速修复方案

### 方案 A: 加速测试（推荐）
临时提高 Hunger 增长速率，快速看到效果：

在 MetabolismComponent 的 Details 面板：
```
HungerRate: 0.5  (原来是 0.0005)
EnergyRate: 0.3  (原来是 0.0003)
```

这样几秒钟就会饿了。

### 方案 B: 手动设置初始 Hunger
在 BeginPlay 时直接设置高 Hunger：

```cpp
// 在 UtilityAIController::BeginPlay 末尾添加
if (MentalState)
{
    MentalState->Hunger = 0.8f;  // 测试用
    MentalState->Energy = 0.7f;  // 测试用
}
```

### 方案 C: 检查 DataTable 配置
确保 `DT_UtilityActions` 中有这一行：

```
Row Name: Action_Eat
ActionClass: UAction_SmartObject
SmartObjectTag: Activity.Eat
BaseReward: 2.0
Considerations[0]:
  InputType: Hunger
  ConsiderationType: Motivation
  CurveType: Logistic
Considerations[1]:
  InputType: HasFoodNearby
  ConsiderationType: Context
  CurveType: TargetThreshold
```

## 📊 常见问题

### Q1: NPC 一直 Idle，不去吃饭
**原因**：
- Hunger 还不够高（需要 > 0.5 才会触发）
- 附近没有食物（HasFoodNearby = 0）
- Action_Eat 的 BaseReward 太低

**解决**：
- 提高 HungerRate 加速测试
- 确保场景中有食物
- 提高 Action_Eat 的 BaseReward 到 5.0

### Q2: NPC 去了食物附近但不吃
**原因**：
- SmartObject 的 Tag 不对
- Action_SmartObject 的 Execute 没有被调用

**解决**：
- 检查 Tag 是否完全匹配 `Activity.Eat`
- 查看日志是否有 "Moving to Smart Object"

### Q3: NPC 吃了但 Hunger 不降
**原因**：
- RestoreValue = 0
- RestoreStats 没有被调用

**解决**：
- 设置 RestoreValue > 0
- 查看日志是否有 "Eating... Hunger: X.XX"

## 🔧 调试代码（临时添加）

在 `MetabolismComponent::TickComponent` 开头添加：
```cpp
// 每 2 秒打印一次
static float LastLogTime = 0.0f;
float CurrentTime = GetWorld()->GetTimeSeconds();
if (CurrentTime - LastLogTime > 2.0f)
{
    UE_LOG(LogTemp, Warning, TEXT("[Metabolism DEBUG] Hunger: %.3f, Energy: %.3f"), 
           State->Hunger, State->Energy);
    LastLogTime = CurrentTime;
}
```

在 `UtilityActionBase::GetConsiderationValue` 的 `HasFoodNearby` case 中添加：
```cpp
case EUtilityInputType::HasFoodNearby:
{
    if (USensoryComponent* Sensory = Controller->FindComponentByClass<USensoryComponent>())
    {
        AActor* Food = Sensory->FindBestSmartObject(FGameplayTag::RequestGameplayTag("Activity.Eat"));
        UE_LOG(LogTemp, Warning, TEXT("[DEBUG] HasFoodNearby: %s"), Food ? TEXT("YES (1.0)") : TEXT("NO (0.0)"));
        return Food ? 1.0f : 0.0f;
    }
    return 0.0f;
}
```
