// Fill out your copyright notice in the Description page of Project Settings.


#include "Base/UtilityActionBase.h"

float UUtilityActionBase::CalculateScore(const UNPCMentalState* State)
{
	return 0.0f; // 基类默认 0 分
}

void UUtilityActionBase::Enter(AAIController* Controller)
{
	
}

void UUtilityActionBase::Execute(AAIController* Controller)
{
	// 基类留空，或者打印日志
	UE_LOG(LogTemp, Log, TEXT("Executing Base Action: %s"), *ActionName);
}

void UUtilityActionBase::Exit(AAIController* Controller)
{
	// 清理逻辑
}