// Fill out your copyright notice in the Description page of Project Settings.


#include "Actions/Action_Attack.h"
#include "UtilityAI/UNPCMentalState.h"


UAction_Attack::UAction_Attack()
{
	ActionName = "Attack Enemy";
}

float UAction_Attack::CalculateScore(const UNPCMentalState* State)
{
	// 安全检查
	if (!State) return 0.0f;

	// === 核心公式 ===
	// 简单版：分数 = 愤怒值 * 100
	// 如果 Anger 是 0.8，得分就是 80
	// 你可以在这里写很复杂的数学公式
	float Score = State->Anger * 100.0f;
    
	// 抑制欲望：越害怕越不敢打 (恐惧值是负权重)
	Score -= State->Fear * 80.0f;   

	// 修正：如果肚子很饿 (Hunger 1.0)，可能为了抢食物而被迫攻击
	Score += State->Hunger * 50.0f;

	// 结果：
	// Anger(0.2)*100 = 20
	// Fear(0.9)*80 = 72
	// Result = -52 (分数极低，不应该被选中)
	return FMath::Max(Score, 0.0f);
}

void UAction_Attack::Execute(AAIController* Controller)
{
	// 调用父类逻辑（如果有日志的话）
	Super::Execute(Controller);

	// 真正的逻辑：这里应该写 MoveTo 或 PlayMontage
	// 现在咱们先打印个红色的日志证明它在跑
	UE_LOG(LogTemp, Error, TEXT(">>> [ACTION] NPC is ATTACKING! (Anger Driven) <<<"));
}