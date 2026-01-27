#include "Components/SensoryComponent.h"
#include "Components/SmartObjectComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/NPCDefinitionComponent.h"
#include "Components/EmotionDisplayComponent.h"
#include "Components/MemoryComponent.h"
#include "Components/FactionReputationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Social/SocialGameplayTags.h"
#include "AINPC.h"
#include "CognitionComponent.h"
#include "UtilityAIComponent.h"
#include "UtilityAI/MentalStateInterpolation.h"
#include "UtilityAI/UNPCMentalState.h"
#include "Actions/Action_TalkTo.h"
#include "Variant_Combat/AI/CombatEnemy.h"

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
		AINPC_LOG(Log, "Transmitting Event: %s (Mag: %.2f)", *NewEvent.Content, NewEvent.Magnitude);
	}
	else
	{
		AINPC_LOG_VERBOSE("Filtered out trivial event: %s", *NewEvent.Content);
	}
}

bool USensoryComponent::ProcessEventFilter(FSemanticEvent& Event)
{
	// Physiological Layer Logic: Filter out noise
	
	// 🔍 调试：显示进入过滤器的事件
	AINPC_LOG(Warning, "ProcessEventFilter: Target=%s, Verb=%s, Magnitude=%.2f", 
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
		AINPC_LOG(Warning, "→ PASS: Immediate Threat (Danger Event)");
		return true;
	}
	
	// ✅ Rule 0.5: Death events always pass immediately (critical for memory resolution)
	// 死亡事件总是立即通过（对记忆解析至关重要）
	if (Event.Verb.MatchesTag(AINPCTags::Event_Death) ||
	    Event.Verb.MatchesTag(AINPCTags::Event_Death_Self) ||
	    Event.Verb.MatchesTag(AINPCTags::Event_Death_Witnessed))
	{
		AINPC_LOG(Warning, "→ PASS: Death Event (bypassing all filters)");
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
		
		AINPC_LOG(Warning, "[Sensory] Checking high-priority for %s: Magnitude=%.2f, IsHighPriority=%s", 
			*Event.Target->GetName(), Event.Magnitude, bIsHighPriority ? TEXT("YES") : TEXT("NO"));
		
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
			
			AINPC_LOG(Warning, "[Sensory] → PASS: High-Priority Target (Magnitude: %.2f)", Event.Magnitude);
			
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
		
		AINPC_LOG(Warning, "[Sensory] Visual Accumulation: %s count=%d/%d", 
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
			
			AINPC_LOG(Warning, "[Sensory] → PASS: Accumulation Threshold Reached");
			
			// Let it pass through
			return true;
		}
		else
		{
			// Not enough sightings yet, filter it out
			AINPC_LOG(Warning, "[Sensory] → FILTERED: Not enough accumulation yet");
			return false;
		}
	}

	// Rule 2: Mundane Activity Filter
	// If it's a "Mundane" activity with low intensity, maybe ignore it?
	if (Event.Verb.MatchesTag(AINPCTags::Interaction_Mundane) && Event.Magnitude < 0.2f)
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
        // ✅ 对话中抑制玩家视觉事件 - 但敌人事件正常通过
        // Suppress PLAYER vision events during conversation, but allow Enemy/Danger events
        if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(GetOwner()))
        {
            if (UAICon->bInConversation)
            {
                // Only suppress non-hostile targets (player)
                // 只抑制非敌对目标（玩家），敌人仍然可以打断对话
                bool bIsEnemy = Actor->ActorHasTag(TEXT("Enemy")) || AreActorsHostile(GetOwner(), Actor);
                if (!bIsEnemy)
                {
                    // Skip player vision events while in conversation
                    return;
                }
                // Enemy detected! Let this event through to trigger combat
            }
        }

        // ✅ 死亡检查：如果目标已死亡（布娃娃或有Dead标签），立即清除Focus并忽略
        // Death Check: If target is dead (ragdoll or has Dead tag), clear Focus and ignore
        bool bIsDead = Actor->ActorHasTag(TEXT("Dead"));
        if (ACharacter* Char = Cast<ACharacter>(Actor))
        {
            if (Char->GetMesh() && Char->GetMesh()->IsSimulatingPhysics())
            {
                bIsDead = true;
            }
        }
        
        if (bIsDead)
        {
            // 如果我们正在攻击这个死掉的目标，清除 Focus
            // If we were attacking this dead target, clear Focus
            if (AAIController* AIController = Cast<AAIController>(GetOwner()))
            {
                if (AIController->GetFocusActor() == Actor)
                {
                    AIController->ClearFocus(EAIFocusPriority::Gameplay);
                    AINPC_LOG(Warning, "⚰️ Target %s is dead! Cleared FocusActor.", *Actor->GetName());
                }
            }
            return; // 忽略死亡目标的感知
        }

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
        
        AINPC_LOG(Warning, "Perception: Self=%s (Faction: %s) saw Target=%s (Faction: %s), Hostile=%s", 
            *GetOwner()->GetName(), *SelfFaction.ToString(),
            *Actor->GetName(), *TargetFaction.ToString(),
            bIsHostile ? TEXT("YES") : TEXT("NO"));
            
        // ✅ Bind to CombatEnemy Death Delegate
        // 绑定战斗敌人的死亡委托，以便在其死亡时将其记忆标记为已解决
        if (ACombatEnemy* CombatEnemy = Cast<ACombatEnemy>(Actor))
        {
             if (!CombatEnemy->OnEnemyDied.IsAlreadyBound(this, &USensoryComponent::HandleCombatEnemyDeath))
             {
                 CombatEnemy->OnEnemyDied.AddDynamic(this, &USensoryComponent::HandleCombatEnemyDeath);
             }
        }
        
        // 优先级 1: 检查阵营敌对关系（最高优先级，绕过累积系统）
        // Priority 1: Check faction hostility (highest priority, bypass accumulation)
        if (bIsHostile)
        {
            Magnitude = 0.8f; // 敌对阵营：高重要性，直接传到 Cognition (Hostile faction: High importance, direct to Cognition)
            AINPC_LOG(Warning, "→ Hostile faction detected! Magnitude set to 0.8");
            
            // ✅ 设置 FocusActor 为敌对目标，让 Attack 动作知道攻击谁
            // Set FocusActor to hostile target so Attack action knows who to attack
            if (AAIController* AIController = Cast<AAIController>(GetOwner()))
            {
                AIController->SetFocus(Actor, EAIFocusPriority::Gameplay);
				AINPC_LOG(Log, "[Sensory] → Set FocusActor to hostile target: %s", *Actor->GetName());
				
				// ✅ CRITICAL: Directly set Perceived_Threat (AMYGDALA HIJACK for Mindless Creatures)
				// 关键：直接设置威胁感知（为无意识生物提供杏仁核劫持反应）
				// This is ESSENTIAL for Zombies who have bEnableReasoning=false!
				if (AUtilityAIController* UtilController = Cast<AUtilityAIController>(AIController))
				{
					if (UtilController->CognitionComp && UtilController->CognitionComp->Interpolator)
					{
						UtilController->CognitionComp->Interpolator->SetTargetValue(TEXT("Perceived_Threat"), 0.9f);
						AINPC_LOG(Warning, "[Sensory] 🧠⚡ HOSTILE DETECTED! Spiking Perceived_Threat to 0.9 (Amygdala Response)");
					}
				}
            }
        }
        // 优先级 2: 玩家是中等优先级（进入累积系统）
        // Priority 2: Player is medium priority (enters accumulation system)
        else if (Actor->ActorHasTag("Player"))
        {
            Magnitude = 0.3f; // 玩家：中等重要性，需要累积 (Player: Medium importance, requires accumulation)
			AINPC_LOG(Warning, "[Sensory] → Player detected! Magnitude set to 0.3");
        }
        // 优先级 3: 同阵营（友军）
        // Priority 3: Same faction (friendly)
        else
        {
            if (SelfFaction == TargetFaction && SelfFaction != "Neutral")
            {
                Magnitude = 0.1f; // 同阵营：低重要性，需要累积 (Same faction: Low importance, requires accumulation)
				AINPC_LOG(Warning, "[Sensory] → Same faction! Magnitude set to 0.1");
            }
            else
            {
                Magnitude = 0.1f; // 中立或未知：默认低重要性 (Neutral/Unknown: Default low importance)
				AINPC_LOG(Warning, "[Sensory] → Neutral/Unknown! Magnitude set to 0.1");
            }
        }

        // Generate text description with faction/hostility context
        // 生成包含阵营/敌对信息的描述
        FString Desc = FormatDescriptionWithContext(Verb, Actor, SelfFaction, TargetFaction, bIsHostile);

        // New: Generate Semantic Event
        FSemanticEvent Event;
        Event.Instigator = GetOwner();
        Event.Target = Actor;
        Event.Verb = PerceptionTag;
        Event.Content = Desc;
        Event.Magnitude = Magnitude;

        // 标记目标已被感知
        MarkTargetPerceived(Actor);

        // ⚠️ CRITICAL FIX: 敌对单位绕过过滤器，直接传递
        // Hostile units bypass filter and go directly to Cognition
        bool bShouldBroadcast = false;
        
        if (Magnitude >= 0.5f)
        {
            // 高重要性事件（敌对单位、危险）：直接传递，不过滤
            // High importance events (hostile, danger): Direct pass, no filter
            bShouldBroadcast = true;
			AINPC_LOG(Warning, "[Sensory] → HIGH PRIORITY (Mag=%.2f): Bypassing filter!", Magnitude);
        }
        else
        {
            // 低重要性事件：通过过滤器（累积系统）
            // Low importance events: Through filter (accumulation system)
            bShouldBroadcast = ProcessEventFilter(Event);
        }
        
        if (bShouldBroadcast)
        {
            // ✅ Single Event Path: Semantic Memory System
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
    
    // Generate Semantic Event
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
    // Filter: Don't hear yourself speak
    AActor* Owner = GetOwner();
    if (Speaker == Owner) return;
    if (AAIController* OwnerController = Cast<AAIController>(Owner))
    {
        if (Speaker == OwnerController->GetPawn()) return;
    }

    // Validate
    if (!Speaker || Message.IsEmpty()) return;

    // 1. Get Speaker Name (try for better name)
    FString SpeakerName = Speaker->GetName();
    if (APawn* SpeakerPawn = Cast<APawn>(Speaker))
    {
         if (UNPCDefinitionComponent* DefComp = SpeakerPawn->FindComponentByClass<UNPCDefinitionComponent>())
         {
             FNPCNameDef NameDef;
             if (DefComp->GetNameDef(NameDef))
             {
                 SpeakerName = NameDef.FirstName; 
             }
         }
    }
    
    AINPC_LOG(Warning, "💬 SPEECH RECEIVED from %s: \"%s\"", *SpeakerName, *Message);

    // 2. Format message for display
    FString FormattedMsg = FString::Printf(TEXT("%s: %s"), *SpeakerName, *Message);

    // 3. Trigger EmotionDisplayComponent
    if (AActor* ActorToCheck = Cast<AActor>(Speaker))
    {
        UEmotionDisplayComponent* EmotionComp = ActorToCheck->FindComponentByClass<UEmotionDisplayComponent>();
        if (!EmotionComp)
        {
            if (APawn* P = Cast<APawn>(ActorToCheck))
            {
               if (AController* C = P->GetController())
               {
                   EmotionComp = C->FindComponentByClass<UEmotionDisplayComponent>();
               }
            }
        }

        if (EmotionComp)
        {
            EmotionComp->ShowSpeechBubble(Message);
        }
    }

    // 4. Generate Semantic Event - with topic continuation guidance
    FSemanticEvent Event;
    Event.Instigator = Speaker;
    Event.Target = GetOwner();
    Event.Verb = AINPCTags::Social_Chat;
    Event.Content = FString::Printf(TEXT("%s said to you: \"%s\". Listen and respond naturally to what they just said."), *SpeakerName, *Message);
    Event.Magnitude = 0.8f; 

    if (ProcessEventFilter(Event))
    {
        OnSemanticEventSensed.Broadcast(Event);
    }

    // ✅ 5. Player Speech Interruption Logic (打断机制)
    // When PLAYER speaks to this NPC, boost Loneliness to trigger Social directive
    // This will interrupt Sleep/Eat actions and make the NPC respond
    if (Speaker && Speaker->ActorHasTag(TEXT("Player")))
    {
        if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(GetOwner()))
        {
            if (UAICon->MentalState)
            {
                // Boost Loneliness to trigger Social directive switch
                float OldLoneliness = UAICon->MentalState->Loneliness;
                UAICon->MentalState->Loneliness = FMath::Min(1.0f, UAICon->MentalState->Loneliness + 0.5f);
                
                AINPC_LOG(Warning, "🗣️ Player spoke! Boosting Loneliness: %.2f -> %.2f (Interruption trigger)", 
                    OldLoneliness, UAICon->MentalState->Loneliness);
                
                // Sync with Interpolator if available
                if (UAICon->CognitionComp && UAICon->CognitionComp->Interpolator)
                {
                    UAICon->CognitionComp->Interpolator->SetTargetValue(TEXT("Loneliness"), UAICon->MentalState->Loneliness);
                }
            }
            
            // ✅ Reset conversation timer - NPC will respond immediately instead of waiting
            // 重置对话计时器 - NPC 立即回复而不是等待
            if (UAICon->UtilityComp && UAICon->UtilityComp->CurrentAction)
            {
                if (UAction_TalkTo* TalkAction = Cast<UAction_TalkTo>(UAICon->UtilityComp->CurrentAction))
                {
                    TalkAction->ResetConversationTimer();
                    AINPC_LOG(Log, "⏱️ Conversation timer reset - player spoke");
                }
            }
        }
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
        
        // Use FormatDescription for clarity
        FString KillerDesc = Killer ? FormatDescription("was killed by", Killer, "") : TEXT("I have died");
        Desc = KillerDesc.IsEmpty() ? TEXT("I have died") : KillerDesc;
        
        Magnitude = 1.0f; // 最高重要性
        
		AINPC_LOG(Warning, "[Sensory] SELF DEATH EVENT: %s", *Desc);
    }
    else
    {
        // 目睹他人死亡 - 重要但不如自己死亡
        DeathTag = AINPCTags::Event_Death_Witnessed;
        
        // ✅ Use FormatDescription to show meaningful names (Personality/Profession)
        // Instead of "BP_NPC_C_123", show "Cautious (Miner)"
        FString DeadActorDesc = FormatDescription("witnessed the death of", DeadActor, "");
        
        if (Killer)
        {
            FString KillerName = "Unknown";
            
            // Get killer's meaningful name
            if (APawn* KillerPawn = Cast<APawn>(Killer))
            {
                if (AController* KillerController = KillerPawn->GetController())
                {
                    if (UPersonalityComponent* PersonalityComp = KillerController->FindComponentByClass<UPersonalityComponent>())
                    {
                        if (!PersonalityComp->PersonalityID.IsNone())
                        {
                            KillerName = PersonalityComp->PersonalityID.ToString();
                        }
                    }
                }
            }
            else if (Killer->ActorHasTag("Player"))
            {
                KillerName = "Player";
            }
            
            if (KillerName == "Unknown")
            {
                KillerName = Killer->GetName();
            }
            
            Desc = DeadActorDesc + FString::Printf(TEXT(" (killed by %s)"), *KillerName);
        }
        else
        {
            Desc = DeadActorDesc;
        }
        
        Magnitude = 0.7f; // 高重要性
        
		AINPC_LOG(Warning, "[Sensory] WITNESSED DEATH: %s", *Desc);
        
        // ✅ 如果死者是我们的攻击目标，立即清除 Focus
        // If the dead actor was our attack target, clear Focus immediately
        if (AAIController* AIController = Cast<AAIController>(Owner))
        {
            if (AIController->GetFocusActor() == DeadActor)
            {
                AIController->ClearFocus(EAIFocusPriority::Gameplay);
				AINPC_LOG(Warning, "[Sensory] ⚰️ Target %s is dead! Cleared FocusActor.", *DeadActor->GetName());
            }
            
            // ✅ NEW: Reset Perceived_Threat when hostile enemy dies
            // 当敌对目标死亡时，立即重置威胁感知
            // This prevents the NPC from staying in "threat mode" after the enemy is gone
            // ⚠️ Only reset if the dead actor was HOSTILE (not friendly NPC death)
            if (AreActorsHostile(Owner, DeadActor))
            {
                if (AUtilityAIController* UtilController = Cast<AUtilityAIController>(AIController))
                {
                    if (UtilController->MentalState)
                    {
                        float OldThreat = UtilController->MentalState->Perceived_Threat;
                        UtilController->MentalState->Perceived_Threat = 0.0f;
						AINPC_LOG(Warning, "[Sensory] ✅ Hostile enemy dead! Reset Perceived_Threat: %.2f -> 0.0", OldThreat);
                        
                        // Also update Interpolator target to prevent bounce-back from LLM
                        if (UtilController->CognitionComp && UtilController->CognitionComp->Interpolator)
                        {
                            UtilController->CognitionComp->Interpolator->SetTargetValue(TEXT("Perceived_Threat"), 0.0f);
                        }
                    }
                }
            }
        }
        
        // ✅ Mark all memories about the dead actor as RESOLVED
        // 将关于死者的所有记忆标记为已解决（防止"僵尸循环"问题）
        if (UMemoryComponent* MemoryComp = Owner->FindComponentByClass<UMemoryComponent>())
        {
             // Try both Smart Name (e.g. "zombie") and Object Name (e.g. "BP_Zombie_C_0")
             // 尝试使用 Smart Name 和 Object Name 进行清除
             FString SmartName = GetSmartActorName(DeadActor);
             MemoryComp->MarkMemoriesResolvedForActor(SmartName);
             MemoryComp->MarkMemoriesResolvedForActor(DeadActor->GetName());
        }
    }

    // ✅ 过滤器: 使用与视觉相同的过滤机制
    // Filter: Use same filtering mechanism as sight
    
    // ⚠️ CRITICAL: Do NOT block death events based on vision cooldown!
    // Death is a unique event and must always be processed even if we just saw the actor.
    // 我们不应该因为最近刚看到该 Actor 就即使是在它死亡时也忽略它。
    // if (!ShouldPerceiveTarget(DeadActor)) return; <-- REMOVED
    
    MarkTargetPerceived(DeadActor); // Update timestamp so we don't "see" it again instantly as a corpse

    // Generate Semantic Event
    FSemanticEvent Event;
    Event.Instigator = Killer ? Killer : DeadActor; // 凶手或死者
    Event.Target = DeadActor;
    Event.Verb = DeathTag;
    Event.Content = Desc;
    Event.Magnitude = Magnitude;

    // 2. Process via Filter (respects magnitude and other rules)
    // 通过过滤器处理（遵循重要性和其他规则）
    if (ProcessEventFilter(Event))
    {
        OnSemanticEventSensed.Broadcast(Event);
    }
}

FString USensoryComponent::FormatDescription(FString Verb, AActor* Target, FString ExtraInfo)
{
    FString TargetName = "Unknown";
    FString ProfessionInfo = "";
    
    if (Target)
    {
        if (APawn* TargetPawn = Cast<APawn>(Target))
        {
            if (AController* TargetController = TargetPawn->GetController())
            {
                // 1. Try UtilityAIController (Personality)
                if (AUtilityAIController* UtilityController = Cast<AUtilityAIController>(TargetController))
                {
                    if (UtilityController->PersonalityComp && !UtilityController->PersonalityComp->PersonalityID.IsNone())
                    {
                        TargetName = UtilityController->PersonalityComp->PersonalityID.ToString();
                    }
                }

                // 2. Try NPCDefinition (Profession) - Check Controller first
                if (UNPCDefinitionComponent* NPCDef = TargetController->FindComponentByClass<UNPCDefinitionComponent>())
                {
                    if (!NPCDef->ProfessionID.IsNone())
                    {
                        ProfessionInfo = FString::Printf(TEXT(" (%s)"), *NPCDef->ProfessionID.ToString());
                    }
                }
            }
            
            // 3. Try NPCDefinition on Pawn (CombatEnemy case)
            if (ProfessionInfo.IsEmpty())
            {
                 if (UNPCDefinitionComponent* NPCDef = TargetPawn->FindComponentByClass<UNPCDefinitionComponent>())
                 {
                    if (!NPCDef->ProfessionID.IsNone())
                    {
                        ProfessionInfo = FString::Printf(TEXT(" (%s)"), *NPCDef->ProfessionID.ToString());
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
    
    // Append Profession to Name if available
    FString DisplayName = TargetName + ProfessionInfo;
    
    if (ExtraInfo.IsEmpty())
        return FString::Printf(TEXT("I %s %s"), *Verb, *DisplayName);
    else
        return FString::Printf(TEXT("I %s %s %s"), *Verb, *DisplayName, *ExtraInfo);
}

// ✅ NEW: Enhanced description with faction/hostility context for LLM
// 新增：包含阵营/敌对信息的增强描述，供 LLM 使用
FString USensoryComponent::FormatDescriptionWithContext(FString Verb, AActor* Target, FName SelfFaction, FName TargetFaction, bool bIsHostile)
{
    FString TargetName = "Unknown";
    FString ProfessionInfo = "";
    
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

                // Try NPCDefinition (Profession)
                if (UNPCDefinitionComponent* NPCDef = TargetController->FindComponentByClass<UNPCDefinitionComponent>())
                {
                    if (!NPCDef->ProfessionID.IsNone())
                    {
                        ProfessionInfo = FString::Printf(TEXT(" (%s)"), *NPCDef->ProfessionID.ToString());
                    }
                }
            }

            // Try NPCDefinition on Pawn
            if (ProfessionInfo.IsEmpty())
            {
                 if (UNPCDefinitionComponent* NPCDef = TargetPawn->FindComponentByClass<UNPCDefinitionComponent>())
                 {
                    if (!NPCDef->ProfessionID.IsNone())
                    {
                        ProfessionInfo = FString::Printf(TEXT(" (%s)"), *NPCDef->ProfessionID.ToString());
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
    
    // ✅ Build context-aware description
    // 构建包含上下文的描述
    // Append Profession Info
    FString DisplayName = TargetName + ProfessionInfo;
    FString Description = FString::Printf(TEXT("I %s %s"), *Verb, *DisplayName);
    
    // Add faction information
    // 添加阵营信息
    if (TargetFaction != "Neutral" && TargetFaction != "None")
    {
        Description += FString::Printf(TEXT(" (Faction: %s)"), *TargetFaction.ToString());
    }
    
    // ⚠️ CRITICAL: Add hostility warning for LLM
    // 关键：为 LLM 添加敌对警告
    if (bIsHostile)
    {
        Description += TEXT(" - HOSTILE ENEMY DETECTED!");
    }
    else if (SelfFaction == TargetFaction && SelfFaction != "Neutral")
    {
        Description += TEXT(" - Ally");
    }
    
    return Description;
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



EFactionType USensoryComponent::GetFaction(AActor* Actor)
{
    if (!Actor) return EFactionType::Neutral;

    // ✅ FIX: If the Actor is a Controller (not a Pawn), get its controlled Pawn
    // SensoryComponent is attached to Controller, so GetOwner() returns Controller
    // 修复：如果 Actor 是 Controller（而非 Pawn），获取其控制的 Pawn
    APawn* Pawn = Cast<APawn>(Actor);
    if (!Pawn)
    {
        if (AController* Controller = Cast<AController>(Actor))
        {
            Pawn = Controller->GetPawn();
        }
    }
    
    // ✅ REFACTORED: Faction is now managed by FactionReputationComponent, not PersonalityComponent
    // 重构：Faction 现在由 FactionReputationComponent 管理，而非 PersonalityComponent
    // PersonalityConfig no longer has a Faction field
    
    // 1. Try FactionReputationComponent first
    // 1. 优先尝试 FactionReputationComponent
    if (Pawn)
    {
        if (UFactionReputationComponent* FacComp = Pawn->FindComponentByClass<UFactionReputationComponent>())
        {
            // Convert FName to EFactionType
            FName FactionID = FacComp->CurrentFactionID;
            if (FactionID == "Human" || FactionID == "Humans") return EFactionType::Human;
            if (FactionID == "Monster" || FactionID == "Monsters") return EFactionType::Monster;
            if (FactionID == "Orc" || FactionID == "Orcs") return EFactionType::Monster; // Orcs are hostile
            if (FactionID == "Elf" || FactionID == "Elves") return EFactionType::Human; // Elves are peaceful
        }
        
        // Try on controller
        if (AController* Controller = Pawn->GetController())
        {
            if (UFactionReputationComponent* FacComp = Controller->FindComponentByClass<UFactionReputationComponent>())
            {
                FName FactionID = FacComp->CurrentFactionID;
                if (FactionID == "Human" || FactionID == "Humans") return EFactionType::Human;
                if (FactionID == "Monster" || FactionID == "Monsters") return EFactionType::Monster;
                if (FactionID == "Orc" || FactionID == "Orcs") return EFactionType::Monster;
                if (FactionID == "Elf" || FactionID == "Elves") return EFactionType::Human;
            }
        }
        
        // ✅ CRITICAL: Fallback to NPCDefinitionComponent (for CombatEnemy which has no FactionReputat ionComponent)
        // 关键：回退到 NPCDefinitionComponent（适用于没有 FactionReputationComponent 的 CombatEnemy）
        if (UNPCDefinitionComponent* DefComp = Pawn->FindComponentByClass<UNPCDefinitionComponent>())
        {
            FName FactionID = DefComp->FactionID;
            if (FactionID == "Human" || FactionID == "Humans") return EFactionType::Human;
            if (FactionID == "Monster" || FactionID == "Monsters") return EFactionType::Monster; 
            if (FactionID == "Orc" || FactionID == "Orcs") return EFactionType::Monster;
            if (FactionID == "Elf" || FactionID == "Elves") return EFactionType::Human;
        }
    }

    // 2. Fallback: Tag Mapping (Configuration Layer for non-AI actors)
    // ⚠️ CRITICAL: Check tags on the PAWN, not the Controller!
    // Tags are added to the Pawn in CombatEnemy::BeginPlay
    AActor* ActorToCheck = Pawn ? Pawn : Actor;
    
    // Monsters
    if (ActorToCheck->ActorHasTag("Zombie")) return EFactionType::Monster;
    if (ActorToCheck->ActorHasTag("Bandit")) return EFactionType::Monster;
    if (ActorToCheck->ActorHasTag("Monster")) return EFactionType::Monster;

    // Humans
    if (ActorToCheck->ActorHasTag("Human") || 
        ActorToCheck->ActorHasTag("Guard") || 
        ActorToCheck->ActorHasTag("Merchant") || 
        ActorToCheck->ActorHasTag("Villager")) 
    {
        return EFactionType::Human;
    }
    
    // Player -> Human
    if (ActorToCheck->ActorHasTag("Player")) return EFactionType::Human;
    if (Pawn && Pawn->IsPlayerControlled()) return EFactionType::Human;

    return EFactionType::Neutral;
}

bool USensoryComponent::AreActorsHostile(AActor* ActorA, AActor* ActorB) const
{
	if (!ActorA || !ActorB) return false;
	if (ActorA == ActorB) return false;

    // ✅ Use FactionReputationComponent for numerical check (0-100)
    // First try Component on A (Primary Source of Truth)
    if (UFactionReputationComponent* FacCompA = ActorA->FindComponentByClass<UFactionReputationComponent>())
    {
        return FacCompA->IsHostile(ActorB);
    }

    // Fallback: If A is just a Pawn, maybe its Controller has the component
    if (APawn* PawnA = Cast<APawn>(ActorA))
    {
        if (AController* ConA = PawnA->GetController())
        {
             if (UFactionReputationComponent* FacCompCon = ConA->FindComponentByClass<UFactionReputationComponent>())
             {
                 return FacCompCon->IsHostile(ActorB);
             }
        }
    }

    // Double Fallback: Check from B's perspective (Mutual Hostility?)
    if (UFactionReputationComponent* FacCompB = ActorB->FindComponentByClass<UFactionReputationComponent>())
    {
        return FacCompB->IsHostile(ActorA);
    }

    // Final Fallback: Legacy Enum Logic (for actors without components)
    EFactionType FactionA = GetFaction(ActorA);
    EFactionType FactionB = GetFaction(ActorB);

    if (FactionA == EFactionType::Neutral || FactionB == EFactionType::Neutral)
    {
        return false; 
    }
    return FactionA != FactionB;
}

// Non-const implementations for compatibility
FName USensoryComponent::GetActorFaction(AActor* Actor) const
{
	// Try new FactionReputationComponent
	if (UFactionReputationComponent* FacComp = Actor->FindComponentByClass<UFactionReputationComponent>())
	{
		return FacComp->CurrentFactionID;
	}

	// Fallback: Check FactionReputationComponent static helper
	return UFactionReputationComponent::GetFactionID(Actor);
}


AActor* USensoryComponent::FindBestSmartObject(FGameplayTag ActivityTag)
{
    AActor* BestCandidate = nullptr;
    float ClosestDistSq = FLT_MAX;
    FVector MyLoc = GetOwner()->GetActorLocation();

    // 1. 优先检查最近感知到的 Actor (Short-term Memory)
    // Priority 1: Check recently perceived actors
    for (auto& Pair : RecentlyPerceivedActors)
    {
        AActor* Candidate = Pair.Key;
        if (!IsValid(Candidate)) continue;

        // 检查是否有 SmartObjectComponent
        USmartObjectComponent* SmartComp = Candidate->FindComponentByClass<USmartObjectComponent>();
        if (SmartComp && SmartComp->Semantics.SocialTag.MatchesTag(ActivityTag))
        {
            float DistSq = FVector::DistSquared(MyLoc, Candidate->GetActorLocation());
            if (DistSq < ClosestDistSq)
            {
                ClosestDistSq = DistSq;
                BestCandidate = Candidate;
            }
        }
    }

    // 2. 如果记忆里没有，为了 Showcase 的鲁棒性，我们可以扫描一定范围内的所有 Actor
    // Priority 2: Fallback scan (only for showcase smoothness, can be removed for strict realism)
    if (!BestCandidate)
    {
        TArray<AActor*> AllActors;
        // 这是一个昂贵的操作，但在 demo 关卡中通常可以接受
        // In a real game, you'd use a spatial grid or specific tag queries
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);
        
        for (AActor* Candidate : AllActors)
        {
             if (!IsValid(Candidate)) continue;
             if (Candidate == GetOwner()) continue;
             
             // 距离检查 (放宽到 15000 cm / 150m，防止跑远了找不到床)
             // Distance check (Relaxed to 15000cm / 150m to find bed even if far)
             float DistSq = FVector::DistSquared(MyLoc, Candidate->GetActorLocation());
             if (DistSq > 15000.0f * 15000.0f) continue;

             USmartObjectComponent* SmartComp = Candidate->FindComponentByClass<USmartObjectComponent>();
             if (SmartComp && SmartComp->Semantics.SocialTag.MatchesTag(ActivityTag))
             {
                 if (DistSq < ClosestDistSq)
                 {
                     ClosestDistSq = DistSq;
                     BestCandidate = Candidate;
                 }
             }
        }
    }

    return BestCandidate;
}



void USensoryComponent::HandleCombatEnemyDeath(ACombatEnemy* DeadEnemy, AActor* Killer)
{
    HandleDeath(DeadEnemy, Killer);
}

FString USensoryComponent::GetSmartActorName(AActor* Target)
{
    FString TargetName = "Unknown";
    if (!Target) return TargetName;

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
    
    return TargetName;
}
