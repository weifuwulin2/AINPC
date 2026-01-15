// MentalStateFields.h
// 
// 【唯一需要修改的配置文件】
// 添加或删除情绪属性只需在这里修改 MENTAL_STATE_FIELDS 宏
//
// 格式: FIELD(字段名, 默认值, 显示名称, 描述)
//

#pragma once

/**
 * 定义所有马斯洛需求层次字段 / Define all Maslow's Hierarchy fields
 * 
 * 基于马斯洛需求层次理论的 10 个核心变量
 * 10 core variables based on Maslow's Hierarchy of Needs
 * 
 * 管辖权划分 / Jurisdiction Division:
 * - [ENGINE] = 游戏引擎独裁 (Engine exclusive control)
 * - [LLM]    = LLM 独裁 (LLM exclusive control)
 * - [HYBRID] = 混合管辖 (Hybrid - both Engine and LLM)
 * 
 * 详见: docs/design/Maslow_Variables_Jurisdiction.md
 * 
 * 使用方法 / Usage:
 * 1. 添加新字段: FIELD(NewField, 0.0f, "New Field", "Description")
 * 2. 删除字段: 直接删除对应的 FIELD 行
 * 3. 修改默认值: 改变第二个参数
 * 
 * 注意: 修改后需要重新编译项目 / Note: Recompile project after modification
 */
#define MENTAL_STATE_FIELDS(FIELD) \
	/* ========== 生理层 (Physiological) - ENGINE ========== */ \
	FIELD(Hunger,            0.0f, "Hunger",            "[ENGINE] 饥饿感 - 随时间增长，吃东西减少") \
	FIELD(Fatigue,           0.0f, "Fatigue",           "[ENGINE] 疲劳度 - 随时间增长，睡觉减少") \
	/* ========== 安全层 (Safety) - HYBRID ========== */ \
	FIELD(Perceived_Threat,  0.0f, "Perceived Threat",  "[HYBRID] 威胁感 - Engine检测敌人 + LLM分析威胁语言/缺资源") \
	/* ========== 社交层 (Belonging) - HYBRID ========== */ \
	FIELD(Loneliness,        0.0f, "Loneliness",        "[HYBRID] 孤独感/被冷落 - Engine计时器 + LLM检测被忽视") \
	/* ========== 尊严层 (Esteem) - LLM ========== */ \
	FIELD(Indignity,         0.0f, "Indignity",         "[LLM] 屈辱感/尊严受损 - 随侮辱/轻视升高") \
	/* ========== 自我实现层 (Self-Actualization) - LLM ========== */ \
	FIELD(Boredom,           0.0f, "Boredom",           "[LLM] 无聊感/缺乏意义 - 随时间/重复升高")

/**
 * 使用说明:
 * 
 * 这个宏会自动生成:
 * 1. FMentalState 结构体的所有字段
 * 2. UNPCMentalState 类的所有属性
 * 3. UpdateFromStruct() 的转换代码
 * 4. ToStruct() 的转换代码
 * 5. ResetState() 的重置代码
 * 6. LLM Prompt 的字段列表
 * 
 * 你不需要手动修改任何其他文件！
 */
