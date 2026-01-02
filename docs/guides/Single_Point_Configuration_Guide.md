# ✅ 单点配置系统 - 使用指南

**完成时间:** 2026-01-03 00:52  
**功能:** 只需修改一个文件即可管理所有情绪属性  
**状态:** ✅ 已实现并可用

---

## 🎯 核心优势

### ✅ 只需修改一个地方！

现在添加/删除/修改情绪属性，**只需编辑一个文件**：

```
Source/AINPC/Public/UtilityAI/MentalStateFields.h
```

所有其他代码**自动同步更新**：
- ✅ `FMentalState` 结构体字段
- ✅ `UNPCMentalState` 类属性
- ✅ 初始化代码
- ✅ 重置代码
- ✅ 转换函数
- ✅ LLM Prompt 字段列表

---

## 📝 如何添加新属性

### 步骤1: 打开配置文件

```
Source/AINPC/Public/UtilityAI/MentalStateFields.h
```

### 步骤2: 添加一行

```cpp
#define MENTAL_STATE_FIELDS(FIELD) \
	FIELD(Anger,         0.0f, "Anger",          "愤怒值，影响攻击欲望") \
	FIELD(Fear,          0.0f, "Fear",           "恐惧值，影响逃跑欲望") \
	FIELD(Confidence,    0.5f, "Confidence",     "自信值，影响战斗策略") \
	FIELD(SocialBattery, 0.8f, "Social Battery", "社交电量，影响对话欲望") \
	FIELD(Hunger,        0.0f, "Hunger",         "饥饿值，影响觅食行为") \
	FIELD(Boredom,       0.0f, "Boredom",        "无聊值，影响探索欲望")  // ✅ 新增！
```

### 步骤3: 重新编译

```bash
# 在 UE 编辑器中
Build -> Compile AINPC
```

### 完成！

所有代码自动更新：
- ✅ 结构体有了 `Boredom` 字段
- ✅ 类有了 `Boredom` 属性
- ✅ 转换函数自动处理 `Boredom`
- ✅ LLM Prompt 包含 `Boredom`
- ✅ 蓝图中可以访问 `Boredom`

---

## 📋 字段格式说明

```cpp
FIELD(字段名, 默认值, 显示名称, 描述)
```

### 参数说明

| 参数 | 说明 | 示例 |
|------|------|------|
| **字段名** | C++变量名（驼峰命名） | `Boredom` |
| **默认值** | 初始值（0.0-1.0） | `0.0f` |
| **显示名称** | 编辑器中显示的名字 | `"Boredom"` |
| **描述** | 工具提示文本 | `"无聊值，影响探索欲望"` |

### 示例

```cpp
// 添加"好奇心"属性
FIELD(Curiosity, 0.5f, "Curiosity", "好奇心，影响调查行为")

// 添加"疲劳"属性
FIELD(Fatigue, 0.0f, "Fatigue", "疲劳值，影响休息欲望")

// 添加"忠诚度"属性
FIELD(Loyalty, 0.8f, "Loyalty", "对玩家的忠诚度")
```

---

## 🔧 如何删除属性

### 步骤1: 打开配置文件

```
Source/AINPC/Public/UtilityAI/MentalStateFields.h
```

### 步骤2: 删除对应行

```cpp
#define MENTAL_STATE_FIELDS(FIELD) \
	FIELD(Anger,         0.0f, "Anger",          "愤怒值") \
	FIELD(Fear,          0.0f, "Fear",           "恐惧值") \
	// FIELD(Confidence, 0.5f, "Confidence",     "自信值")  // ❌ 删除或注释掉
	FIELD(SocialBattery, 0.8f, "Social Battery", "社交电量") \
	FIELD(Hunger,        0.0f, "Hunger",         "饥饿值")
```

### 步骤3: 重新编译

所有引用自动移除！

---

## 🎨 编辑器中的效果

### 蓝图中

添加新字段后，在蓝图中可以直接访问：

```
Get Mental State -> Boredom (float)
```

### 详情面板中

选中 `MentalState` 对象，可以看到：

```
Mental State
  ├─ Anger: 0.0
  ├─ Fear: 0.0
  ├─ Confidence: 0.5
  ├─ Social Battery: 0.8
  ├─ Hunger: 0.0
  └─ Boredom: 0.0  ← 新增的字段
```

鼠标悬停显示工具提示：
```
Boredom
无聊值，影响探索欲望
```

---

## 🔄 自动同步的代码

### 1. FMentalState 结构体

```cpp
// LLMCommunicator.h (自动生成)
struct FMentalState {
    float Anger = 0.0f;
    float Fear = 0.0f;
    float Boredom = 0.0f;  // ✅ 自动添加
};
```

### 2. UNPCMentalState 类

```cpp
// UNPCMentalState.h (自动生成)
class UNPCMentalState {
    UPROPERTY(...) float Anger;
    UPROPERTY(...) float Fear;
    UPROPERTY(...) float Boredom;  // ✅ 自动添加
};
```

### 3. 转换函数

```cpp
// UNPCMentalState.cpp (自动生成)
void UpdateFromStruct(const FMentalState& NewState) {
    Anger = FMath::Clamp(NewState.Anger, 0.0f, 1.0f);
    Fear = FMath::Clamp(NewState.Fear, 0.0f, 1.0f);
    Boredom = FMath::Clamp(NewState.Boredom, 0.0f, 1.0f);  // ✅ 自动添加
}
```

### 4. LLM Prompt

```cpp
// LLMCommunicator.cpp (自动生成)
"Return specific float values (0.0 to 1.0) for these fields:
{
  \"Anger\": float,
  \"Fear\": float,
  \"Boredom\": float  // ✅ 自动添加
}"
```

---

## 📊 完整示例

### 添加3个新属性

```cpp
// MentalStateFields.h
#define MENTAL_STATE_FIELDS(FIELD) \
	FIELD(Anger,         0.0f, "Anger",          "愤怒值，影响攻击欲望") \
	FIELD(Fear,          0.0f, "Fear",           "恐惧值，影响逃跑欲望") \
	FIELD(Confidence,    0.5f, "Confidence",     "自信值，影响战斗策略") \
	FIELD(SocialBattery, 0.8f, "Social Battery", "社交电量，影响对话欲望") \
	FIELD(Hunger,        0.0f, "Hunger",         "饥饿值，影响觅食行为") \
	FIELD(Boredom,       0.0f, "Boredom",        "无聊值，影响探索欲望") \
	FIELD(Curiosity,     0.5f, "Curiosity",      "好奇心，影响调查行为") \
	FIELD(Fatigue,       0.0f, "Fatigue",        "疲劳值，影响休息欲望")
```

### 编译后自动生成

**C++ 结构体:**
```cpp
struct FMentalState {
    float Anger = 0.0f;
    float Fear = 0.0f;
    float Confidence = 0.5f;
    float SocialBattery = 0.8f;
    float Hunger = 0.0f;
    float Boredom = 0.0f;
    float Curiosity = 0.5f;
    float Fatigue = 0.0f;
};
```

**LLM Prompt:**
```json
{
  "Anger": float,
  "Fear": float,
  "Confidence": float,
  "SocialBattery": float,
  "Hunger": float,
  "Boredom": float,
  "Curiosity": float,
  "Fatigue": float
}
```

---

## ⚠️ 注意事项

### 1. 命名规则

- ✅ 使用驼峰命名：`SocialBattery`
- ❌ 不要用下划线：`social_battery`
- ❌ 不要用空格：`Social Battery`（显示名称可以用空格）

### 2. 默认值范围

- ✅ 使用 0.0f 到 1.0f 之间的值
- ❌ 不要超出范围（虽然会自动Clamp）

### 3. 编译顺序

修改 `MentalStateFields.h` 后：
1. 先编译 C++ 代码
2. 再打开蓝图（避免引用错误）

### 4. 数据迁移

如果删除字段，现有的蓝图和DataTable中的数据会丢失该字段的值。

---

## 🧪 测试新属性

### 1. C++ 测试

```cpp
UNPCMentalState* State = NewObject<UNPCMentalState>();
State->Boredom = 0.8f;  // ✅ 可以直接访问
```

### 2. 蓝图测试

1. 打开任意蓝图
2. 获取 `MentalState` 引用
3. 查看是否有新属性

### 3. LLM 测试

触发感知事件，查看日志：
```
[LLM] Request Sent: {...}
[LLM] Success! Parsed: Anger=0.10, Fear=0.20, Boredom=0.50
```

---

## 📈 性能影响

### 编译时

- **宏展开:** 编译时完成，无运行时开销
- **代码大小:** 每个字段增加约 ~50 字节

### 运行时

- **内存:** 每个字段 4 字节 (float)
- **性能:** 与手动编写完全相同
- **无额外开销**

---

## 🎯 最佳实践

### 1. 语义化命名

```cpp
// ✅ 好的命名
FIELD(Boredom,    0.0f, "Boredom",    "无聊值")
FIELD(Curiosity,  0.5f, "Curiosity",  "好奇心")

// ❌ 不好的命名
FIELD(Value1,     0.0f, "Value 1",    "数值1")
FIELD(Temp,       0.0f, "Temp",       "临时")
```

### 2. 合理的默认值

```cpp
// ✅ 合理的默认值
FIELD(Anger,      0.0f, ...)  // 默认不愤怒
FIELD(Confidence, 0.5f, ...)  // 默认中等自信
FIELD(Hunger,     0.0f, ...)  // 默认不饿

// ⚠️ 需要考虑的默认值
FIELD(Loyalty,    1.0f, ...)  // 默认完全忠诚？
FIELD(Fear,       0.5f, ...)  // 默认就害怕？
```

### 3. 清晰的描述

```cpp
// ✅ 清晰的描述
FIELD(Boredom, 0.0f, "Boredom", "无聊值，影响探索和娱乐行为的欲望")

// ❌ 模糊的描述
FIELD(Boredom, 0.0f, "Boredom", "一个值")
```

---

## 🔧 故障排除

### 问题1: 编译错误 "未定义的标识符"

**原因:** 宏展开失败

**解决:**
1. 检查 `MentalStateFields.h` 语法
2. 确保每行以 `\` 结尾
3. 最后一行不要有 `\`

### 问题2: 蓝图中看不到新字段

**原因:** 蓝图缓存未更新

**解决:**
1. 关闭蓝图
2. 重新编译 C++
3. 重新打开蓝图

### 问题3: LLM 不返回新字段

**原因:** Prompt 未更新

**解决:**
1. 检查 `LLMCommunicator.cpp` 是否重新编译
2. 查看日志中的 Prompt 内容
3. 重启编辑器

---

## 📚 相关文档

- `MentalStateFields.h` - 唯一需要修改的配置文件
- `Bug1_Fix_Report.md` - 数据结构优化详情
- `DataFlow_Analysis.md` - 完整的数据流转分析

---

## 🎉 总结

### 修改前

添加新属性需要修改 **4个文件**：
1. `LLMCommunicator.h` - 添加字段
2. `UNPCMentalState.h` - 添加属性
3. `UNPCMentalState.cpp` - 更新转换函数
4. `LLMCommunicator.cpp` - 更新Prompt

### 修改后 ✅

添加新属性只需修改 **1个文件**：
1. `MentalStateFields.h` - 添加一行

**其他所有代码自动同步！** 🎉

---

**配置文件位置:**  
`Source/AINPC/Public/UtilityAI/MentalStateFields.h`

**下次添加属性:** 只需编辑这一个文件！
