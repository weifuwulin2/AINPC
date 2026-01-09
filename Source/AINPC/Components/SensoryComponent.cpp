#include "Components/SensoryComponent.h"
#include "Components/SmartObjectComponent.h"
#include "Social/SocialGameplayTags.h"

USensoryComponent::USensoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

void USensoryComponent::HandleSmartObjectInteraction(AActor* Instigator, AActor* SmartObjectActor)
{
	if (!SmartObjectActor) return;

	// 1. Get Semantics from the Object
	USmartObjectComponent* SmartComp = SmartObjectActor->FindComponentByClass<USmartObjectComponent>();
	if (!SmartComp) return; // Not a smart object

	// 2. The Interaction Equation (Interaction Logic)
	// Transforms static data (Semantics) into dynamic runtime data (Event)
	FSemanticEvent NewEvent;
	NewEvent.Instigator = Instigator;
	NewEvent.Target = SmartObjectActor;
	NewEvent.Verb = SmartComp->Semantics.SocialTag;
	NewEvent.Content = ParseDescriptionTemplate(SmartComp->Semantics.DescriptionTemplate, Instigator, SmartObjectActor);
	NewEvent.Magnitude = SmartComp->Semantics.Intensity;

	// 3. Process Logic (The "Spinal Cord")
	if (ProcessEventFilter(NewEvent))
	{
		// 4. Send to Brain (Memory/Cognition)
		OnSemanticEventSensed.Broadcast(NewEvent);
		UE_LOG(LogTemp, Log, TEXT("[Sensory] Transmitting Event: %s (Mag: %.2f)"), *NewEvent.Content, NewEvent.Magnitude);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Sensory] Filtered out trivial event: %s"), *NewEvent.Content);
	}
}

bool USensoryComponent::ProcessEventFilter(FSemanticEvent& Event)
{
	// Physiological Layer Logic: Filter out noise
	// If it's a "Mundane" activity with low intensity, maybe ignore it?
	if (Event.Verb.MatchesTag(AINPCTags::Activity_Mundane) && Event.Magnitude < 0.2f)
	{
		return false; // Ignore trivial background noise
	}

	// Calculate distance factor here? (Example: Far away insults might be ignored)
	if (GetOwner() && Event.Instigator)
	{
		float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Event.Instigator->GetActorLocation());
		if (Distance > 2000.0f && !Event.Verb.MatchesTag(AINPCTags::Event_Global))
		{
			// Too far away to care, unless it's a global event (like a nuke)
			return false; 
		}
	}

	return true;
}

FString USensoryComponent::ParseDescriptionTemplate(const FString& Template, AActor* Instigator, AActor* Target)
{
	// Simple string replacement: {Instigator} -> Name, {Target} -> Name
	FString Result = Template;
	FString InstigatorName = Instigator ? Instigator->GetActorLabel() : TEXT("Someone");
	FString TargetName = Target ? Target->GetActorLabel() : TEXT("Object");

	Result.ReplaceInline(TEXT("{Instigator}"), *InstigatorName);
	Result.ReplaceInline(TEXT("{Target}"), *TargetName);

	return Result;
}

// ==========================================
// Legacy Logic (Restored for Stability)
// ==========================================

#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "GameFramework/Character.h"
#include "Controller/UtilityAIController.h"
#include "Components/PersonalityComponent.h"
#include "Engine/DamageEvents.h"

void USensoryComponent::InitializeSensorySystem(UAIPerceptionComponent* InPerceptionComp)
{
    if (InPerceptionComp)
    {
        InPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &USensoryComponent::HandleTargetPerceived);
    }
}

void USensoryComponent::HandleTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor) return;

    if (Stimulus.WasSuccessfullySensed())
    {
        // ✅ 注意力过滤：检查是否应该感知这个目标
        // Attention Filter: Check if we should perceive this target
        if (!ShouldPerceiveTarget(Actor))
        {
            return; // 冷却中，忽略
        }

        // 确定感知类型和动词
        // Determine perception type and verb
        FGameplayTag PerceptionTag;
        FString Verb;
        
        if (Stimulus.Type.Name == "Default__AISense_Hearing")
        {
            PerceptionTag = AINPCTags::Perception_Hearing;
            Verb = "heard a noise from";
        }
        else 
        {
            PerceptionTag = AINPCTags::Perception_Vision;
            Verb = "saw";

            // Refinement: If it's a dangerous visual confirmation, Upgrade the Tag!
            // This ensures Memory System verifies this as a High Importance event (+5 score)
            if (Actor->ActorHasTag("Enemy"))
            {
                PerceptionTag = AINPCTags::Event_Danger;
            }
        }

        // Legacy: Generate text description
        FString Desc = FormatDescription(Verb, Actor);
        OnStimulusProduced.Broadcast(Desc);

        // ✅ 智能重要性计算：根据目标类型调整
        // Smart Importance Calculation: Adjust based on target type
        float Magnitude = 0.3f; // 默认：中等重要性
        
        // 检查目标是否是敌人
        if (Actor->ActorHasTag("Enemy"))
        {
            Magnitude = 0.8f; // 敌人：高重要性 (Base)
        }
        // 检查是否是玩家
        else if (Actor->ActorHasTag("Player"))
        {
            Magnitude = 0.5f; // 玩家：中高重要性
        }
        // 检查是否是友军（同类）
        else if (APawn* TargetPawn = Cast<APawn>(Actor))
        {
            if (APawn* SelfPawn = Cast<APawn>(GetOwner()))
            {
                // 如果是同类（相同类型的 Pawn）
                if (TargetPawn->GetClass() == SelfPawn->GetClass())
                {
                    Magnitude = 0.2f; // 友军：低重要性
                }
            }
        }

        // New: Generate Semantic Event
        FSemanticEvent Event;
        Event.Instigator = GetOwner();
        Event.Target = Actor;
        Event.Verb = PerceptionTag;
        Event.Content = Desc;
        Event.Magnitude = Magnitude;

        // 标记目标已被感知
        MarkTargetPerceived(Actor);

        // Filter and broadcast
        if (ProcessEventFilter(Event))
        {
            OnSemanticEventSensed.Broadcast(Event);
        }
    }
}

// ✅ 注意力追踪辅助方法实现
// Attention Tracking Helper Methods Implementation

bool USensoryComponent::ShouldPerceiveTarget(AActor* Target)
{
    if (!Target) return false;

    // 定期清理过期记录
    CleanupPerceptionTracking();

    // 检查是否在冷却中
    if (float* LastPerceivedTime = RecentlyPerceivedActors.Find(Target))
    {
        UWorld* World = GetWorld();
        if (World)
        {
            float CurrentTime = World->GetTimeSeconds();
            float TimeSinceLastPerception = CurrentTime - *LastPerceivedTime;
            
            if (TimeSinceLastPerception < PerceptionCooldown)
            {
                // 还在冷却中
                return false;
            }
        }
    }

    return true;
}

void USensoryComponent::MarkTargetPerceived(AActor* Target)
{
    if (!Target) return;

    UWorld* World = GetWorld();
    if (World)
    {
        float CurrentTime = World->GetTimeSeconds();
        RecentlyPerceivedActors.Add(Target, CurrentTime);
    }
}

void USensoryComponent::CleanupPerceptionTracking()
{
    UWorld* World = GetWorld();
    if (!World) return;

    float CurrentTime = World->GetTimeSeconds();
    
    // 移除过期的记录（超过冷却时间 * 2）
    TArray<AActor*> ToRemove;
    for (auto& Pair : RecentlyPerceivedActors)
    {
        if (CurrentTime - Pair.Value > PerceptionCooldown * 2.0f)
        {
            ToRemove.Add(Pair.Key);
        }
    }

    for (AActor* Actor : ToRemove)
    {
        RecentlyPerceivedActors.Remove(Actor);
    }
}

void USensoryComponent::HandleDamageTaken(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    FString Extra = FString::Printf(TEXT("taking %.1f damage"), Damage);
    FString Desc = FormatDescription("was attacked by", DamageCauser, Extra);
    
    // Legacy: Broadcast text
    OnStimulusProduced.Broadcast(Desc);

    // New: Generate Semantic Event
    FSemanticEvent Event;
    Event.Instigator = DamageCauser;
    Event.Target = GetOwner();
    Event.Verb = AINPCTags::Perception_Damage;
    Event.Content = Desc;
    Event.Magnitude = FMath::Clamp(Damage / 100.0f, 0.0f, 1.0f); // Normalize damage to 0-1

    // Damage is always important, bypass filter
    OnSemanticEventSensed.Broadcast(Event);
}

void USensoryComponent::ReceiveSpeech(AActor* Speaker, FString Message)
{
    FString Extra = FString::Printf(TEXT("saying: \"%s\""), *Message);
    FString Desc = FormatDescription("heard", Speaker, Extra);

    // Legacy: Broadcast text
    OnStimulusProduced.Broadcast(Desc);

    // New: Generate Semantic Event
    FSemanticEvent Event;
    Event.Instigator = Speaker;
    Event.Target = GetOwner();
    Event.Verb = AINPCTags::Social_Chat; // Speech is social interaction
    Event.Content = Desc;
    Event.Magnitude = 0.5f; // Medium importance

    // Filter and broadcast
    if (ProcessEventFilter(Event))
    {
        OnSemanticEventSensed.Broadcast(Event);
    }
}

void USensoryComponent::HandleDeath(AActor* DeadActor, AActor* Killer)
{
    if (!DeadActor) return;

    AActor* Owner = GetOwner();
    bool bIsSelfDeath = (DeadActor == Owner);

    FString Desc;
    FGameplayTag DeathTag;
    float Magnitude;

    if (bIsSelfDeath)
    {
        // 自己死亡 - 极其重要的事件
        DeathTag = AINPCTags::Event_Death_Self;
        Desc = FString::Printf(TEXT("I have died%s"), 
            Killer ? *FString::Printf(TEXT(" (killed by %s)"), *Killer->GetName()) : TEXT(""));
        Magnitude = 1.0f; // 最高重要性
        
        UE_LOG(LogTemp, Warning, TEXT("[Sensory] SELF DEATH EVENT: %s"), *Desc);
    }
    else
    {
        // 目睹他人死亡 - 重要但不如自己死亡
        DeathTag = AINPCTags::Event_Death_Witnessed;
        Desc = FString::Printf(TEXT("I witnessed %s die%s"), 
            *DeadActor->GetName(),
            Killer ? *FString::Printf(TEXT(" (killed by %s)"), *Killer->GetName()) : TEXT(""));
        Magnitude = 0.7f; // 高重要性
        
        UE_LOG(LogTemp, Log, TEXT("[Sensory] WITNESSED DEATH: %s"), *Desc);
    }

    // Legacy: Broadcast text
    OnStimulusProduced.Broadcast(Desc);

    // New: Generate Semantic Event
    FSemanticEvent Event;
    Event.Instigator = Killer ? Killer : DeadActor; // 凶手或死者
    Event.Target = DeadActor;
    Event.Verb = DeathTag;
    Event.Content = Desc;
    Event.Magnitude = Magnitude;

    // 死亡事件总是重要的，直接广播不过滤
    OnSemanticEventSensed.Broadcast(Event);
}

FString USensoryComponent::FormatDescription(FString Verb, AActor* Target, FString ExtraInfo)
{
    FString TargetName = "Unknown";
    
    if (Target)
    {
        if (APawn* TargetPawn = Cast<APawn>(Target))
        {
            if (AController* TargetController = TargetPawn->GetController())
            {
                if (AUtilityAIController* UtilityController = Cast<AUtilityAIController>(TargetController))
                {
                    if (UtilityController->PersonalityComp && !UtilityController->PersonalityComp->PersonalityID.IsNone())
                    {
                        TargetName = UtilityController->PersonalityComp->PersonalityID.ToString();
                    }
                }
            }
        }
        
        if (TargetName == "Unknown" && Target->ActorHasTag("Player"))
        {
            TargetName = "Player";
        }
        
        if (TargetName == "Unknown")
        {
            TargetName = Target->GetName();
        }
    }
    
    if (ExtraInfo.IsEmpty())
        return FString::Printf(TEXT("I %s %s"), *Verb, *TargetName);
    else
        return FString::Printf(TEXT("I %s %s %s"), *Verb, *TargetName, *ExtraInfo);
}