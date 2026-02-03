#include "CognitionComponent.h"
#include "AINPC.h"
#include "FactionReputationComponent.h"
#include "SensoryComponent.h"
#include "UtilityAIComponent.h"
#include "Controller/UtilityAIController.h"
#include "Components/PersonalityComponent.h"
#include "Components/GoalComponent.h"
#include "Social/ProfessionTypes.h"
#include "Social/FactionSubsystem.h"
#include "Social/FactionTypes.h"
#include "LLM/LLMCommunicator.h"
#include "Components/NPCDefinitionComponent.h"
#include "Social/FactionSubsystem.h"
#include "UtilityAI/SentimentMapping.h"
#include "Subsystems/NarrativeDirectorSubsystem.h"
#include "Subsystems/NarrativeSquadSubsystem.h"
#include "UtilityAI/MentalStateInterpolation.h"
#include "Components/MemoryComponent.h" // ✅ Added
#include "Social/SocialTypes.h"         // ✅ Added
#include "Utilities/FactionHelpers.h"   // ✅ Added for GetAttitudeDescription

// ✅ Performance Tracking

UCognitionComponent::UCognitionComponent()
{
	// 大脑需要每帧 Tick 来更新插值
	// Brain needs to Tick every frame to update interpolation
	PrimaryComponentTick.bCanEverTick = true;
}

void UCognitionComponent::BeginPlay()
{
	Super::BeginPlay();

	// 0. 获取 Owner 上的 MemoryComponent
	// Get MemoryComponent from Owner
	if (AActor* Owner = GetOwner())
	{
		MemoryComp = Owner->FindComponentByClass<UMemoryComponent>();
		if (!MemoryComp)
		{
			AINPC_LOG_WARNING("No MemoryComponent found on Owner! Memories will not be stored/retrieved.");
		}
		else
		{
			AINPC_LOG(Log, "Connected to MemoryComponent.");
			
			// ✅ Bind OnDreamingNeeded to StartDreaming
			MemoryComp->OnDreamingNeeded.AddDynamic(this, &UCognitionComponent::StartDreaming);
		}
	}

	// 1. 初始化 LLM 服务对象
	LLMService = NewObject<ULLMCommunicator>(this);

	// 2. 读取配置 (逻辑从 Controller 移到了这里)
	FString ConfigApiKey;
	FString ConfigApiUrl;

	GConfig->GetString(TEXT("LLM.Settings"), TEXT("ApiKey"), ConfigApiKey, GGameIni);
	GConfig->GetString(TEXT("LLM.Settings"), TEXT("ApiUrl"), ConfigApiUrl, GGameIni);

	// 安全检查
	if (ConfigApiKey.IsEmpty())
	{
		AINPC_LOG_ERROR("FATAL: API Key not found in DefaultGame.ini!");
		return;
	}

	if (ConfigApiUrl.IsEmpty())
	{
		ConfigApiUrl = TEXT("https://api.deepseek.com/chat/completions");
	}

	// 3. 初始化 Service
	LLMService->Init(ConfigApiKey, ConfigApiUrl);
    
	AINPC_LOG(Log, "Brain Initialized via Config.");

	// 4. 初始化语义映射器 / Initialize Sentiment Mapper
	SentimentMapper = NewObject<USentimentMapper>(this);
	SentimentMapper->Initialize();  // 使用默认映射
	AINPC_LOG(Log, "SentimentMapper initialized");

	// 5. 初始化插值器 / Initialize Interpolator
	Interpolator = NewObject<UMentalStateInterpolator>(this);
	
	// 配置不同情绪的插值速度 / Configure interpolation speeds
	FMentalStateInterpConfig FastConfig;
	FastConfig.InterpSpeed = 5.0f;  // 快速变化
	FastConfig.SnapThreshold = 0.01f;
	FastConfig.bEnableRandomPerturbation = true;
	FastConfig.PerturbationRange = 0.05f;
	
	FMentalStateInterpConfig MediumConfig;
	MediumConfig.InterpSpeed = 2.0f;  // 中速变化
	MediumConfig.SnapThreshold = 0.01f;
	MediumConfig.bEnableRandomPerturbation = true;
	MediumConfig.PerturbationRange = 0.05f;
	
	FMentalStateInterpConfig SlowConfig;
	SlowConfig.InterpSpeed = 0.5f;  // 慢速变化
	SlowConfig.SnapThreshold = 0.01f;
	SlowConfig.bEnableRandomPerturbation = true;
	SlowConfig.PerturbationRange = 0.03f;
	
	// 应用配置 / Apply configurations
	Interpolator->SetInterpConfig("Indignity", FastConfig);        // 屈辱：快速 (5.0)
	Interpolator->SetInterpConfig("Perceived_Threat", FastConfig); // 威胁：快速
	
	Interpolator->SetInterpConfig("Hunger", MediumConfig);         // 饥饿：中速 (2.0)
	Interpolator->SetInterpConfig("Fatigue", MediumConfig);        // 疲劳：中速
	Interpolator->SetInterpConfig("Loneliness", MediumConfig);     // 孤独：中速
	
	Interpolator->SetInterpConfig("Trust", SlowConfig);            // 信任：慢速 (0.5) (兼容)
	Interpolator->SetInterpConfig("Boredom", SlowConfig);          // 无聊：慢速
	
	AINPC_LOG(Log, "Interpolator initialized with custom speeds");
}

void UCognitionComponent::BeginDestroy()
{
	// ✅ P3 Fix: Clean up timers explicitly
	if (UWorld* World = GetWorld())
	{
		if (RetryStimulusTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(RetryStimulusTimerHandle);
		}
	}
	
	Super::BeginDestroy();
}

void UCognitionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 获取 MentalState（从 Controller）
	// Get MentalState (from Controller)
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (AIController)
	{
		AUtilityAIController* UtilityController = Cast<AUtilityAIController>(AIController);
		if (UtilityController && UtilityController->MentalState && Interpolator)
		{
			// 每帧更新插值
			// Update interpolation every frame
			Interpolator->UpdateInterpolation(UtilityController->MentalState, DeltaTime);
		}
	}
}

void UCognitionComponent::SetLOD(EContextLOD NewLOD)
{
	if (CurrentLOD != NewLOD)
	{
		CurrentLOD = NewLOD;
		// Optional: Log LOD change
		// AINPC_LOG(Log, "[Cognition] 🧠 Switched to %s", *UEnum::GetValueAsString(CurrentLOD));
	}
}

void UCognitionComponent::ProcessStimulus(FString SituationDescription)
{
	// 🔍 DEBUG TRACE: Force Visibility with LogTemp Error
	UE_LOG(LogTemp, Error, TEXT("🔍 [Cognition] ProcessStimulus Called! Input Len: %d | Data: %.20s"), 
		SituationDescription.Len(), *SituationDescription);

	// 1. Amygdala Hijack
	if (CheckAmygdalaHijack(SituationDescription)) {
		UE_LOG(LogTemp, Warning, TEXT("[Cognition] Exiting due to Amygdala Hijack."));
		return;
	}
	if (!bEnableReasoning) {
		UE_LOG(LogTemp, Warning, TEXT("[Cognition] Exiting due to Reasoning Disabled."));
		return;
	}

	// 2. Memory Retrieval (No return here)
	// ... (Code omitted for brevity, assuming standard execution) ...

	// 3. Get Basic IDs
	FString PersonalityIDStr = "Unknown";
	FString FactionStr = "Neutral";
	FString RoleDesc = RoleDescription;

	if (AAIController* AICon = Cast<AAIController>(GetOwner()))
	{
		if (AUtilityAIController* UtilCon = Cast<AUtilityAIController>(AICon))
		{
			if (UPersonalityComponent* PersComp = UtilCon->PersonalityComp)
			{
				PersonalityIDStr = PersComp->PersonalityID.ToString();
				RoleDesc = PersComp->Personality.RoleDescription.IsEmpty() ? RoleDesc : PersComp->Personality.RoleDescription;
			}
		}
		if (UNPCDefinitionComponent* DefComp = GetOwner()->FindComponentByClass<UNPCDefinitionComponent>())
			if (!DefComp->FactionID.IsNone()) FactionStr = DefComp->FactionID.ToString();
	}

	UE_LOG(LogTemp, Warning, TEXT("[Cognition] IDs Extracted - Personality: %s, Faction: %s"), *PersonalityIDStr, *FactionStr);

	// 4. Data Readiness Check
	if (!IsDataReady(PersonalityIDStr, FactionStr, SituationDescription)) {
		UE_LOG(LogTemp, Error, TEXT("[Cognition] ❌ Exiting: Data Not Ready."));
		return;
	}

	// 5. Rate Limiting (check BEFORE building prompt to avoid wasted work)
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bool bIsHighPriority = SituationDescription.Contains(TEXT("HOSTILE")) || SituationDescription.Contains(TEXT("DANGER"));
	bool bIsPlayerSpeech = SituationDescription.Contains(TEXT("said to you"));
	float Cooldown = bIsHighPriority ? 1.5f : 4.0f;

	if (bIsPlayerSpeech)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Cognition] Player Speech detected - bypassing rate limit"));
	}
	else if (CurrentTime - LastLLMRequestTime < Cooldown)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Cognition] Rate Limited. Time: %.2f, Last: %.2f, Cd: %.2f"), CurrentTime, LastLLMRequestTime, Cooldown);
		return;
	}
	LastLLMRequestTime = CurrentTime;

	// 6. Build Prompt Blocks (token-optimized)
	// Full detail only for high-priority or player speech; simplified for routine stimuli
	bool bFullDetail = bIsHighPriority || bIsPlayerSpeech || (CurrentLOD == EContextLOD::Standard);

	FString IdentityBlock = BuildIdentityBlock(RoleDesc, PersonalityIDStr, FactionStr, bFullDetail);
	FString WorldviewBlock = BuildWorldviewBlock(FactionStr, SituationDescription);

	FString ProfessionName = "", ProfessionDesc = "";
	if (AAIController* AICon = Cast<AAIController>(GetOwner()))
		if (UGoalComponent* Goal = AICon->FindComponentByClass<UGoalComponent>()) {
			ProfessionName = Goal->ProfessionConfig.ProfessionName.ToString();
			ProfessionDesc = Goal->ProfessionConfig.Description;
		}
	FString ContextBlock = BuildContextBlock(ProfessionName, ProfessionDesc);

	// Conditional World State: only include for high-priority/player speech
	FString GlobalHistory = "";
	if (bFullDetail)
	{
		if (UWorld* World = GetWorld())
			if (UNarrativeDirectorSubsystem* Dir = World->GetSubsystem<UNarrativeDirectorSubsystem>())
				GlobalHistory = Dir->GetWorldStateDescription(3);
	}

	// Memory retrieval
	FString ContextMemory = "None";
	if (MemoryComp) {
		float CurrentGameTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		TArray<FMemoryItem> Memories = MemoryComp->RetrieveRelevantMemories(SituationDescription, 5);
		ContextMemory = "";
		for (const FMemoryItem& Item : Memories) ContextMemory += FString::Printf(TEXT("- %s\n"), *MemoryComp->GetFormattedDescription(Item, CurrentGameTime));
	}

	FString VolatileBlock = BuildVolatileBlock(SituationDescription, ContextMemory, GlobalHistory);

	// 7. Assemble Final Prompt (Instructions moved to LLMCommunicator system prompt)
	FString Prompt = FString::Printf(TEXT(
		"%s"   // 1. Identity
		"%s\n" // 2. Worldview (filtered)
		"%s\n" // 3. Context
		"%s\n" // 4. Volatile
	), *IdentityBlock, *WorldviewBlock, *ContextBlock, *VolatileBlock);

	AINPC_LOG(Warning, TEXT("━━━━━━━━━━ [Cognition] LLM REQUEST ━━━━━━━━━━"));
	AINPC_LOG(Warning, TEXT("%s"), *Prompt);
	AINPC_LOG(Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));

	LLMService->SendRoleplayRequest(Prompt, FOnLLMResponse::CreateUObject(this, &UCognitionComponent::OnLLMReply));
}

void UCognitionComponent::OnLLMReply(bool bSuccess, const FMentalState& NewState)
{
	if (bSuccess)
	{
		// ✅ Log complete LLM response
		AINPC_LOG(Warning, TEXT("━━━━━━━━━━ [Cognition] 📥 LLM RESPONSE ━━━━━━━━━━"));
		AINPC_LOG(Warning, TEXT("  🗣️ Speech: \"%s\""), NewState.Speech.IsEmpty() ? TEXT("(none)") : *NewState.Speech);
		AINPC_LOG(Warning, TEXT("  🎭 Emotion: %s"), NewState.Emotion.IsEmpty() ? TEXT("Neutral") : *NewState.Emotion);
		AINPC_LOG(Warning, TEXT("  🎯 Intention: %s"), NewState.Intention.IsEmpty() ? TEXT("(none)") : *NewState.Intention);
		AINPC_LOG(Warning, TEXT("  📊 Threat: %.2f | Indignity: %.2f | Loneliness: %.2f | Boredom: %.2f"), 
			NewState.Perceived_Threat, NewState.Indignity, NewState.Loneliness, NewState.Boredom);
		AINPC_LOG(Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));

		if (Interpolator)
		{
			// 使用宏自动为所有字段设置目标值
			// Use macro to automatically set target values for all fields
			#define SET_TARGET_VALUE(Name, DefaultValue, DisplayName, Description) \
				Interpolator->SetTargetValue(TEXT(#Name), NewState.Name);
			
			MENTAL_STATE_FIELDS(SET_TARGET_VALUE)
			
			#undef SET_TARGET_VALUE
			
			// 注意：实际的 MentalState 更新在 TickComponent 中通过插值完成
			// Note: Actual MentalState update happens in TickComponent via interpolation
			
			// 仍然广播事件，但现在是"目标值已设置"的通知
			// Still broadcast event, but now it's a "target values set" notification
			OnMentalStateChanged.Broadcast(NewState);

			// ✅ 触发 Utility AI 的详细日志
			// Trigger detailed Utility AI logging
			if (AAIController* AIController = Cast<AAIController>(GetOwner()))
			{
				if (UUtilityAIComponent* UtilityComp = AIController->FindComponentByClass<UUtilityAIComponent>())
				{
					UtilityComp->RequestDebugLog();
				}
			}
		}
		else
		{
			AINPC_LOG_WARNING("Interpolator not initialized! Broadcasting directly.");
			OnMentalStateChanged.Broadcast(NewState);
		}
	}
	else
	{
		AINPC_LOG_ERROR("Failed to process thought.");
	}
}

// CognitionComponent.cpp

void UCognitionComponent::StartDreaming()
{
	if (!MemoryComp || !LLMService) return;

	// 1. Get top memories by importance (token-efficient, skip low-value noise)
	FString DailyLogs = MemoryComp->GetTopMemoriesAsString(15);
	if (DailyLogs.IsEmpty()) return;

	// 2. Functional request: deterministic analysis, capped output
	FString SystemPrompt = TEXT(
		"You are a memory consolidation AI. Analyze NPC memories and extract key insights.\n"
		"Output Format: A pure JSON array of 3 strings. Example: [\"Player is hostile\", \"Food is scarce\"]\n"
		"Do NOT output markdown. Do NOT explain."
	);

	FString UserPrompt = FString::Printf(TEXT("Here are my recent memories:\n%s"), *DailyLogs);

	AINPC_LOG(Log, "[Dreaming] Sending %d top memories to LLM...", 15);

	LLMService->SendFunctionalRequest(
		SystemPrompt,
		UserPrompt,
		FOnLLMResponseRaw::CreateUObject(this, &UCognitionComponent::OnDreamingAnalysisComplete),
		0.3f,   // Low temperature for deterministic analysis
		false,  // No JSON mode (output is JSON array, not object)
		256     // Cap output tokens
	);
}

void UCognitionComponent::OnDreamingAnalysisComplete(bool bSuccess, const FString& RawResponse)
{
	if (!bSuccess) return;

	// 4. 解析 JSON 数组
	// RawResponse 应该是 ["Insight 1", "Insight 2"]
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawResponse);

	TArray<FString> ExtractedInsights;
    
	if (FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		for (auto Value : JsonArray)
		{
			ExtractedInsights.Add(Value->AsString());
		}
	}

	// 5. 存入长期记忆
	if (MemoryComp && ExtractedInsights.Num() > 0)
	{
		MemoryComp->ConsolidateMemories(ExtractedInsights);
		
		// ✅ Clear short-term memories after consolidation
		MemoryComp->ClearShortTermMemories();
		AINPC_LOG(Log, "Dream cycle complete: %d insights consolidated", ExtractedInsights.Num());
	}
}

void UCognitionComponent::ReportDecisionContext(const FString& WinnerName, const FString& RunnerUpName, float ConflictLevel)
{
	// Only report if there is meaningful conflict (ConflictLevel < 0.2 means scores are close)
	// Lower ConflictLevel = Higher Conflict (because it's percentage diff)
    // 0.0 = Equal, 1.0 = Winner is infinitely better
	
	if (ConflictLevel < 0.25f && !RunnerUpName.IsEmpty())
	{
		CurrentDecisionContext = FString::Printf(TEXT("[INTERNAL CONFLICT] You are torn between '%s' and '%s'. The urge is almost equal."), 
			*WinnerName, *RunnerUpName);
	}
	else
	{
		CurrentDecisionContext = ""; // Clear if decision is clear
	}
}

// =========================================================
// ✅ REFACTOR HELPERS
// =========================================================

bool UCognitionComponent::CheckAmygdalaHijack(const FString& SituationDescription)
{
	// ✅ PROPER AMYGDALA HIJACK: Query actual hostile actors in perception
	// 杏仁核劫持：查询感知范围内的真实敌对目标（基于 Faction Attitude）
	
	if (!Interpolator) return false;

	// Get SensoryComponent from owner
	USensoryComponent* SensoryComp = nullptr;
	if (AAIController* AICon = Cast<AAIController>(GetOwner()))
	{
		if (APawn* ControlledPawn = AICon->GetPawn())
		{
			SensoryComp = ControlledPawn->FindComponentByClass<USensoryComponent>();
		}
	}

	if (!SensoryComp) return false;

	// Check if any recently perceived actors are hostile
	// Access private member via reflection workaround is complex, but we can use a simpler approach:
	// The SensoryComponent should have already sent "I see [HOSTILE] X" if there was a hostile actor.
	// BUT the user wants us to query directly, not rely on text.
	
	// Alternative: Check if owner has a FocusActor that is hostile
	AActor* FocusActor = nullptr;
	if (AAIController* AICon = Cast<AAIController>(GetOwner()))
	{
		FocusActor = AICon->GetFocusActor();
	}

	if (FocusActor)
	{
		// Use SensoryComponent's public helper to check hostility
		// But AreActorsHostile is private... we need to expose it or use another method.
		// Let's check Faction directly via FactionSubsystem.
		
		FName SelfFaction = NAME_None;
		FName TargetFaction = NAME_None;
		
		// Get self faction
		if (AAIController* AICon = Cast<AAIController>(GetOwner()))
		{
			if (APawn* SelfPawn = AICon->GetPawn())
			{
				if (UNPCDefinitionComponent* DefComp = SelfPawn->FindComponentByClass<UNPCDefinitionComponent>())
				{
					SelfFaction = DefComp->FactionID;
				}
			}
		}
		
		// Get target faction
		if (UNPCDefinitionComponent* TargetDefComp = FocusActor->FindComponentByClass<UNPCDefinitionComponent>())
		{
			TargetFaction = TargetDefComp->FactionID;
		}
		else if (FocusActor->IsA<APlayerController>() || FocusActor->Tags.Contains("Player"))
		{
			TargetFaction = FName("Player");
		}
		
		// Query FactionSubsystem for relationship
		if (!SelfFaction.IsNone() && !TargetFaction.IsNone())
		{
			if (UWorld* World = GetWorld())
			{
				if (UFactionSubsystem* FactionSys = World->GetSubsystem<UFactionSubsystem>())
				{
					float Reputation = FactionSys->GetBaseAttitude(SelfFaction, TargetFaction);
					
					// If reputation is low (hostile), trigger hijack
					if (Reputation <= 25.0f)
					{
						Interpolator->SetTargetValue(TEXT("Perceived_Threat"), 0.9f);
						AINPC_LOG(Warning, "[Cognition] 🧠⚡ AMYGDALA HIJACK! Hostile actor detected: %s (Faction: %s, Rep: %.0f)", 
							*FocusActor->GetName(), *TargetFaction.ToString(), Reputation);
						return true;
					}
				}
			}
		}
	}
	
	return false;
}

bool UCognitionComponent::IsDataReady(const FString& PersonalityID, const FString& FactionStr, const FString& SituationDescription)
{
	// A. Data Readiness Check
	bool bPersonalityReady = !(PersonalityID == "None" || PersonalityID == "Default" || PersonalityID.IsEmpty());
	
	// ✅ FIX: Allow "Neutral" faction. Only block "None" or empty.
	// Previously blocked "Neutral" which caused civilian NPCs (who are Neutral) to get stuck in retry loops.
	bool bFactionReady = !(FactionStr == "None" || FactionStr.IsEmpty()); 
	
	// Allow Combat NPCs to be Neutral if tagged
	bool bDataReady = bPersonalityReady && (bFactionReady || GetOwner()->ActorHasTag("AllowNeutral"));
	
	if (!bDataReady)
	{
		AINPC_LOG_WARNING("[Cognition] 🔄 Data not ready - PersonalityID: %s, Faction: %s. Scheduling retry...", *PersonalityID, *FactionStr);
		
		PendingStimulus = SituationDescription;
		if (!GetWorld()->GetTimerManager().IsTimerActive(RetryStimulusTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(RetryStimulusTimerHandle, [this]()
			{
				if (!PendingStimulus.IsEmpty())
				{
					FString CheckStimulus = PendingStimulus;
					PendingStimulus.Empty();
					ProcessStimulus(CheckStimulus);
				}
			}, 0.5f, false);
		}
		return false;
	}
	
	AINPC_LOG(Log, "[Cognition] ✅ Data ready - PersonalityID: %s, Faction: %s", *PersonalityID, *FactionStr);
	return true;
}

FString UCognitionComponent::BuildIdentityBlock(const FString& RoleDesc, const FString& PersonalityID, const FString& FactionStr, bool bFullDetail)
{
	// 1. Basic Role Info (always included)
	FString RoleSection = FString::Printf(TEXT(
		"You are: %s (Faction: %s)\n"
		"Role: %s\n"), *PersonalityID, *FactionStr, *RoleDesc);

	// 2. Behavioral Guidelines (always included - affects decision making)
	FString ActualBehavioralGuidelines = BehavioralGuidelines;
	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		if (AUtilityAIController* UtilityController = Cast<AUtilityAIController>(AIController))
		{
			if (UPersonalityComponent* PersonalityComp = UtilityController->PersonalityComp)
			{
				if (!PersonalityComp->Personality.BehavioralGuidelines.IsEmpty())
				{
					ActualBehavioralGuidelines = PersonalityComp->Personality.BehavioralGuidelines;
				}
			}
		}
	}
	if (!ActualBehavioralGuidelines.IsEmpty())
	{
		RoleSection += FString::Printf(TEXT("Rules: %s\n"), *ActualBehavioralGuidelines);
	}

	// 3. Zombie Override (always included - fundamental identity)
	if (PersonalityID.Contains(TEXT("Zombie")) || FactionStr.Contains(TEXT("Zombie")))
	{
		RoleSection += TEXT("\n[INSTINCTS] Driven purely by insatiable hunger for living flesh. No fear, no pain, no higher logic.\n[LIMITATION] Brain rot preventing complex speech.\n");
	}

	// --- Simplified Mode: Return early with minimal identity ---
	if (!bFullDetail)
	{
		// Still include name for dialogue
		FString CharName = "Unknown";
		if (AAIController* AICon = Cast<AAIController>(GetOwner()))
		{
			APawn* ControlledPawn = AICon->GetPawn();
			UNPCDefinitionComponent* DefComp = nullptr;
			if (ControlledPawn) DefComp = ControlledPawn->FindComponentByClass<UNPCDefinitionComponent>();
			if (!DefComp) DefComp = AICon->FindComponentByClass<UNPCDefinitionComponent>();
			if (DefComp)
			{
				FNPCNameDef NameDef;
				if (DefComp->GetNameDef(NameDef)) CharName = FString::Printf(TEXT("%s %s"), *NameDef.FirstName, *NameDef.Surname);
			}
		}
		RoleSection += FString::Printf(TEXT("Name: %s\n"), *CharName);
		return RoleSection;
	}

	// --- Full Detail Mode: Include complete backstory ---

	// 4. Faction Description
	FString FactionDescription = "";
	if (UNPCDefinitionComponent* DefComp = GetOwner()->FindComponentByClass<UNPCDefinitionComponent>())
	{
		if (UDataTable* FactionTable = DefComp->FactionTable)
		{
			FName FactionRowName = FName(*FactionStr);
			FFactionDef* FactionDef = FactionTable->FindRow<FFactionDef>(FactionRowName, TEXT("CognitionComponent_GetFactionDesc"));
			if (FactionDef) FactionDescription = FactionDef->Description;
		}
	}
	if (!FactionDescription.IsEmpty())
	{
		RoleSection += FString::Printf(TEXT("Faction Identity: %s\n"), *FactionDescription);
	}

	// 5. Full Backstory Assembly — prescriptive, not a fact sheet
	FString BackstorySection = "";
	if (AAIController* AICon = Cast<AAIController>(GetOwner()))
	{
		APawn* ControlledPawn = AICon->GetPawn();
		UNPCDefinitionComponent* DefComp = nullptr;
		if (ControlledPawn) DefComp = ControlledPawn->FindComponentByClass<UNPCDefinitionComponent>();
		if (!DefComp) DefComp = AICon->FindComponentByClass<UNPCDefinitionComponent>();

		if (DefComp)
		{
			 FString CharName = "Unknown";
			 FString OceanTraits = "Balanced";
			 FString HistoryDesc = "";
			 FString PhobiaStr = "";
			 FString MentalScar = "";
			 FString ValuesStr = "";
			 FString StatusStr = "";

			 FNPCNameDef NameDef;
			 if (DefComp->GetNameDef(NameDef)) CharName = FString::Printf(TEXT("%s %s"), *NameDef.FirstName, *NameDef.Surname);

             if (AUtilityAIController* UtilCon = Cast<AUtilityAIController>(AICon))
                if (UtilCon->PersonalityComp) OceanTraits = UtilCon->PersonalityComp->Personality.GetOCEANDescription();

             FPastEventDef EventDef;
             if (DefComp->GetPastEventDef(EventDef)) {
                 HistoryDesc = EventDef.EventDescription;
                 MentalScar = EventDef.MentalScar;
                 if (EventDef.ResultingPhobias.Num() > 0) PhobiaStr = FGameplayTagContainer::CreateFromArray(EventDef.ResultingPhobias).ToStringSimple();
             }

             FSocialProfileDef SocialProfile;
             if (DefComp->GetSocialProfileDef(SocialProfile)) {
                 if (SocialProfile.KeyValues.Num() > 0) ValuesStr = FString::Join(SocialProfile.KeyValues, TEXT(", "));
                 StatusStr = UEnum::GetValueAsString(SocialProfile.SocialStatus);
             }

			 // --- Core Identity ---
			 BackstorySection = FString::Printf(TEXT(
				 "\n[IDENTITY]\n"
				 "Name: %s\n"
				 "Personality Traits (OCEAN): %s\n"
				 "Social Class: %s\n"
			 ), *CharName, *OceanTraits, *StatusStr);

			 // --- Speech Triggers: trauma/phobia as behavioral directives ---
			 bool bHasTriggers = false;
			 FString TriggersBlock = TEXT("\n[SPEECH TRIGGERS] (involuntary reactions that override normal behavior)\n");

			 if (!MentalScar.IsEmpty())
			 {
				 TriggersBlock += FString::Printf(TEXT("- SCAR: \"%s\" — When the situation reminds you of this, your voice betrays fear/pain. Mention it or react to it.\n"), *MentalScar);
				 bHasTriggers = true;
			 }

			 if (!PhobiaStr.IsEmpty())
			 {
				 TriggersBlock += FString::Printf(TEXT("- PHOBIA: [%s] — When you encounter anything related, panic overrides logic. Raise Threat, Speech reflects irrational fear.\n"), *PhobiaStr);
				 bHasTriggers = true;
			 }

			 if (!HistoryDesc.IsEmpty())
			 {
				 TriggersBlock += FString::Printf(TEXT("- PAST: \"%s\" — This defines your worldview. Reference it when explaining your motives or judging others.\n"), *HistoryDesc);
				 bHasTriggers = true;
			 }

			 if (bHasTriggers)
			 {
				 BackstorySection += TriggersBlock;
			 }

			 // --- Values as speech motivators ---
			 if (!ValuesStr.IsEmpty())
			 {
				 BackstorySection += FString::Printf(TEXT("\n[CORE VALUES] %s — These are what you fight for, argue about, and judge others by. Work them into your Speech when taking a stance.\n"), *ValuesStr);
			 }
		}
	}

	return FString::Printf(TEXT("%s\n%s\n"), *RoleSection, *BackstorySection);
}

FString UCognitionComponent::BuildWorldviewBlock(const FString& FactionStr, const FString& SituationDescription)
{
	FString WorldviewSection = "";
	if (!FactionStr.IsEmpty() && !FactionStr.Equals("Neutral"))
	{
		if (UWorld* World = GetWorld())
		{
			if (UFactionSubsystem* FacSys = World->GetSubsystem<UFactionSubsystem>())
			{
				TMap<FName, float> Relations = FacSys->RuntimeFactionMatrix.FindRef(FName(*FactionStr));
				if (Relations.Num() > 0)
				{
					// Extract mentioned factions from stimulus text (format: "Faction: X")
					TSet<FString> MentionedFactions;
					int32 SearchIdx = 0;
					while (true)
					{
						int32 FoundIdx = SituationDescription.Find(TEXT("Faction: "), ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchIdx);
						if (FoundIdx == INDEX_NONE) break;

						int32 ValueStart = FoundIdx + 9; // len("Faction: ")
						int32 EndIdx = SituationDescription.Find(TEXT(","), ESearchCase::IgnoreCase, ESearchDir::FromStart, ValueStart);
						int32 ParenIdx = SituationDescription.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromStart, ValueStart);

						// Take whichever delimiter comes first
						if (EndIdx == INDEX_NONE || (ParenIdx != INDEX_NONE && ParenIdx < EndIdx))
							EndIdx = ParenIdx;

						if (EndIdx != INDEX_NONE && EndIdx > ValueStart)
						{
							FString FactionName = SituationDescription.Mid(ValueStart, EndIdx - ValueStart).TrimStartAndEnd();
							MentionedFactions.Add(FactionName);
						}
						SearchIdx = ValueStart + 1;
					}

					WorldviewSection = "\n[WORLDVIEW / FACTIONS]\n";
					WorldviewSection += FString::Printf(TEXT("%s: Your Faction (members are your allies)\n"), *FactionStr);

					for (const auto& Pair : Relations)
					{
						FString KeyStr = Pair.Key.ToString();
						if (KeyStr == FactionStr) continue;

						// Only include factions mentioned in the stimulus, OR all if none extracted (safety)
						if (MentionedFactions.Num() > 0 && !MentionedFactions.Contains(KeyStr))
							continue;

						FString RelDesc = FactionHelpers::GetAttitudeDescription(Pair.Value);
						WorldviewSection += FString::Printf(TEXT("%s: %s\n"), *KeyStr, *RelDesc);
					}
				}
			}
		}
	}
	return WorldviewSection;
}

FString UCognitionComponent::BuildContextBlock(const FString& ProfessionName, const FString& ProfessionDesc)
{
	FString ContextBlock = "";
    if (!ProfessionDesc.IsEmpty())
    {
        ContextBlock += FString::Printf(TEXT("Current Job (%s): %s\n"), *ProfessionName, *ProfessionDesc);
    }
    
    // Narrative Plot Context (Dynamic/Lazy Fetch)
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeSquadSubsystem* SquadSys = World->GetSubsystem<UNarrativeSquadSubsystem>())
		{
            FString NarrativePlotContext = SquadSys->GetMemberContext(GetOwner());
            if (NarrativePlotContext.IsEmpty()) {
                if (AAIController* AICon = Cast<AAIController>(GetOwner()))
                    if (APawn* Pawn = AICon->GetPawn())
                        NarrativePlotContext = SquadSys->GetMemberContext(Pawn);
            }

            if (!NarrativePlotContext.IsEmpty())
            {
                ContextBlock += FString::Printf(TEXT("\n[NARRATIVE PLOT]\n%s\n"), *NarrativePlotContext);
                AINPC_LOG(Verbose, "[Cognition] ✅ Injected Narrative Context");
            }
        }
    }
    return ContextBlock;
}

// ========================================
// Target Selection (Async LLM)
// ========================================

FString UCognitionComponent::SuggestTarget(const TArray<FString>& CandidateNames, const FString& SelectionContext)
{
	if (CandidateNames.Num() == 0) return TEXT("");
	if (!bEnableReasoning || !LLMService) return TEXT("");

	// 1. Check if we have a cached suggestion
	FString* CachedResult = CachedTargetSuggestions.Find(SelectionContext);
	
	// If pending, just return what we have (or empty if nothing yet)
	if (PendingTargetRequests.Contains(SelectionContext))
	{
		return CachedResult ? *CachedResult : TEXT("");
	}

	// 2. Refresh logic: trusted TargetSelectionSubsystem to only call when cache expires
	// Initiate new request
	PendingTargetRequests.Add(SelectionContext);

	// --- Build Prompts ---
	FString SystemPrompt = TEXT("You are a target selection AI. Return ONLY a name from the candidate list. No explanation.");

	FString UserPrompt = FString::Printf(
		TEXT("You are %s. You need to choose a target for: %s.\n\n"),
		*RoleDescription,
		*SelectionContext
	);

	// Inject Narrative Plot Context
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeSquadSubsystem* SquadSys = World->GetSubsystem<UNarrativeSquadSubsystem>())
		{
			FString PlotContext = SquadSys->GetMemberContext(GetOwner());
			if (PlotContext.IsEmpty())
			{
				if (AAIController* AICon = Cast<AAIController>(GetOwner()))
					if (APawn* Pawn = AICon->GetPawn())
						PlotContext = SquadSys->GetMemberContext(Pawn);
			}

			if (!PlotContext.IsEmpty())
			{
				UserPrompt += FString::Printf(TEXT("[CURRENT PLOT EVENT]\n%s\n\n"), *PlotContext);
			}
		}
	}

	// Add Memories (only high-importance)
	if (MemoryComp)
	{
		TArray<FMemoryItem> Memories = MemoryComp->RetrieveRelevantMemories("combat attack kill friend enemy interaction", 5);
		bool bHasMemories = false;
		for (const FMemoryItem& Memory : Memories)
		{
			if (Memory.ImportanceScore > 5.0f)
			{
				if (!bHasMemories) { UserPrompt += TEXT("Recent memories:\n"); bHasMemories = true; }
				UserPrompt += FString::Printf(TEXT("- %s\n"), *Memory.Description);
			}
		}
	}

	// Add Candidates
	UserPrompt += TEXT("\nCandidates:\n");
	for (const FString& Name : CandidateNames)
	{
		UserPrompt += FString::Printf(TEXT("- %s\n"), *Name);
	}

	// --- Send Functional Request (deterministic, capped) ---
	LLMService->SendFunctionalRequest(
		SystemPrompt,
		UserPrompt,
		FOnLLMResponseRaw::CreateLambda(
			[this, SelectionContext](bool bSuccess, const FString& Response)
			{
				this->OnTargetSuggestionReceived(bSuccess, Response, SelectionContext);
			}
		),
		0.1f,  // Near-deterministic
		false, // No JSON mode
		32     // Only need a name
	);

	return CachedResult ? *CachedResult : TEXT("");
}

void UCognitionComponent::OnTargetSuggestionReceived(bool bSuccess, const FString& Response, FString Context)
{
	PendingTargetRequests.Remove(Context);

	if (bSuccess && !Response.IsEmpty())
	{
		FString CleanResponse = Response.TrimStartAndEnd();
		// Remove punctuation if any
		CleanResponse = CleanResponse.Replace(TEXT("."), TEXT(""));
		
		CachedTargetSuggestions.Add(Context, CleanResponse);
		
		TARGET_LOG(Warning, "[%s] LLM Suggests Target for '%s': '%s'",
			*GetOwner()->GetName(), *Context, *CleanResponse);
	}
	else
	{
		TARGET_LOG(Warning, "[%s] LLM Target Selection FAILED for '%s'",
			*GetOwner()->GetName(), *Context);
	}
}

FString UCognitionComponent::BuildVolatileBlock(const FString& Situation, const FString& Memories, const FString& GlobalHistory)
{
    // Global History (Timestamps)
    FString Block = "";
    if (!GlobalHistory.IsEmpty())
    {
        Block += GlobalHistory;
    }
    
    Block += FString::Printf(TEXT(
		"Situation: %s %s\n"
		"Memories: %s\n"
	), *Situation, *CurrentDecisionContext, *Memories);
    
    return Block;
}