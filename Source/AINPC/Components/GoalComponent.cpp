#include "Components/GoalComponent.h"
#include "Components/SensoryComponent.h"
#include "Components/CognitionComponent.h"
#include "Subsystems/TimeManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Controller/UtilityAIController.h"
#include "UtilityAI/UNPCMentalState.h"

UGoalComponent::UGoalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f; // Run Arbitration at 5Hz, not every frame
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
				
				UE_LOG(LogTemp, Log, TEXT("[GoalComponent] Initial Directive set to: %s"), 
				       *CurrentDirective.ToString());
			}
		}
	}
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
			UE_LOG(LogTemp, Log, TEXT("[GoalComponent] Loaded Profession: %s"), *ProfessionID.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GoalComponent] Failed to find ProfessionID '%s' in table."), *ProfessionID.ToString());
		}
	}


	// Randomize offset to prevent thundering herd
	ScheduleCheckRandomOffset = FMath::RandRange(0.0f, 2.0f);
	TimeSinceLastScheduleCheck = ScheduleCheckInterval + ScheduleCheckRandomOffset; // Force immediate check
}

void UGoalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateArbitration();

	// Time Slicing for Schedule
	// Only run schedule optimization if we are NOT in Critical mode (LOD 0), 
	// because in Critical mode we don't care about schedule, and when we exit Critical we want quick update.
	// Actually, simpler logic: Always tick timer, but only check if valid.
	
	TimeSinceLastScheduleCheck += DeltaTime;
	if (TimeSinceLastScheduleCheck >= (ScheduleCheckInterval + ScheduleCheckRandomOffset))
	{
		CheckSchedule();
		TimeSinceLastScheduleCheck = 0.0f;
		ScheduleCheckRandomOffset = FMath::RandRange(-0.5f, 0.5f); // Jitter
	}
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
					UE_LOG(LogTemp, Warning, TEXT("[GoalComponent] ✅ MentalState acquired (delayed initialization)"));
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
		// FIX: Use Fatigue Threshold for Fatigue check, and > for Fatigue as it increases?
		// Wait, Fatigue: 0=Rested, 1=Tired. Hunger: 0=Full, 1=Starving.
		// So both should check > Threshold.
		// Original code had `Fatigue < CriticalHungerThreshold` which seems wrong if Fatigue is 0-1 (High is bad).
		// Assuming Fatigue increases over time.
		
		if (MentalState->Hunger > CriticalHungerThreshold || MentalState->Fatigue > CriticalHungerThreshold) 
		{
			bSurvivalTriggered = true;
			UE_LOG(LogTemp, Warning, TEXT("[GoalComponent] ⚠️ SURVIVAL TRIGGERED: Hunger=%.2f, Fatigue=%.2f (Threshold=%.2f)"),
			       MentalState->Hunger, MentalState->Fatigue, CriticalHungerThreshold);
		}
	}
	else
	{
		// Only log once every 100 frames to avoid spam
		static int32 NullStateLogCounter = 0;
		if (NullStateLogCounter++ % 100 == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[GoalComponent] ❌ MentalState is NULL! Cannot check Survival needs!"));
		}
	}

	// C. Health Check
	// TODO: Get Health

    if (bSurvivalTriggered)
    {
    	SetDirective(FGameplayTag::RequestGameplayTag("Directive.Survival"));
    	SetLOD(EContextLOD::Critical);
    	return;
    }

	// 🧟 Monster Check: Monsters only have Survival instincts, no Social/Work
	// Zombies, beasts, etc. should not have schedules or social needs
	EFactionType OwnerFaction = USensoryComponent::GetFaction(GetOwner());
	if (OwnerFaction == EFactionType::Monster)
	{
		// Monsters are always in "Survival" mode
		// This gives bonus to Attack/Flee/Eat/Sleep, but penalizes Work/Social
		SetDirective(FGameplayTag::RequestGameplayTag("Directive.Survival"));
		SetLOD(EContextLOD::Standard);
		return;
	}

	// 2. Social Layer (Medium Priority) - Only for Human/Neutral NPCs
	// Trigger Social directive if NPC is lonely
	// Note: We don't check for actual friendly NPCs here. The Talk action itself has
	// HasFriendlyNearby as a Context, so if no friends are nearby, Talk score will be 0.
	// The directive just indicates "I want to socialize", the action decides if it's possible.
	bool bSocialTriggered = false;
	
	if (MentalState && MentalState->Loneliness > 0.5f)
	{
		bSocialTriggered = true;
		UE_LOG(LogTemp, Log, TEXT("[GoalComponent] 💬 SOCIAL TRIGGERED: Loneliness=%.2f"),
		       MentalState->Loneliness);
	}
	
	if (bSocialTriggered)
	{
		SetDirective(FGameplayTag::RequestGameplayTag("Directive.Social"));
		SetLOD(EContextLOD::Standard);
		return;
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
	}
	else
	{
		CachedScheduleDirective = FGameplayTag::RequestGameplayTag("Directive.Idle");
	}
}

void UGoalComponent::SetDirective(FGameplayTag NewDirective)
{
	if (CurrentDirective != NewDirective)
	{
		CurrentDirective = NewDirective;
		UE_LOG(LogTemp, Warning, TEXT("[GoalComponent] 🎯 Directive Changed: %s (LOD: %d)"), 
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
