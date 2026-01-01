// Fill out your copyright notice in the Description page of Project Settings.


#include "UtilityAI/UNPCMentalState.h"

UNPCMentalState::UNPCMentalState()
{
	// 初始化默认值
	Anger = 0.0f;
	Fear = 0.0f;
	Confidence = 0.5f; // 默认普通自信
	SocialBattery = 0.8f; // 默认稍微想说话
	Hunger = 0.0f;
}

void UNPCMentalState::ResetState()
{
	Anger = 0.0f;
	Fear = 0.0f;
	Confidence = 0.5f;
	// 其他重置逻辑...
}