# ⚠️ UENUM 宏限制说明

**问题:** UE的反射系统不支持在 UENUM 中使用宏展开  
**影响:** 枚举值需要手动添加  
**解决方案:** Switch case 仍然自动生成

---

## 🐛 问题说明

### 编译错误
```
Error: Found '(' when expecting ',' or '}' while parsing UENUM
```

### 原因
UE的 **Unreal Header Tool (UHT)** 在预处理阶段解析 UENUM，此时宏还没有展开。

```cpp
// ❌ 不支持
UENUM(BlueprintType)
enum class EUtilityInputType : uint8
{
    #define DECLARE_ENUM(Name, ...) Name,
    MENTAL_STATE_FIELDS(DECLARE_ENUM)  // UHT 看不懂这个
    #undef DECLARE_ENUM
};
```

---

## ✅ 当前解决方案

### 半自动化方案

**需要手动修改的地方 (2个):**
1. `MentalStateFields.h` - 定义属性
2. `EUtilityInputType` - 添加枚举值

**自动生成的地方 (7个):**
1. ✅ `FMentalState` 字段
2. ✅ `UNPCMentalState` 属性
3. ✅ 构造函数初始化
4. ✅ 转换函数
5. ✅ LLM Prompt
6. ✅ `GetConsiderationValue()` switch case ← **仍然自动**
7. ✅ 其他所有代码

---

## 📝 添加新属性步骤

### 添加 "Boredom" 示例

**步骤1:** 修改 `MentalStateFields.h`
```cpp
#define MENTAL_STATE_FIELDS(FIELD) \
    FIELD(Anger,         0.0f, "Anger",          "愤怒值") \
    FIELD(Fear,          0.0f, "Fear",           "恐惧值") \
    FIELD(Confidence,    0.5f, "Confidence",     "自信值") \
    FIELD(SocialBattery, 0.8f, "Social Battery", "社交电量") \
    FIELD(Hunger,        0.0f, "Hunger",         "饥饿值") \
    FIELD(Boredom,       0.0f, "Boredom",        "无聊值")  // ✅ 添加
```

**步骤2:** 修改 `UtilityActionBase.h`
```cpp
UENUM(BlueprintType)
enum class EUtilityInputType : uint8
{
    Anger,
    Fear,
    Confidence,
    SocialBattery,
    Hunger,
    Boredom,  // ✅ 添加
    
    // 环境字段...
    SelfHealth,
    TargetHealth,
    // ...
};
```

**步骤3:** 重新编译

**自动更新:**
- ✅ `FMentalState` 有 `Boredom` 字段
- ✅ `UNPCMentalState` 有 `Boredom` 属性
- ✅ 构造函数初始化 `Boredom`
- ✅ 转换函数处理 `Boredom`
- ✅ LLM Prompt 包含 `Boredom`
- ✅ `GetConsiderationValue()` 自动处理 `Boredom` ← **重要！**
- ✅ DataTable 中可以选择 `Boredom`

---

## 🎯 优势

### 仍然比完全手动好很多

**修改前 (完全手动):**
- 需要修改 8 个地方

**修改后 (半自动):**
- 需要修改 2 个地方
- Switch case 自动生成 ✅
- 其他代码自动生成 ✅

**减少了 75% 的手动工作！** 🎉

---

## 💡 为什么 Switch Case 仍然自动？

### Switch Case 在 .cpp 文件中

```cpp
// UtilityActionBase.cpp
float GetConsiderationValue(EUtilityInputType InputType, ...)
{
    switch (InputType)
    {
        // ✅ 这里可以使用宏，因为在 .cpp 文件中
        #define HANDLE_FIELD(Name, ...) \
            case EUtilityInputType::Name: \
                return State ? State->Name : 0.0f;
        
        MENTAL_STATE_FIELDS(HANDLE_FIELD)
        
        #undef HANDLE_FIELD
        
        // 环境字段...
    }
}
```

**.cpp 文件不受 UHT 限制**，可以正常使用宏！

---

## 🔍 其他尝试过的方案

### 方案A: 使用字符串映射 (放弃)

**优点:**
- 完全动态
- 不需要枚举

**缺点:**
- ❌ 失去类型安全
- ❌ 性能略低
- ❌ 编辑器体验差

### 方案B: 使用反射系统 (太复杂)

**优点:**
- 完全自动化

**缺点:**
- ❌ 实现复杂
- ❌ 运行时开销
- ❌ 需要自定义反射

### 方案C: 当前方案 (最佳平衡)

**优点:**
- ✅ 类型安全
- ✅ 性能最优
- ✅ Switch case 自动生成
- ✅ 大部分代码自动生成

**缺点:**
- ⚠️ 枚举需要手动添加

---

## 📊 对比

| 特性 | 完全手动 | 当前方案 | 理想方案 |
|------|----------|----------|----------|
| 修改点数 | 8个 | 2个 ✅ | 1个 |
| Switch自动 | ❌ | ✅ | ✅ |
| 类型安全 | ✅ | ✅ | ✅ |
| 性能 | ✅ | ✅ | ✅ |
| 实现难度 | 简单 | 简单 | 不可能 |
| UE兼容性 | ✅ | ✅ | ❌ |

---

## ✅ 最佳实践

### 添加新属性时

1. **先修改 `MentalStateFields.h`**
   ```cpp
   FIELD(NewEmotion, 0.0f, "New Emotion", "描述")
   ```

2. **再修改 `EUtilityInputType`**
   ```cpp
   enum class EUtilityInputType : uint8 {
       // ...
       NewEmotion,  // 添加这里
       // ...
   };
   ```

3. **编译**
   - Switch case 自动更新 ✅
   - 其他代码自动更新 ✅

### 验证清单

- [ ] `MentalStateFields.h` 已添加
- [ ] `EUtilityInputType` 已添加
- [ ] 编译成功
- [ ] DataTable 中可以选择新枚举
- [ ] Switch case 自动处理新字段

---

## 🎉 总结

虽然 UENUM 不支持宏，但我们仍然实现了：

### ✅ 自动生成 (7个地方)
1. `FMentalState` 字段
2. `UNPCMentalState` 属性
3. 构造函数
4. 转换函数
5. LLM Prompt
6. **Switch case** ← 最重要！
7. 其他代码

### ⚠️ 需要手动 (2个地方)
1. `MentalStateFields.h` - 配置
2. `EUtilityInputType` - 枚举

**仍然减少了 75% 的手动工作！** 🎉

---

## 📁 相关文档

- `MentalStateFields.h` - 配置文件
- `UtilityActionBase.h` - 枚举定义
- `Single_Point_Configuration_Guide.md` - 使用指南

---

**记住:** 虽然不是完美的单点配置，但已经是 UE 限制下的最佳方案了！
