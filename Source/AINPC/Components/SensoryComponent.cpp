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
	
	// 🔍 调试：显示进入过滤器的事件
	UE_LOG(LogTemp, Warning, TEXT("[Sensory] ProcessEventFilter: Target=%s, Verb=%s, Magnitude=%.2f"), 
		Event.Target ? *Event.Target->GetName() : TEXT("NULL"),
		*Event.Verb.ToString(),
		Event.Magnitude);
	
	// ✅ Rule 0: Immediate Threat Response (Bypass Accumulation)
	// 即时威胁响应：对于敌人和玩家，立即触发认知事件
	// Immediate Threat Response: For enemies and players, trigger cognitive event immediately
	if (Event.Verb.MatchesTag(AINPCTags::Event_Danger))
	{
		// This is already upgraded to Danger in HandleTargetPerceived
		// Let it pass through immediately
		UE_LOG(LogTemp, Warning, TEXT("[Sensory] → PASS: Immediate Threat (Danger Event)"));
		return true;
	}
	
	// Check if target is a high-priority entity based on magnitude
	// 基于 Magnitude 检查目标是否是高优先级实体
	// (Magnitude is already set in HandleTargetPerceived based on faction/player status)
	if (bEnableImmediateResponse && Event.Target && Event.Verb.MatchesTag(AINPCTags::Perception_Vision))
	{
		// High magnitude (>= 0.5) indicates high-priority target (player or hostile faction)
		// 高 Magnitude (>= 0.5) 表示高优先级目标（玩家或敌对阵营）
		bool bIsHighPriority = Event.Magnitude >= 0.5f;
		
		UE_LOG(LogTemp, Warning, TEXT("[Sensory] Checking high-priority: Magnitude=%.2f, IsHighPriority=%s"), 
			Event.Magnitude, bIsHighPriority ? TEXT("YES") : TEXT("NO"));
		
		if (bIsHighPriority)
		{
			// Upgrade to immediate awareness for high-priority targets
			// 对高优先级目标升级为即时感知
			Event.Verb = AINPCTags::Social_Greet; // Or use a dedicated "Awareness" tag
			Event.Magnitude = FMath::Max(Event.Magnitude, HighPriorityMagnitude); // Ensure minimum magnitude
			
			FName TargetFaction = GetActorFaction(Event.Target);
			FName SelfFaction = GetActorFaction(GetOwner());
			bool bIsPlayer = Event.Target->ActorHasTag("Player");
			
			Event.Content = FString::Printf(TEXT("I noticed %s (%s%s)"), 
				*Event.Target->GetName(), 
				bIsPlayer ? TEXT("Player, ") : TEXT(""),
				*TargetFaction.ToString());
			
			UE_LOG(LogTemp, Warning, TEXT("[Sensory] → PASS: High-Priority Target (Magnitude: %.2f)"), Event.Magnitude);
			
			// Reset accumulation to prevent duplicate events
			ResetVisualAccumulation(Event.Target);
			
			return true;
		}
	}
	
	// Rule 1: Visual Accumulation Filter (for low-priority targets)
	// 视觉累积过滤器（用于低优先级目标）
	// Just "Seeing" someone (Friend/Neutral) is NOT immediately a Cognitive Event.
	// It belongs to the Fast System (Blackboard/Targeting), not Semantic Memory.
	// However, if we see them REPEATEDLY, it becomes noteworthy.
	if (Event.Verb.MatchesTag(AINPCTags::Perception_Vision))
	{
		// Note: "Event.Danger" is NOT "Perception.Vision" (it was swapped), so Danger passes this check.
		// If it is still Perception.Vision, it means it wasn't upgraded to Danger.
		
		// Increment accumulation count
		int32 CurrentCount = IncrementVisualAccumulation(Event.Target);
		
		UE_LOG(LogTemp, Warning, TEXT("[Sensory] Visual Accumulation: %s count=%d/%d"), 
			*Event.Target->GetName(), CurrentCount, AccumulationThreshold);
		
		if (CurrentCount >= AccumulationThreshold)
		{
			// Threshold reached! This becomes a cognitive event.
			// Upgrade the tag to indicate "Awareness" rather than just "Vision"
			Event.Verb = AINPCTags::Social_Greet; // Or create a new tag like "Social.Awareness"
			Event.Magnitude = 0.4f; // Medium importance
			Event.Content = FString::Printf(TEXT("I've noticed %s is around frequently"), 
				*Event.Target->GetName());
			
			// Reset counter after triggering
			ResetVisualAccumulation(Event.Target);
			
			UE_LOG(LogTemp, Warning, TEXT("[Sensory] → PASS: Accumulation Threshold Reached"));
			
			// Let it pass through
			return true;
		}
		else
		{
			// Not enough sightings yet, filter it out
			UE_LOG(LogTemp, Warning, TEXT("[Sensory] → FILTERED: Not enough accumulation yet"));
			return false;
		}
	}

	// Rule 2: Mundane Activity Filter
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


        // ✅ 智能重要性计算：基于阵营和目标类型调整
        // Smart Importance Calculation: Adjust based on faction and target type
        float Magnitude = 0.1f; // 默认：非常低 (Default: Very Low for just "seeing")
        
        // 🔍 调试：获取阵营信息
        FName SelfFaction = GetActorFaction(GetOwner());
        FName TargetFaction = GetActorFaction(Actor);
        bool bIsHostile = AreActorsHostile(GetOwner(), Actor);
        
        UE_LOG(LogTemp, Warning, TEXT("[Sensory] Perception: Self=%s (Faction: %s) saw Target=%s (Faction: %s), Hostile=%s"), 
            *GetOwner()->GetName(), *SelfFaction.ToString(),
            *Actor->GetName(), *TargetFaction.ToString(),
            bIsHostile ? TEXT("YES") : TEXT("NO"));
        
        // 优先级 1: 检查阵营敌对关系（最高优先级，绕过累积系统）
        // Priority 1: Check faction hostility (highest priority, bypass accumulation)
        if (bIsHostile)
        {
            Magnitude = 0.8f; // 敌对阵营：高重要性，直接传到 Cognition (Hostile faction: High importance, direct to Cognition)
            UE_LOG(LogTemp, Warning, TEXT("[Sensory] → Hostile faction detected! Magnitude set to 0.8"));
            
            // ✅ 设置 FocusActor 为敌对目标，让 Attack 动作知道攻击谁
            // Set FocusActor to hostile target so Attack action knows who to attack
            if (AAIController* AIController = Cast<AAIController>(GetOwner()))
            {
                AIController->SetFocus(Actor, EAIFocusPriority::Gameplay);
                UE_LOG(LogTemp, Log, TEXT("[Sensory] → Set FocusActor to hostile target: %s"), *Actor->GetName());
            }
        }
        // 优先级 2: 玩家是中等优先级（进入累积系统）
        // Priority 2: Player is medium priority (enters accumulation system)
        else if (Actor->ActorHasTag("Player"))
        {
            Magnitude = 0.3f; // 玩家：中等重要性，需要累积 (Player: Medium importance, requires accumulation)
            UE_LOG(LogTemp, Warning, TEXT("[Sensory] → Player detected! Magnitude set to 0.3"));
        }
        // 优先级 3: 同阵营（友军）
        // Priority 3: Same faction (friendly)
        else
        {
            if (SelfFaction == TargetFaction && SelfFaction != "Neutral")
            {
                Magnitude = 0.1f; // 同阵营：低重要性，需要累积 (Same faction: Low importance, requires accumulation)
                UE_LOG(LogTemp, Warning, TEXT("[Sensory] → Same faction! Magnitude set to 0.1"));
            }
            else
            {
                Magnitude = 0.1f; // 中立或未知：默认低重要性 (Neutral/Unknown: Default low importance)
                UE_LOG(LogTemp, Warning, TEXT("[Sensory] → Neutral/Unknown! Magnitude set to 0.1"));
            }
        }

        // Generate text description (for both paths)
        FString Desc = FormatDescription(Verb, Actor);

        // New: Generate Semantic Event
        FSemanticEvent Event;
        Event.Instigator = GetOwner();
        Event.Target = Actor;
        Event.Verb = PerceptionTag;
        Event.Content = Desc;
        Event.Magnitude = Magnitude;

        // 标记目标已被感知
        MarkTargetPerceived(Actor);

        // ⚠️ CRITICAL: Filter BEFORE broadcasting to either system
        // This prevents visual spam from reaching Cognition/Memory
        if (ProcessEventFilter(Event))
        {
            // Legacy Path: For backward compatibility with old Cognition logic
            OnStimulusProduced.Broadcast(Desc);
            
            // New Path: Semantic Memory System
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
        // Also clean up accumulation count for this actor
        VisualAccumulationCount.Remove(Actor);
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

// ==========================================
// Visual Accumulation Helpers
// ==========================================

int32 USensoryComponent::IncrementVisualAccumulation(AActor* Target)
{
    if (!Target) return 0;
    
    // Get current count, or 0 if not found
    int32* CurrentCount = VisualAccumulationCount.Find(Target);
    if (CurrentCount)
    {
        (*CurrentCount)++;
        return *CurrentCount;
    }
    else
    {
        // First sighting
        VisualAccumulationCount.Add(Target, 1);
        return 1;
    }
}

void USensoryComponent::ResetVisualAccumulation(AActor* Target)
{
    if (!Target) return;
    
    VisualAccumulationCount.Remove(Target);
}

// ==========================================
// Faction Detection Helpers
// ==========================================

FName USensoryComponent::GetActorFaction(AActor* Actor) const
{
	if (!Actor) return "Neutral";
	
	UPersonalityComponent* PersonalityComp = nullptr;
	
	// Case 1: Actor is a Controller (e.g., when GetOwner() is called on SensoryComponent)
	// 情况 1: Actor 是 Controller（例如在 SensoryComponent 上调用 GetOwner() 时）
	if (AController* Controller = Cast<AController>(Actor))
	{
		PersonalityComp = Controller->FindComponentByClass<UPersonalityComponent>();
	}
	// Case 2: Actor is a Pawn, get its Controller
	// 情况 2: Actor 是 Pawn，获取其 Controller
	else if (APawn* Pawn = Cast<APawn>(Actor))
	{
		if (AController* Controller1 = Pawn->GetController())
		{
			PersonalityComp = Controller1->FindComponentByClass<UPersonalityComponent>();
		}
	}
	
	// If we found PersonalityComponent, return its Faction
	// 如果找到了 PersonalityComponent，返回其 Faction
	if (PersonalityComp)
	{
		return PersonalityComp->Personality.Faction;
	}
	
	// Fallback: Check tags
	if (Actor->ActorHasTag("Player"))
	{
		return "Human";
	}
	
	return "Neutral";
}

bool USensoryComponent::AreActorsHostile(AActor* ActorA, AActor* ActorB) const
{
	if (!ActorA || !ActorB) return false;
	
	FName FactionA = GetActorFaction(ActorA);
	FName FactionB = GetActorFaction(ActorB);
	
	// Neutral faction is never hostile
	// 中立阵营永远不敌对
	if (FactionA == "Neutral" || FactionB == "Neutral")
	{
		return false;
	}
	
	// Same faction is not hostile
	// 同阵营不敌对
	if (FactionA == FactionB)
	{
		return false;
	}
	
	// Different non-neutral factions are hostile
	// 不同的非中立阵营互相敌对
	return true;
}
