#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "SensoryComponent.generated.h"

// 定义一个委托：当感官把信号翻译成文字后广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSensoryStimulusProduced, const FString&, StimulusDescription);

class UAIPerceptionComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API USensoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USensoryComponent();

	// --- 对外事件 ---
	// 认知组件(Cognition)或者 Controller 监听这个事件
	UPROPERTY(BlueprintAssignable, Category = "AI Sensory")
	FOnSensoryStimulusProduced OnStimulusProduced;

	// --- 外部接口：接收对话 ---
	UFUNCTION(BlueprintCallable, Category = "AI Communication")
	void ReceiveSpeech(AActor* Speaker, FString Message);

	// --- 初始化依赖 ---
	// Controller 在 BeginPlay 时调用这个，把 AIPerception 传进来
	void InitializeSensorySystem(UAIPerceptionComponent* InPerceptionComp);

protected:
	virtual void BeginPlay() override;

	// --- 内部回调处理 ---
	UFUNCTION()
	void HandleTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void HandleDamageTaken(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

private:
	// 辅助翻译函数
	FString FormatDescription(FString Verb, AActor* Target, FString ExtraInfo = "");
};