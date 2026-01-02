// SmartFoodActor.cpp
#include "SmartFoodActor.h"

ASmartFoodActor::ASmartFoodActor()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = MeshComp;
}

// 1. 告诉 AI 我有什么用
void ASmartFoodActor::GetAffordances_Implementation(TMap<FString, float>& OutAffordances)
{
	// 这个苹果能减少 0.5 的饥饿 (Hunger)
	OutAffordances.Add(TEXT("Hunger"), -0.5f);
    
	// 吃苹果让人开心，增加一点 Comfort
	OutAffordances.Add(TEXT("Confidence"), 0.1f);
}

// 2. 具体的执行逻辑
bool ASmartFoodActor::Interact_Implementation(AActor* User)
{
	if (!User) return false;

	UE_LOG(LogTemp, Log, TEXT("%s is eating %s"), *User->GetName(), *GetName());

	// 在这里播放声音、特效...
	// UGameplayStatics::PlaySoundAtLocation(...)

	// 吃完了，销毁自己
	Destroy();

	return true;
}