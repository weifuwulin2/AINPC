#include "Components/GoalComponent.h"
#include "Social/SocialGameplayTags.h"
#include "Components/SensoryComponent.h"
#include "Components/CognitionComponent.h"
#include "Subsystems/TimeManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Controller/UtilityAIController.h"
#include "UtilityAI/UNPCMentalState.h"
#include "AINPC.h"
#include "Config/AINPCSettings.h"


UGoalComponent::UGoalComponent()
{
	// ✅ Performance Optimization: No need to tick every frame
	// Schedule checks can run every second via timer
	PrimaryComponentTick.bCanEverTick = false;
}

void UGoalComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache Subsystems
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		TimeManager = GI->GetSubsystem<UTimeManager>();
	}

	// Cache Components
	AActor* Owner = GetOwner();
	if (Owner)
	{
		// Attempt to find Controller
		AUtilityAIController* Controller = Cast<AUtilityAIController>(Owner);
		APawn* Pawn = Cast<APawn>(Owner);

		if (!Controller && Pawn)
		{
			Controller = Cast<AUtilityAIController>(Pawn->GetController());
		}

		// If we found a controller, use it to find components (Logic often on Controller)
		// Or try finding on Owner first (Components might be on Pawn)
		
		if (Controller)
		{
			MentalState = Controller->MentalState;
			// If Sensory/Cognition are on Controller:
			if (!SensoryComp) SensoryComp = Controller->SensoryComp; // Assuming accessor or FindComponent
            if (!SensoryComp) SensoryComp = Controller->FindComponentByClass<USensoryComponent>();
            
            if (!CognitionComp) CognitionComp = Controller->CognitionComp; // Assuming public access
            if (!CognitionComp) CognitionComp = Controller->FindComponentByClass<UCognitionComponent>();
		}
        
        // Fallback: Find on Pawn/Owner directly if not found on Controller
        if (Pawn)
        {
             if (!SensoryComp) SensoryComp = Pawn->FindComponentByClass<USensoryComponent>();
             if (!CognitionComp) CognitionComp = Pawn->FindComponentByClass<UCognitionComponent>();
        }
        
        // Last resort: Owner
        if (!SensoryComp) SensoryComp = Owner->FindComponentByClass<USensoryComponent>();
        if (!CognitionComp) CognitionComp = Owner->FindComponentByClass<UCognitionComponent>();
	}

	// 4. Load Profession Config
	if (!ProfessionID.IsNone())
	{
		InitializeProfession(ProfessionID);
	}
	
	// 5. ✅ Initialize Directive immediately to avoid 'None' state
	// This ensures Directive is set before first Utility AI evaluation
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UTimeManager* TimeMgr = GameInstance->GetSubsystem<UTimeManager>())
			{
				CheckSchedule();
				UpdateArbitration();  // Set initial directive
				
				BRAIN_LOG(Log, "Initial Directive set to: %s", 
				       *CurrentDirective.ToString());
			}
		}
	}
	
	// ✅ Performance Optimization: Use timers instead of tick
	// 使用定时器替代每帧Tick，节省CPU
	// Random offset to prevent all NPCs updating simultaneously (thundering herd)
	float RandomOffset = FMath::RandRange(0.0f, 1.0f);
	
	// Arbitration timer (every 1 second)
	GetWorld()->GetTimerManager().SetTimer(
		ArbitrationTimerHandle,
		this,
		&UGoalComponent::UpdateArbitration,
		1.0f,  // Every second
		true,  // Loop
		RandomOffset  // Initial delay
	);
	
	// Schedule check timer (every ScheduleCheckInterval)
	GetWorld()->GetTimerManager().SetTimer(
		ScheduleCheckTimerHandle,
		this,
		&UGoalComponent::CheckSchedule,
		5.0f + RandomOffset,  // Every 5 seconds + offset
		true,
		RandomOffset
	);
}

void UGoalComponent::InitializeProfession(FName NewProfessionID)
{
	ProfessionID = NewProfessionID;
	
	if (ProfessionTable && !ProfessionID.IsNone())
	{
		static const FString ContextString(TEXT("GoalComponent::InitializeProfession"));
		FProfessionConfig* Row = ProfessionTable->FindRow<FProfessionConfig>(ProfessionID, ContextString);
		if (Row)
		{
			ProfessionConfig = *Row;
			BRAIN_LOG(Log, "Loaded Profession: %s", *ProfessionID.ToString());
		}
		else
		{
			BRAIN_LOG(Error, "Failed to find ProfessionID '%s' in table.", *ProfessionID.ToString());
		}
	}

	// ✅ Note: No longer need these variables - using timers instead
}


void UGoalComponent::UpdateArbitration()
{
	// ✅ Lazy initialization: Retry fetching MentalState if it's null
	// This handles cases where GoalComponent::BeginPlay runs before Controller initialization
	if (!MentalState)
	{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			AUtilityAIController* Controller = Cast<AUtilityAIController>(Owner);
			APawn* Pawn = Cast<APawn>(Owner);
			
			if (!Controller && Pawn)
			{
				Controller = Cast<AUtilityAIController>(Pawn->GetController());
			}
			
			if (Controller)
			{
				MentalState = Controller->MentalState;
				if (MentalState)
				{
					BRAIN_LOG(Warning, "✅ MentalState acquired (delayed initialization)");
				}
			}
		}
	}
	
	// 1. Survival Layer (Highest Priority)
	bool bSurvivalTriggered = false;

	// A. Threat Check
	// Note: Sensory->HasActiveThreat() needs to be checked. SensoryComponent.h has no HasActiveThreat?
	// It has OnSemanticEventSensed. We might need to query Cognition or check 'AreActorsHostile' state?
	// For now, let's use a placeholder check or check if FocusActor is Hostile.
	
	// Assuming Sensory or Controller knows about threats. 
	// Let's use Controller->GetFocusActor() and check hostility as a proxy if Sensory doesn't expose it directly.
	if (USensoryComponent::GetFaction(GetOwner()) != EFactionType::Neutral) // Only if I have faction logic
	{
		// TODO: Real Threat Check
	}

	// B. Maslow Check
	if (MentalState)
	{
		if (MentalState->Hunger > CriticalHungerThreshold || MentalState->Fatigue > CriticalHungerThreshold) 
		{
			bSurvivalTriggered = true;
			AINPC_LOG(Warning, "⚠️ SURVIVAL TRIGGERED: Hunger=%.2f, Fatigue=%.2f (Threshold=%.2f)",
			       MentalState->Hunger, MentalState->Fatigue, CriticalHungerThreshold);
		}
	}
	else
	{
		static int32 NullStateLogCounter = 0;
		if (NullStateLogCounter++ % 100 == 0)
		{
			AINPC_LOG_ERROR("❌ MentalState is NULL! Cannot check Survival needs!");
		}
	}

    if (bSurvivalTriggered)
    {
    	SetDirective(AINPCTags::Directive_Survival);
    	SetLOD(EContextLOD::Critical);
    	return;
    }

	// 🧟 Monster Check
	EFactionType OwnerFaction = USensoryComponent::GetFaction(GetOwner());
	if (OwnerFaction == EFactionType::Monster)
	{
		SetDirective(AINPCTags::Directive_Survival);
		SetLOD(EContextLOD::Standard);
		return;
	}

	// 1.5. ✅ COGNITION OVERRIDE (LLM Intention)
	// If the Brain (LLM) has explicitly decided on an intention (via MentalState), respect it!
	if (MentalState && !MentalState->Intention.IsEmpty())
	{
		FString IntentionStr = MentalState->Intention;
		
		// Skip Idle intention strings
		if (!IntentionStr.Contains("Idle") && !IntentionStr.Contains("None"))
		{
			FGameplayTag MappedDirective = FGameplayTag::EmptyTag;

			// String-based matching (robust against partial matches)
			if (IntentionStr.Contains("Attack") || IntentionStr.Contains("Combat")) MappedDirective = AINPCTags::Directive_Combat;
			else if (IntentionStr.Contains("Flee") || IntentionStr.Contains("Survival")) MappedDirective = AINPCTags::Directive_Survival;
			else if (IntentionStr.Contains("Talk") || IntentionStr.Contains("Social") || IntentionStr.Contains("Interact")) MappedDirective = AINPCTags::Directive_Social;
			else if (IntentionStr.Contains("Work")) MappedDirective = AINPCTags::Directive_Work;
			
			if (MappedDirective.IsValid())
			{
				SetDirective(MappedDirective);
				SetLOD(EContextLOD::Standard);
				return;
			}
		}
	}

	// 2. Social Layer
	bool bSocialTriggered = false;
	
	// ✅ NARRATIVE SUPPRESSION: If in a scene, suppress social needs (unless overridden)
	// 在剧情模式下，抑制社交需求，保持 NPC 专注于表演（Work/Idle）
	// ✅ NARRATIVE SUPPRESSION: If in a scene, suppress social needs (unless overridden)
	// 在剧情模式下，抑制社交需求，保持 NPC 专注于表演（Work/Idle）
	bool bInNarrativeScene = HasContextTag(AINPCTags::Status_InScene);
	
	// Legacy fallback: Check Actor tags if not found in component
	if (!bInNarrativeScene)
	{
		if (AActor* Owner = GetOwner())
		{
			// Check both Owner and Pawn/Controller if applicable
			if (Owner->ActorHasTag("Status.InScene")) bInNarrativeScene = true;
			else if (APawn* P = Cast<APawn>(Owner)) { if (P->GetController() && P->GetController()->ActorHasTag("Status.InScene")) bInNarrativeScene = true; }
			else if (AController* C = Cast<AController>(Owner)) { if (C->GetPawn() && C->GetPawn()->ActorHasTag("Status.InScene")) bInNarrativeScene = true; }
		}
	}

	if (!bInNarrativeScene)
	{
		float SocialThreshold = UAINPCSettings::Get()->SocialLonelinessThreshold;
		if (MentalState && MentalState->Loneliness > SocialThreshold)

		{
			bSocialTriggered = true;
			AINPC_LOG(Log, "💬 SOCIAL TRIGGERED: Loneliness=%.2f",
				   MentalState->Loneliness);
		}
		
		if (bSocialTriggered)
		{
			SetDirective(AINPCTags::Directive_Social);
			SetLOD(EContextLOD::Standard);
			return;
		}
	}
	else
	{
		// Optional: Log suppression if loneliness is high
		if (MentalState && MentalState->Loneliness > 0.7f)
		{
			// Subtle log, don't spam
			// AINPC_LOG(Verbose, "Social suppressed by Narrative Scene (Loneliness: %.2f)", MentalState->Loneliness);
		}
	}

	// 3. Schedule Layer (Lowest Priority) - Only for Human/Neutral NPCs
	SetDirective(CachedScheduleDirective);
	SetLOD(EContextLOD::Standard);
}

void UGoalComponent::CheckSchedule()
{
	if (!TimeManager) return;
	
	float CurrentHour = TimeManager->GetCurrentHour();
	
	// Find the task for current hour
	// Simple linear search or finding the *latest start time before now*
	
	const FScheduleTask* CurrentTask = nullptr;
	float BestStartTime = -1.0f;

	for (const FScheduleTask& Task : ProfessionConfig.Schedule)
	{
		// Handle day wrap if needed? Assuming simple 0-24 sorted for now.
		if (Task.StartTime <= CurrentHour && Task.StartTime > BestStartTime)
		{
			BestStartTime = Task.StartTime;
			CurrentTask = &Task;
		}
	}

	// Default fallback if no task found
	if (CurrentTask && CurrentTask->IsValid())
	{
		CachedScheduleDirective = CurrentTask->TaskTag;
		CachedScheduleActivity = CurrentTask->ActivityTag; // ✅ Cache Activity Tag
	}
	else
	{
		CachedScheduleDirective = AINPCTags::Directive_Idle;
		CachedScheduleActivity = FGameplayTag::EmptyTag;
	}
}

void UGoalComponent::SetDirective(FGameplayTag NewDirective)
{
	if (CurrentDirective != NewDirective)
	{
		CurrentDirective = NewDirective;
		AINPC_LOG(Warning, "🎯 Directive Changed: %s (LOD: %d)", 
			   *CurrentDirective.ToString(), (int)CurrentLOD);
		// Optional: Broadcast change
	}
}

void UGoalComponent::SetLOD(EContextLOD NewLOD)
{
	if (CurrentLOD != NewLOD)
	{
		CurrentLOD = NewLOD;
		// Notify Cognition
		if (CognitionComp)
		{
			CognitionComp->SetLOD(CurrentLOD);
		}
	}
}

void UGoalComponent::AddContextTag(FGameplayTag Tag)
{
	if (Tag.IsValid() && !ActiveContextTags.HasTagExact(Tag))
	{
		ActiveContextTags.AddTag(Tag);
		AINPC_LOG(Log, "🏷️ Context Tag Added: %s", *Tag.ToString());
		UpdateArbitration(); // Re-evaluate immediately
	}
}

void UGoalComponent::RemoveContextTag(FGameplayTag Tag)
{
	if (Tag.IsValid() && ActiveContextTags.HasTagExact(Tag))
	{
		ActiveContextTags.RemoveTag(Tag);
		AINPC_LOG(Log, "🏷️ Context Tag Removed: %s", *Tag.ToString());
		UpdateArbitration(); // Re-evaluate immediately
	}
}

bool UGoalComponent::HasContextTag(FGameplayTag Tag) const
{
	if (Tag.IsValid())
	{
		return ActiveContextTags.HasTag(Tag);
	}
	return false;
}
