// MentalStateFields.h
// 
// 【唯一需要修改的配置文件】
// 添加或删除情绪属性只需在这里修改 MENTAL_STATE_FIELDS 宏
//
// 格式: FIELD(字段名, 默认值, 显示名称, 描述)
//

#pragma once

/**
 * 定义所有情绪状态字段
 * 
 * 使用方法:
 * 1. 添加新字段: FIELD(NewEmotion, 0.0f, "New Emotion", "Description")
 * 2. 删除字段: 直接删除对应的 FIELD 行
 * 3. 修改默认值: 改变第二个参数
 * 
 * 注意: 修改后需要重新编译项目
 */
#define MENTAL_STATE_FIELDS(FIELD) \
	FIELD(Anger,         0.0f, "Anger",          "愤怒值，影响攻击欲望") \
	FIELD(Fear,          0.0f, "Fear",           "恐惧值，影响逃跑欲望") \
	FIELD(Confidence,    0.5f, "Confidence",     "自信值，影响战斗策略") \
	FIELD(SocialBattery, 0.8f, "Social Battery", "社交电量，影响对话欲望") \
	FIELD(Hunger,        0.0f, "Hunger",         "饥饿值，影响觅食行为")
	// 添加新字段示例:
	// FIELD(Boredom,    0.0f, "Boredom",        "无聊值，影响探索欲望") \
	// FIELD(Curiosity,  0.5f, "Curiosity",      "好奇心，影响调查行为")

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
