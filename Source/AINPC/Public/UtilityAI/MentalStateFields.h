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
	/* ========== 生理层 (Physiological) - ENGINE 独裁 ========== */ \
	FIELD(Hunger,            0.0f, "Hunger",            "[ENGINE] 饥饿感 - 随时间增长，吃东西减少") \
	FIELD(Energy,            1.0f, "Energy",            "[ENGINE] 精力 - 活动消耗，休息恢复") \
	/* ========== 安全层 (Safety) - HYBRID 混合管辖 ========== */ \
	FIELD(Perceived_Threat,  0.0f, "Perceived Threat",  "[HYBRID] 感知威胁 - Engine检测敌人 + LLM分析语言威胁") \
	FIELD(Resource_Anxiety,  0.0f, "Resource Anxiety",  "[HYBRID] 资源焦虑 - Engine检测背包 + LLM分析传闻") \
	/* ========== 社交层 (Love/Belonging) - HYBRID + LLM ========== */ \
	FIELD(Loneliness,        0.0f, "Loneliness",        "[HYBRID] 孤独感 - Engine计时器 + LLM检测被冷落") \
	FIELD(Trust,             0.5f, "Trust",             "[LLM] 信任度 - LLM分析诚实、背叛、帮助") \
	/* ========== 尊严层 (Esteem) - LLM 独裁 ========== */ \
	FIELD(Anger,             0.0f, "Anger",             "[LLM] 愤怒 - LLM分析侮辱、挑衅、道歉") \
	FIELD(Social_Status,     0.5f, "Social Status",     "[LLM] 地位需求 - LLM分析尊重、轻视") \
	/* ========== 自我实现层 (Self-Actualization) - HYBRID + LLM ========== */ \
	FIELD(Duty_Urgency,      0.0f, "Duty Urgency",      "[HYBRID] 责任紧迫感 - Engine检查时间 + LLM分析命令") \
	FIELD(Curiosity,         0.5f, "Curiosity",         "[LLM] 好奇心 - LLM分析秘密、奇怪事物")

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
