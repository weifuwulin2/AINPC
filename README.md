# AINPC - LLM-Driven Utility AI System for Unreal Engine 5

<div align="center">

**基于 UE5 的 LLM-Utility AI 混合架构**  
*Stanford Agent Logic (Brain) + Utility AI (Body)*

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.3+-blue.svg)](https://www.unrealengine.com/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Documentation](https://img.shields.io/badge/docs-latest-brightgreen.svg)](docs/README.md)

</div>

---

## 📖 目录 / Table of Contents

- [项目简介](#-项目简介)
- [核心特性](#-核心特性)
- [系统架构](#-系统架构)
- [快速开始](#-快速开始)
- [项目结构](#-项目结构)
- [文档导航](#-文档导航)
- [最新更新](#-最新更新)
- [开发路线](#-开发路线)

---

## 🎯 项目简介 / Project Introduction

**AINPC** 是一个创新的 AI NPC 系统,旨在解决传统 LLM Agent 响应慢、动作僵硬的问题。通过**双层解耦设计**,实现了智能决策与实时响应的完美结合:
**AINPC** is an innovative AI NPC system designed to solve the problems of slow response and rigid actions in traditional LLM Agents. Through a **dual-layer decoupled design**, it achieves a perfect combination of intelligent decision-making and real-time response:

- **🧠 认知层 (Cognitive Layer)**: 使用 LLM 处理复杂的非结构化数据(对话、环境感知),输出结构化的情绪参数
- **💪 行动层 (Action Layer)**: 使用 Utility AI 系统基于情绪参数实时计算最优动作,保证**毫秒级响应**

### 核心优势 / Core Advantages

| 传统 LLM Agent | AINPC 混合架构 |
|:--------------|:--------------|
| ❌ 响应延迟 1-3 秒 | ✅ 动作响应 < 50ms |
| ❌ 动作僵硬、不连贯 | ✅ 流畅的行为过渡 |
| ❌ 难以调试和优化 | ✅ 参数可视化调试 |
| ❌ 每次决策都需要 LLM | ✅ LLM 仅处理认知更新 |

---

## ✨ 核心特性 / Core Features

### 🔧 单点配置系统 (Single-Point Configuration)

**最新实现的自动化配置系统** - 通过宏实现一处定义,全局生效:

```cpp
// MentalStateFields.h - 唯一需要修改的地方
#define MENTAL_STATE_FIELDS(FIELD) \
    FIELD(Anger, 0.0f, "愤怒", "影响攻击欲望") \
    FIELD(Fear, 0.0f, "恐惧", "影响逃跑行为") \
    FIELD(Confidence, 0.5f, "自信", "影响对抗策略")
```

**自动生成 / Auto-generated**:
- ✅ `FMentalState` 结构体字段
- ✅ `UNPCMentalState` 类属性 (含完整 UPROPERTY 元数据)
- ✅ 构造函数初始化代码
- ✅ `ResetState()` 重置函数
- ✅ `UpdateFromStruct()` / `ToStruct()` 转换函数
- ✅ LLM Prompt 字段列表
- ✅ Utility AI 的 `GetConsiderationValue()` switch cases

**维护成本 / Maintenance Cost**: 添加新属性仅需修改 **2 个文件** (原需 8 个)
Adding new attributes only requires modifying **2 files** (previously 8)

### 🧩 模块化组件架构 / Modular Component Architecture

```
UtilityAIController (总控)
├── SensoryComponent      # 感官翻译 (物理信号 → 文本)
├── CognitionComponent    # 认知处理 (记忆 + LLM)
├── UtilityAIComponent    # 决策执行 (Utility 算分)
└── MentalState          # 共享情绪状态
```

### 🌙 Dreaming 系统 / Dreaming System

定期整理记忆,提取长期洞察:
- 每 5 分钟自动触发 (可配置)
- 将短期记忆发送给 LLM 进行总结
- 提取关键信息存入长期记忆
- 避免记忆队列溢出

### 🔄 并发请求管理 / Concurrent Request Management

使用 `TMap` 管理多个并发 LLM 请求:
- ✅ Dreaming 和感知事件可同时触发
- ✅ 每个请求独立的回调处理
- ✅ 请求 ID 追踪,便于调试

---

## 🏗️ 系统架构 / System Architecture

### 数据流向图 / Data Flow Diagram

```
┌─────────────────┐
│  UE5 Perception │  (视觉/听觉)
└────────┬────────┘
         │ 物理信号
         ▼
┌─────────────────┐
│ SensoryComponent│  翻译为文本
└────────┬────────┘
         │ "Saw Player with Gun"
         ▼
┌─────────────────┐
│CognitionComponent│ 存入记忆流
└────────┬────────┘
         │ 累积到阈值
         ▼
┌─────────────────┐
│ LLM Communicator│  发送 Prompt
└────────┬────────┘
         │ JSON Response
         ▼
┌─────────────────┐
│  MentalState    │  更新情绪参数
│ (Anger: 0.8)    │
└────────┬────────┘
         │ 每 0.2 秒
         ▼
┌─────────────────┐
│UtilityAIComponent│ 计算动作得分
└────────┬────────┘
         │ 选择最高分
         ▼
┌─────────────────┐
│  Action Execute │  播放动画/移动
└─────────────────┘
```

### 核心类说明 / Core Classes

| 类名 | 职责 | 关键方法 |
|:-----|:-----|:---------|
| `UtilityAIController` | 总控制器,协调各组件 | `BeginPlay()`, `ReceiveSpeech()` |
| `SensoryComponent` | 翻译物理信号为文本 | `OnPerceptionUpdated()` |
| `CognitionComponent` | 管理记忆和 LLM 通信 | `AddMemory()`, `TriggerDreaming()` |
| `UtilityAIComponent` | Utility AI 决策循环 | `EvaluateActions()`, `ExecuteBestAction()` |
| `UNPCMentalState` | 情绪状态数据容器 | `UpdateFromStruct()`, `ToStruct()` |
| `UtilityActionBase` | 动作基类 | `CalculateScore()`, `Execute()` |

---

## 🚀 快速开始 / Quick Start

### 环境要求 / Requirements

- **Unreal Engine**: 5.3 或更高版本
- **编译器**: Visual Studio 2022 (Windows) / Xcode (Mac)
- **LLM API**: OpenAI / Ollama / 自定义端点

### 安装步骤 / Installation Steps

1. **克隆仓库**
   ```bash
   git clone https://github.com/yourusername/AINPC.git
   cd AINPC
   ```

2. **生成项目文件**
   ```bash
   # Windows
   右键 AINPC.uproject → Generate Visual Studio project files
   
   # Mac/Linux
   ./GenerateProjectFiles.sh
   ```

3. **配置 LLM API**
   
   编辑 `Config/DefaultGame.ini`:
   ```ini
   [/Script/AINPC.LLMCommunicator]
   APIEndpoint=http://localhost:11434/api/generate
   ModelName=llama2
   APIKey=your_api_key_here
   ```

4. **编译并运行**
   - 打开 `AINPC.sln`
   - 设置为 `Development Editor` 配置
   - 编译项目
   - 启动编辑器

### 快速测试 / Quick Test

1. 打开 `Content/Maps/TestLevel`
2. 放置一个 `BP_AICharacter` 到场景中
3. 运行游戏 (PIE)
4. 观察 NPC 的自主行为

详细测试指南: [Framework_Testing_Guide.md](docs/guides/Framework_Testing_Guide.md)

---

## 📁 项目结构 / Project Structure

```
AINPC/
├── Source/AINPC/
│   ├── Controller/
│   │   └── UtilityAIController.h/cpp      # AI 总控制器
│   ├── Components/
│   │   ├── SensoryComponent.h/cpp         # 感官翻译
│   │   ├── CognitionComponent.h/cpp       # 认知处理
│   │   └── UtilityAIComponent.h/cpp       # 决策执行
│   ├── LLM/
│   │   └── LLMCommunicator.h/cpp          # LLM 通信
│   ├── UtilityAI/
│   │   ├── MentalStateFields.h            # ⭐ 单点配置
│   │   └── UNPCMentalState.h/cpp          # 情绪状态
│   └── Base/
│       └── UtilityActionBase.h/cpp        # 动作基类
├── Content/
│   ├── Blueprints/
│   │   ├── AI/                            # AI 蓝图
│   │   └── Actions/                       # 动作蓝图
│   └── Maps/                              # 测试地图
├── docs/
│   ├── guides/                            # 📖 使用指南
│   ├── reports/                           # 📝 进度报告
│   └── analysis/                          # 📊 分析文档
├── README.md                              # 本文档
└── CHANGELOG.md                           # 变更日志
```

---

## 📚 文档导航 / Documentation

### 快速查找 / Quick Reference

| 我想... | 查看文档 |
|:--------|:---------|
| 了解系统架构和数据流 | [DataFlow_Analysis.md](docs/analysis/DataFlow_Analysis.md) |
| 理解Personality→OCEAN→Maslow→LLM的完整转换链路 | [Personality_OCEAN_Maslow_LLM_Pipeline.md](docs/design/Personality_OCEAN_Maslow_LLM_Pipeline.md) ⭐ |
| 配置 LLM API | [LLM_Config_Guide.md](docs/guides/LLM_Config_Guide.md) |
| 添加新的情绪属性 | [Single_Point_Configuration_Guide.md](docs/guides/Single_Point_Configuration_Guide.md) |
| 配置 Utility AI 动作 | [Utility_AI_Configuration_Guide.md](docs/guides/Utility_AI_Configuration_Guide.md) |
| 测试 Dreaming 功能 | [Dreaming_Test_Guide.md](docs/guides/Dreaming_Test_Guide.md) |
| 修复已知 Bug | [BugFixes_Patch.md](docs/guides/BugFixes_Patch.md) |
| 查看完整文档索引 | [docs/README.md](docs/README.md) |

### 核心指南 / Core Guides

- **[单点配置系统指南](docs/guides/Single_Point_Configuration_Guide.md)** - 如何使用宏自动化配置
- **[Utility AI 配置指南](docs/guides/Utility_AI_Configuration_Guide.md)** - 创建和调试动作
- **[框架测试指南](docs/guides/Framework_Testing_Guide.md)** - 完整测试流程
- **[LLM 配置指南](docs/guides/LLM_Config_Guide.md)** - API 设置和故障排查

### 技术报告 / Technical Reports

- **[数据流分析](docs/analysis/DataFlow_Analysis.md)** - 完整的架构分析 + Mermaid 图
- **[单点配置实现报告](docs/reports/Single_Point_Config_Implementation.md)** - 自动化系统详解
- **[Bug 修复报告](docs/reports/Bug1_Fix_Report.md)** - 数据结构优化
- **[并发请求修复](docs/reports/Bug4_Fix_Report.md)** - TMap 并发管理

---

## 🎉 最新更新 / Latest Updates

### v0.4.0 - OCEAN + Maslow + 语义映射系统 (2026-01-08)

#### 🎉 重大改进 / Major Improvements

##### 1. OCEAN 性格 + 马斯洛需求层次系统

- **✅ OCEAN Big Five 性格模型**
  - 5 个性格特质: Openness, Conscientiousness, Extraversion, Agreeableness, Neuroticism
  - 从 DataTable 加载性格配置
  - 预设性格模板: BraveWarrior, CautiousGuard, FriendlyMerchant, ReclusiveScholar

- **✅ 马斯洛需求层次**
  - 10 个科学化的需求变量替代旧情绪系统
  - 5 个层次: 生理、安全、社交、尊严、自我实现
  - 动态权重计算: OCEAN → Maslow 转换矩阵

- **✅ 管辖权划分**
  - Engine 独占: Hunger, Energy (物理、时间驱动)
  - LLM 独占: Trust, Anger, Social_Status, Curiosity (语义、心理)
  - 混合管理: Perceived_Threat, Resource_Anxiety, Loneliness, Duty_Urgency

##### 2. Utility AI 双阶段算法重构

- **✅ Motivation (动机) - 加法**
  - 多个动机可以累加
  - 公式: `Σ(MentalState × Personality)`

- **✅ Context (条件) - 乘法**
  - 必要条件使用乘法 (任何为 0 = 不可行)
  - 公式: `∏(Contexts)`

- **✅ 新评分公式**
  ```
  Score = BaseReward × (Σ Motivations) × (∏ Contexts)
        = 动作奖励 × (心理状态 × 性格权重) × 必要条件
  ```

##### 3. BaseReward 语义重构

- **✅ 更清晰的命名**
  - `BaseWeight` → `BaseReward`
  - 代表动作的内在价值 ("这饭管饱吗?")

- **✅ 三维度计算**
  - MentalState (LLM): "我现在多饿?"
  - Personality (OCEAN): "我有多爱吃?"
  - BaseReward (Config): "这饭管饱吗?"

##### 4. 语义映射 + 平滑插值系统

- **✅ LLM 输出语义标签**
  - 不再输出数值 (0.0-1.0)
  - 使用自然语言标签: "Furious", "Trusting", "Starving"
  - 5点量表法: None, Slight, Moderate, Strong, Extreme

- **✅ SentimentMapper**
  - 自动转换标签为数值
  - 支持自然语言标签 (Annoyed, Angry, Furious, Enraged)
  - 容错率高,稳定性强

- **✅ 平滑插值系统**
  - 情绪变化不再生硬跳变
  - 可配置插值速度 (愤怒快 5.0, 信任慢 0.5)
  - 随机扰动增加个性化
  - 每帧平滑过渡: `0.1 → 0.18 → 0.28 → ... → 0.9`

#### 🔧 技术改进 / Technical Improvements

- **PersonalityComponent 架构修正**
  - 从 Character 移至 AIController
  - 与其他 AI 组件统一管理

- **LLM Prompt 优化**
  - JSON 格式化
  - 管辖权规则说明
  - 标签选择指令

- **枚举命名修复**
  - C++ 兼容的驼峰命名
  - 手动映射到下划线字段名

#### 📊 性能影响 / Performance Impact

- PersonalityComponent 权重计算: 一次性 (BeginPlay)
- 插值计算: ~0.01ms per NPC per frame
- 总影响: < 0.1ms per NPC

#### 📝 新增文档 / New Documentation

**设计文档:**
- `OCEAN_Maslow_System.md` - 完整系统设计
- `UtilityAI_TwoPhase_Algorithm.md` - 双阶段算法
- `BaseReward_Calculation_Formula.md` - 评分公式详解
- `Semantic_Mapping_System.md` - 语义映射系统
- `Maslow_Variables_Jurisdiction.md` - 管辖权划分
- `Personality_OCEAN_Maslow_LLM_Pipeline.md` - **⭐ Personality → OCEAN → Maslow → LLM 完整数据流**

**实现指南:**
- `PersonalityComponent_Integration_Guide.md` - 性格组件集成
- `PersonalityTable_Configuration_Guide.md` - DataTable 配置
- `Semantic_Mapping_Integration_Summary.md` - 语义映射集成
- `Testing_Guide.md` - 综合测试指南
- `Quick_Start_Guide.md` - 5分钟快速开始

---

### v0.3.0 - 单点配置系统 (2026-01-03)

#### 🎉 重大改进 / Major Improvements

- **✅ 实现宏驱动的单点配置系统**
  - 所有情绪属性在 `MentalStateFields.h` 中集中定义
  - 自动生成 7 种不同的代码上下文
  - 维护成本降低 **75%**

- **✅ 优化数据结构**
  - 添加 `UpdateFromStruct()` / `ToStruct()` 转换函数
  - 自动值域限制 (0.0-1.0)
  - 移除手动字段初始化

- **✅ 修复并发请求冲突**
  - 使用 `TMap` 管理多个 LLM 请求
  - 支持 Dreaming 和感知事件同时触发
  - 添加请求 ID 追踪

- **✅ 启用 Dreaming 系统**
  - 每 5 分钟自动整理记忆
  - LLM 提取长期洞察
  - 防止记忆溢出

查看完整更新: [CHANGELOG.md](CHANGELOG.md)

---

## 🗺️ 开发路线 / Roadmap

### ✅ 已完成 / Completed

- [x] 基础 Utility AI 框架
- [x] LLM 通信模块
- [x] 单点配置系统
- [x] Dreaming 记忆整理
- [x] 并发请求管理
- [x] 完整文档体系

### 🚧 进行中 / In Progress

- [ ] action测试

### 📋 计划中 / Planned

- [ ] 多 NPC 社交互动
- [ ] 情绪可视化 UI



## 📄 许可证 / License

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

## 📧 联系方式 / Contact

- **项目主页**: [GitHub Repository](https://github.com/weifuwulin2/AINPC)
- **问题反馈**: [Issues](https://github.com/weifuwulin2/AINPC/issues)
- **文档**: [Documentation](docs/README.md)

---

<div align="center">

**⭐ 如果这个项目对你有帮助,请给一个 Star!**

Made with ❤️ using Unreal Engine 5

</div>
