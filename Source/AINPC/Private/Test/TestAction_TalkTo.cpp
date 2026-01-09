#include "Test/TestAction_TalkTo.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Components/SensoryComponent.h" 
#include "Controller/UtilityAIController.h"

UTestAction_TalkTo::UTestAction_TalkTo()
{
	ActionName = "Test_TalkTo";
	ExecutionTime = 0.0f;
	LastChatTime = -999.0f;
	CurrentTarget = nullptr;
}

void UTestAction_TalkTo::Enter_Implementation(AAIController* Controller)
{
	Super::Enter_Implementation(Controller);

	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		ExecutionTime = World->GetTimeSeconds();
	}
	
	CurrentTarget = FindBestTalkTarget(Controller);

	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] TalkTo Action ENTERED"));
	
	if (CurrentTarget)
	{
		UE_LOG(LogTemp, Display, TEXT("[TalkTo] Target found: %s"), *CurrentTarget->GetName());
		Controller->SetFocus(CurrentTarget);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TalkTo] No friendly target found immediately."));
	}
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
}

void UTestAction_TalkTo::Execute_Implementation(AAIController* Controller)
{
	Super::Execute_Implementation(Controller);

	if (!Controller) return;
	UWorld* World = Controller->GetWorld();
	if (!World) return;
	APawn* ControlledPawn = Controller->GetPawn();
	if (!ControlledPawn) return;

	// 1. 如果没有目标或目标失效，尝试寻找
	bool bTargetInvalid = !CurrentTarget || !IsValid(CurrentTarget) || CurrentTarget->IsPendingKillPending() || CurrentTarget->ActorHasTag("Dead");
	
	if (bTargetInvalid)
	{
		CurrentTarget = FindBestTalkTarget(Controller);
		if (CurrentTarget)
		{
			Controller->SetFocus(CurrentTarget);
			UE_LOG(LogTemp, Log, TEXT("[TalkTo] Found new target: %s"), *CurrentTarget->GetName());
		}
		else
		{
			// 如果找不到人聊天，就没事干了，可以让它随便走走或者停下
			Controller->StopMovement();
			return;
		}
	}

	// 2. 移动逻辑
	float Dist = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
	float ChatRange = 300.0f;

	if (Dist > ChatRange)
	{
		// 还没到，继续走
		Controller->MoveToActor(CurrentTarget, ChatRange - 50.0f); // 走到 ChatRange 里面一点
	}
	else
	{
		// 到了，停下来
		Controller->StopMovement(); // 停止移动
		
		// 确保面向目标
		Controller->SetFocus(CurrentTarget);

		// 3. 聊天逻辑 (每5秒一次)
		float CurrentTime = World->GetTimeSeconds();
		if (CurrentTime - LastChatTime > 5.0f)
		{
			LastChatTime = CurrentTime;
			
			// 触发聊天事件
			// 注意：这里我们简单打印日志，或者调用 Controller 的接口
			UE_LOG(LogTemp, Warning, TEXT("[TalkTo] Chatting with %s: 'Hello my friend!'"), *CurrentTarget->GetName());

			// 尝试调用 UtilityAIController 的 ReceiveSpeech (模拟说话)
			if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(Controller))
			{
                // 发送一个 Semantic Event 表示由于聊天产生的互动
                // 这里我们模拟"听到"自己说话，或者直接让 EmotionDisplay 显示气泡
                // 暂时用 Log 代替
                if (UAICon->SensoryComp)
                {
                    // 让自己发出声音 (Instigator 是自己)
                    UAICon->SensoryComp->ReceiveSpeech(ControlledPawn, TEXT("Nice weather today, isn't it?"));
                }
			}
		}
	}
}

void UTestAction_TalkTo::Exit_Implementation(AAIController* Controller)
{
	Super::Exit_Implementation(Controller);
	
	if (Controller)
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
		Controller->StopMovement();
	}
	
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
	UE_LOG(LogTemp, Display, TEXT("[TEST] TalkTo Action EXITED"));
	UE_LOG(LogTemp, Display, TEXT("───────────────────────────────────────"));
}

AActor* UTestAction_TalkTo::FindBestTalkTarget(AAIController* Controller)
{
	if (!Controller || !Controller->GetWorld()) return nullptr;
	APawn* MyPawn = Controller->GetPawn();
	if (!MyPawn) return nullptr;

	AActor* BestCandidate = nullptr;
	float ClosestDistSq = FLT_MAX;

	// 遍历所有角色
	for (TActorIterator<ACharacter> It(Controller->GetWorld()); It; ++It)
	{
		ACharacter* Candidate = *It;
		
		// 基本过滤
		if (Candidate == MyPawn) continue;
		if (!IsValid(Candidate) || Candidate->IsPendingKillPending()) continue;
        if (Candidate->ActorHasTag("Dead")) continue;
        
        // 排除 Enemy
        if (Candidate->ActorHasTag("Enemy")) continue;

		// 排除布娃娃 (物理模拟)
		if (Candidate->GetMesh() && Candidate->GetMesh()->IsSimulatingPhysics()) continue;

		// 找最近的
		float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			BestCandidate = Candidate;
		}
	}

	return BestCandidate;
}
