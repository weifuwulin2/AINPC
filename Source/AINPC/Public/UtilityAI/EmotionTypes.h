// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EmotionTypes.generated.h"

/**
 * NPC 情绪状态枚举 (7种核心情绪)
 * NPC Emotion State Enum (7 Core Emotions)
 * 
 * 这些是互斥的。每帧 EmotionEvaluator 计算出唯一的 Dominant Emotion。
 * These are mutually exclusive. EmotionEvaluator calculates 1 dominant emotion per frame.
 */
UENUM(BlueprintType)
enum class EEmotionState : uint8
{
	Neutral     UMETA(DisplayName = "Neutral - Rational"),      // 默认：理性人 (Rational) - 逻辑驱动，无Bias
	Angry       UMETA(DisplayName = "Angry - Berserker"),       // 进攻：狂战士 (Berserker) - 攻击欲望极高
	Scared      UMETA(DisplayName = "Scared - Victim"),         // 防御：受惊者 (Victim) - 逃跑/躲藏优先
	Sad         UMETA(DisplayName = "Sad - Depressive"),        // 消极：抑郁者 (Depressive) - 行动力低下
	Happy       UMETA(DisplayName = "Happy - Optimist"),        // 积极：乐天派 (Optimist) - 社交欲望高
	Curious     UMETA(DisplayName = "Curious - Observer"),      // 探索：观察者 (Observer) - 探索未知
	Disgust     UMETA(DisplayName = "Disgust - Haughty")        // 排斥：洁癖/高傲 (Haughty) - 拒绝近身
};
