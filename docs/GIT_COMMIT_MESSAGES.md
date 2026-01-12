# Git Commit Messages - 单点配置系统实现

根据本次修改，这里提供几种不同风格的 commit message 供你选择：

---

## 📋 选项1: 简洁风格 (推荐用于单次提交)

```bash
git commit -m "feat: implement single-point configuration system for mental state attributes

Major Changes:
- Created MentalStateFields.h as central configuration file
- Auto-generate code using C++ macros (fields, properties, conversions, prompts)
- Fixed concurrent LLM request handling with TMap
- Enabled Dreaming mechanism for memory consolidation
- Added conversion functions to UNPCMentalState

Benefits:
- Reduced maintenance: 2 file edits instead of 8 when adding attributes
- Eliminated manual switch cases and conversion code
- 75% reduction in manual work

Limitations:
- UENUM values still require manual addition (UHT limitation)

Files changed: 9 source files, 8 documentation files"
```

---

## 📋 选项2: 详细风格 (推荐用于重要版本)

```bash
git commit -m "feat: implement single-point configuration system for mental state attributes" -m "
🎯 Core Features:
- Created MentalStateFields.h as single source of truth for all mental state attributes
- Implemented macro-based code generation system
- Auto-generates: struct fields, class properties, initialization, conversions, LLM prompts, switch cases

🐛 Bug Fixes:
- Fixed concurrent LLM request handling (replaced single callbacks with TMap)
- Fixed data structure conversion (added UpdateFromStruct/ToStruct functions)
- Removed redundant manual initialization in UtilityAIController

✨ New Capabilities:
- Enabled Dreaming mechanism (5-minute timer for memory consolidation)
- Auto-generated LLM prompts ensure correct field requests
- Auto-generated switch cases in GetConsiderationValue()

📊 Impact:
- Maintenance: 2 file edits instead of 8 (75% reduction)
- Performance: Zero runtime overhead (compile-time macros)
- Code quality: Eliminated duplication, improved type safety

⚠️ Known Limitations:
- UENUM values require manual addition (Unreal Header Tool limitation)
- Trade-off: 2 manual edits vs 8 for practical automation

📁 Files Modified:
Source:
- MentalStateFields.h (NEW)
- LLMCommunicator.h/cpp
- UNPCMentalState.h/cpp
- UtilityAIController.h/cpp
- UtilityActionBase.h/cpp

Documentation:
- Added 8 comprehensive guides and reports
- Updated CHANGELOG.md
"
```

---

## 📋 选项3: Conventional Commits 风格 (推荐用于团队协作)

```bash
git commit -m "feat(config): implement single-point configuration system

BREAKING CHANGE: None

Features:
- feat(config): add MentalStateFields.h as central configuration
- feat(codegen): implement macro-based code generation
- feat(ai): enable Dreaming mechanism with 5-minute timer
- feat(llm): auto-generate prompt field lists

Fixes:
- fix(llm): resolve concurrent request callback conflicts using TMap
- fix(data): add conversion functions to UNPCMentalState
- fix(init): remove redundant manual field initialization

Improvements:
- refactor(codegen): auto-generate struct fields and class properties
- refactor(codegen): auto-generate initialization and conversion code
- refactor(codegen): auto-generate switch cases in GetConsiderationValue
- perf(compile): zero runtime overhead with compile-time macros

Documentation:
- docs(guides): add single-point configuration guide
- docs(guides): add UENUM limitation explanation
- docs(reports): add implementation and bug fix reports

Impact: Reduces maintenance from 8 file edits to 2 (75% reduction)
"
```

---

## 📋 选项4: 分步提交 (如果你想分多次提交)

### Commit 1: 核心配置系统
```bash
git add Source/AINPC/Public/UtilityAI/MentalStateFields.h
git add Source/AINPC/LLM/LLMCommunicator.h
git add Source/AINPC/Public/UtilityAI/UNPCMentalState.h
git add Source/AINPC/Private/UtilityAI/UNPCMentalState.cpp

git commit -m "feat(config): implement single-point configuration system

- Created MentalStateFields.h as central configuration
- Auto-generate FMentalState fields using macros
- Auto-generate UNPCMentalState properties using macros
- Auto-generate initialization and conversion functions

Reduces manual work by 75% when adding new attributes"
```

### Commit 2: LLM 改进
```bash
git add Source/AINPC/LLM/LLMCommunicator.cpp

git commit -m "fix(llm): resolve concurrent request handling and auto-generate prompts

- Fixed concurrent request conflicts using TMap
- Auto-generate LLM prompt field lists from configuration
- Added request ID tracking for debugging

Ensures Dreaming and perception events don't interfere"
```

### Commit 3: Utility AI 集成
```bash
git add Source/AINPC/Public/Base/UtilityActionBase.h
git add Source/AINPC/Private/Base/UtilityActionBase.cpp

git commit -m "refactor(utility): auto-generate switch cases for mental state fields

- Auto-generate GetConsiderationValue() switch cases
- Eliminates manual case additions
- Note: UENUM values still require manual addition (UHT limitation)"
```

### Commit 4: Controller 优化
```bash
git add Source/AINPC/Controller/UtilityAIController.h
git add Source/AINPC/Controller/UtilityAIController.cpp

git commit -m "feat(ai): enable Dreaming and remove redundant initialization

- Added Dreaming timer (5-minute interval)
- Removed manual field initialization (now auto-generated)
- Improved code consistency"
```

### Commit 5: 文档
```bash
git add docs/
git add CHANGELOG.md

git commit -m "docs: add comprehensive guides for single-point configuration

- Added configuration usage guide
- Added UENUM limitation explanation
- Added implementation reports
- Updated CHANGELOG"
```

---

## 📋 选项5: 超简洁风格 (快速提交)

```bash
git commit -m "feat: single-point config system + bug fixes

- Centralized mental state config in MentalStateFields.h
- Auto-generate code using macros (75% less manual work)
- Fixed concurrent LLM requests
- Enabled Dreaming mechanism
- Added comprehensive documentation"
```

---

## 🎯 推荐使用

### 如果是个人项目:
**使用选项1 (简洁风格)** - 清晰明了，包含关键信息

### 如果是团队项目:
**使用选项3 (Conventional Commits)** - 符合业界标准

### 如果想保持清晰的历史:
**使用选项4 (分步提交)** - 每个功能单独提交

---

## 📝 完整提交流程示例

### 单次提交 (推荐)
```bash
# 1. 查看修改
git status

# 2. 添加所有修改
git add .

# 3. 提交 (使用选项1的message)
git commit -m "feat: implement single-point configuration system for mental state attributes

Major Changes:
- Created MentalStateFields.h as central configuration file
- Auto-generate code using C++ macros (fields, properties, conversions, prompts)
- Fixed concurrent LLM request handling with TMap
- Enabled Dreaming mechanism for memory consolidation
- Added conversion functions to UNPCMentalState

Benefits:
- Reduced maintenance: 2 file edits instead of 8 when adding attributes
- Eliminated manual switch cases and conversion code
- 75% reduction in manual work

Limitations:
- UENUM values still require manual addition (UHT limitation)

Files changed: 9 source files, 8 documentation files"

# 4. 推送
git push origin main
```

---

## 🏷️ 可选: 添加 Git Tag

如果这是一个重要版本：

```bash
# 创建带注释的标签
git tag -a v0.3.0 -m "Single-Point Configuration System

Major Features:
- Centralized configuration in MentalStateFields.h
- Macro-based code generation (75% less manual work)
- Fixed concurrent LLM request handling
- Enabled Dreaming mechanism

This release significantly improves maintainability and reduces
the risk of errors when adding new mental state attributes."

# 推送标签
git push origin v0.3.0
```

---

## 📊 本次修改统计

### 文件变更
- **新增:** 1 个配置文件
- **修改:** 8 个源代码文件
- **新增文档:** 8 个文档文件

### 代码变更
- **添加:** ~300 行 (主要是文档)
- **删除:** ~50 行 (移除重复代码)
- **修改:** ~100 行 (使用宏替换手动代码)

### 影响范围
- Mental State System: 完全重构
- LLM Integration: 重大改进
- Utility AI: 部分自动化
- Documentation: 全面补充

---

选择你喜欢的风格，复制对应的命令即可！🚀

---

## [2026-01-12] Fix MetabolismComponent Compilation Error
**Type**: fix
**Scope**: MetabolismComponent
**Description**:
- Fixed `MetabolismComponent.cpp` to include `UtilityAI/UNPCMentalState.h` instead of the incorrect `UtilityAI/NPCMentalState.h`.
- Resolved compilation error C1083.

---

## [2026-01-12] Implement Universal Smart Object Actions
**Type**: feat
**Scope**: ActionSystem
**Description**:
- Implemented `UAction_SmartObject` C++ class for tag-based interaction (Eat/Sleep).
- Updated `SensoryComponent` with `FindBestSmartObject` logic.
- Updated `UtilityActionBase` to support `SmartObjectTag` configuration and new Input Types (`HasFoodNearby`, `HasBedNearby`).
- Added Documentation Guide: `guides/Universal_SmartObject_Action_Guide.md`.

---

## [2026-01-12] Simplify Faction System to Three Core Factions
**Type**: refactor
**Scope**: SensoryComponent, SocialTypes
**Description**:
- Simplified `EFactionType` enum to three core factions: `Neutral`, `Human`, `Monster`
- Removed complex faction types (Bandit, Wildlife, etc.) for clearer semantics
- Updated `SensoryComponent::AreActorsHostile()` with simplified hostility logic:
  - Different non-neutral factions are hostile
  - Same faction actors are never hostile
  - Neutral faction is never hostile to anyone
- Removed tag-based "Enemy" checks in favor of faction-based system
- Updated `SocialTypes.h` with cleaner faction definitions
- Ensured Player characters are correctly assigned to `Human` faction
- Added comprehensive documentation in `docs/updates/Faction_System_Update.md`

**Impact**:
- Easier configuration: Only 3 factions to manage instead of multiple types
- More predictable behavior: Clear hostility rules based on faction membership
- Better maintainability: Centralized faction logic in enum and hostility function
- Fixes issue where same-faction NPCs (e.g., two Zombies) would attack each other

**Files Modified**:
- `Source/AINPC/Public/SocialTypes.h` - Simplified EFactionType enum
- `Source/AINPC/Components/SensoryComponent.h` - Updated hostility detection
- `Source/AINPC/Components/SensoryComponent.cpp` - Implemented simplified hostility logic
- `docs/updates/Faction_System_Update.md` - Added comprehensive update documentation

---

## [2026-01-13] Refine LLM Prompt Behavior & Fix Faction Initialization
**Type**: fix
**Scope**: Cognition, Sensory
**Description**:
- (Conv 710) Implemented robust retry mechanism for LLM requests to handle race conditions during initialization.
- Fixed `PendingStimulus` clearing logic to ensure stimuli are processed correctly after retry.
- (Conv 250) Fixed Faction initialization issues where `GetFaction()` returned Neutral.
- Added `docs/troubleshooting/Faction_Not_Loading_Fix.md` guide.


