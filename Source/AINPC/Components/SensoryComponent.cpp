#include "SensoryComponent.h"

#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "GameFramework/Character.h"

USensoryComponent::USensoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // 不需要 Tick，全靠事件驱动 / No Tick needed, fully event-driven
}

void USensoryComponent::BeginPlay()
{
    Super::BeginPlay();

    // 自动绑定宿主的伤害事件
    // Automatically bind to owner's damage event
    if (AActor* Owner = GetOwner())
    {
        Owner->OnTakeAnyDamage.AddDynamic(this, &USensoryComponent::HandleDamageTaken);
    }
}

void USensoryComponent::InitializeSensorySystem(UAIPerceptionComponent* InPerceptionComp)
{
    if (InPerceptionComp)
    {
        // 订阅 AIPerception 的事件
        // Subscribe to AIPerception events
        InPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &USensoryComponent::HandleTargetPerceived);
    }
}

// ==========================================
// 1. 处理感知 (Vision & Hearing)
// 1. Handle Perception (Vision & Hearing)
// ==========================================
void USensoryComponent::HandleTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor) return;

    if (Stimulus.WasSuccessfullySensed())
    {
        FString Desc;
        // 听觉 / Hearing
        if (Stimulus.Type.Name == "Default__AISense_Hearing")
        {
            Desc = FormatDescription("heard a noise from", Actor);
        }
        // 视觉 / Vision
        else 
        {
            Desc = FormatDescription("saw", Actor);
        }

        // 广播出去！/ Broadcast it!
        OnStimulusProduced.Broadcast(Desc);
    }
}

// ==========================================
// 2. 处理伤害 (Damage)
// 2. Handle Damage
// ==========================================
void USensoryComponent::HandleDamageTaken(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    FString Extra = FString::Printf(TEXT("taking %.1f damage"), Damage);
    FString Desc = FormatDescription("was attacked by", DamageCauser, Extra);
    
    // 广播出去！/ Broadcast it!
    OnStimulusProduced.Broadcast(Desc);
}

// ==========================================
// 3. 处理对话 (Speech)
// 3. Handle Speech
// ==========================================
void USensoryComponent::ReceiveSpeech(AActor* Speaker, FString Message)
{
    FString Extra = FString::Printf(TEXT("saying: \"%s\""), *Message);
    FString Desc = FormatDescription("heard", Speaker, Extra);

    // 广播出去！/ Broadcast it!
    OnStimulusProduced.Broadcast(Desc);
}

FString USensoryComponent::FormatDescription(FString Verb, AActor* Target, FString ExtraInfo)
{
    FString TargetName = Target ? Target->GetName() : "Unknown";
    if (Target && Target->ActorHasTag("Player")) TargetName = "Player";
    
    if (ExtraInfo.IsEmpty())
        return FString::Printf(TEXT("I %s %s"), *Verb, *TargetName);
    else
        return FString::Printf(TEXT("I %s %s %s"), *Verb, *TargetName, *ExtraInfo);
}