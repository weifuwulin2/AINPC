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

	// 5. Build Prompt Blocks...
	FString IdentityBlock = BuildIdentityBlock(RoleDesc, PersonalityIDStr, FactionStr);
	FString WorldviewBlock = BuildWorldviewBlock(FactionStr);
	
	FString ProfessionName = "", ProfessionDesc = "";
	if (AAIController* AICon = Cast<AAIController>(GetOwner()))
		if (UGoalComponent* Goal = AICon->FindComponentByClass<UGoalComponent>()) {
			ProfessionName = Goal->ProfessionConfig.ProfessionName.ToString();
			ProfessionDesc = Goal->ProfessionConfig.Description;
		}
	FString ContextBlock = BuildContextBlock(ProfessionName, ProfessionDesc);
	
	FString GlobalHistory = "";
	if (UWorld* World = GetWorld())
		if (UNarrativeDirectorSubsystem* Dir = World->GetSubsystem<UNarrativeDirectorSubsystem>())
			GlobalHistory = Dir->GetWorldStateDescription(3);
			
	// Pass empty memories for now if checking logic
	FString ContextMemory = "None"; 
    // Wait, need to fix Memory Logic above or just grab it?
    // Re-inserting Memory Logic here to be safe and complete...
	if (MemoryComp) {
		float CurrentGameTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		TArray<FMemoryItem> Memories = MemoryComp->RetrieveRelevantMemories(SituationDescription, 5);
        ContextMemory = "";
		for (const FMemoryItem& Item : Memories) ContextMemory += FString::Printf(TEXT("- %s\n"), *MemoryComp->GetFormattedDescription(Item, CurrentGameTime));
	}
    
	FString VolatileBlock = BuildVolatileBlock(SituationDescription, ContextMemory, GlobalHistory);

	// 6. Assemble Final Prompt
		// 0. Static Instructions (Rules) - MOVED TO TOP FOR CACHING
	// If this is at the bottom, the Volatile Stimuli block (which changes every frame)
	// would break the prefix cache for these 500+ tokens.
	FString InstructionsBlock = TEXT(
		"IMPORTANT Instructions:\n"
		"1. [DEFICIT MODEL] 'Boredom' and 'Loneliness' reflect unmet needs (GROW over time). 'Indignity' and 'Threat' are reactions (DECAY over time).\n"
		"2. [STRATEGY] You MUST output an 'Intention' that overrides your fear if necessary, OR respects it.\n"
		"3. [COWARDICE RULE] If you are a COWARD and Threat is 'Strong'/'Extreme', Intention MUST be 'Flee' or 'Beg', unless you are cornered.\n"
		"4. [JURISDICTION] Do NOT output Hunger/Fatigue (Engine manages them).\n"
		"5. [EMOTION STRICT] 'Emotion' MUST be EXACTLY one of: Neutral, Angry, Scared, Sad, Happy, Curious, Disgust. ABSOLUTELY NO OTHER VALUES (e.g., 'Suspicious', 'Confused', 'Anxious'). If you feel 'suspicious', use 'Curious'. If uncertain, use 'Neutral'.\n"
		"6. [MEMORY TIME] React ONLY to [JUST NOW] memories. [X MIN AGO] and [HISTORY] are context only. [RESOLVED] events are already handled - do NOT react again.\n"
		"\n"
		"  Speech: string;      // approx 10 words, match personality\n"
		"}\n"
	);

	FString Prompt = FString::Printf(TEXT(
		"%s\n" // 0. Instructions (Static)
		"%s"   // 1. Identity (Static)
		"%s\n" // 2. Worldview (Semi-Static)
		"%s\n" // 3. Context (Semi-Static)
		"%s\n" // 4. Volatile (Timestamps/Stimuli)
	), *InstructionsBlock, *IdentityBlock, *WorldviewBlock, *ContextBlock, *VolatileBlock);

	// 7. Rate Limiting & Send
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bool bIsHighPriority = SituationDescription.Contains(TEXT("HOSTILE")) || SituationDescription.Contains(TEXT("DANGER"));
	float Cooldown = bIsHighPriority ? 1.5f : 4.0f;
	
	if (CurrentTime - LastLLMRequestTime < Cooldown) {
		 UE_LOG(LogTemp, Error, TEXT("[Cognition] ❌ Exiting: Rate Limited. Time: %.2f, Last: %.2f, Cd: %.2f"), CurrentTime, LastLLMRequestTime, Cooldown);
		 return; 
	}
	LastLLMRequestTime = CurrentTime;
	
	AINPC_LOG(Warning, TEXT("━━━━━━━━━━ [Cognition] 📤 LLM REQUEST ━━━━━━━━━━"));
	AINPC_LOG(Warning, TEXT("%s"), *Prompt);
	AINPC_LOG(Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
	
	LLMService->SendRequest(Prompt, FOnLLMResponse::CreateUObject(this, &UCognitionComponent::OnLLMReply));
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

	// 1. 获取流水账
	FString DailyLogs = MemoryComp->GetAllRecentMemoriesAsString();
	if (DailyLogs.IsEmpty()) return;

	// 2. 构造 Prompt (强制让 LLM 输出 JSON 数组，方便代码解析)
	FString Prompt = FString::Printf(TEXT(
		"Here are my recent memories:\n%s\n"
		"Task: Summarize these events into 3 concise, high-level insights about the world or the player.\n"
		"Output Format: A pure JSON array of strings. Example: [\"Player is hostile\", \"Food is scarce\"]\n"
		"Do NOT output markdown."
	), *DailyLogs);

	AINPC_LOG(Log, "[Dreaming] Sending logs to LLM...");

	// 3. 发送请求 (注意：这里我们复用 LLMService，但需要 LLMCommunicator 支持返回原始 String 的回调)
	// 假设你的 LLMCommunicator 有一个 SendRequestRaw 或者你重载了回调，这里展示逻辑核心
	LLMService->SendRequestRaw(Prompt, FOnLLMResponseRaw::CreateUObject(this, &UCognitionComponent::OnDreamingAnalysisComplete));
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

FString UCognitionComponent::BuildIdentityBlock(const FString& RoleDesc, const FString& PersonalityID, const FString& FactionStr)
{
	// 1. Basic Role Info
	FString RoleSection = FString::Printf(TEXT(
		"You are: %s (Faction: %s)\n"
		"Role: %s\n"), *PersonalityID, *FactionStr, *RoleDesc);

	// 2. Behavioral Guidelines (Static)
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

	// 3. Faction Description (Static)
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

	// 4. Zombie Override (Static)
	if (PersonalityID.Contains(TEXT("Zombie")) || FactionStr.Contains(TEXT("Zombie")))
	{
		RoleSection += TEXT("\n[INSTINCTS] Driven purely by insatiable hunger for living flesh. No fear, no pain, no higher logic.\n[LIMITATION] Brain rot preventing complex speech.\n");
	}

	// 5. Backstory Assembly
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
			 FString HistoryDesc = "None";
			 FString PhobiaStr = "None";
			 FString MentalScar = "";
			 FString ValuesStr = "None";
			 FString StatusStr = "Unknown";

			 // Fetch details...
			 FNPCNameDef NameDef;
			 if (DefComp->GetNameDef(NameDef)) CharName = FString::Printf(TEXT("%s %s"), *NameDef.FirstName, *NameDef.Surname);

             // --- Quick Re-implementation of Backstory Logic ---
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

			 BackstorySection = FString::Printf(TEXT(
				 "\n[IDENTITY]\n"
				 "Name: %s\n"
				 "Personality Traits (OCEAN): %s\n"
				 "Past Event: %s\n"
				 "Mental Scar: %s\n"
				 "Phobias/Traumas: %s\n"
				 "Core Values: %s\n"
				 "Social Class: %s\n"
			 ), *CharName, *OceanTraits, *HistoryDesc, *MentalScar, *PhobiaStr, *ValuesStr, *StatusStr);
		}
	}

	return FString::Printf(TEXT("%s\n%s\n"), *RoleSection, *BackstorySection);
}

FString UCognitionComponent::BuildWorldviewBlock(const FString& FactionStr)
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
					WorldviewSection = "\n[WORLDVIEW / FACTIONS]\n";
					// List own faction first so LLM knows who its allies are
					WorldviewSection += FString::Printf(TEXT("%s: Your Faction (members are your allies)\n"), *FactionStr);
					for (const auto& Pair : Relations)
					{
						if (Pair.Key.ToString() == FactionStr) continue; // Already listed above
						FString RelDesc = FactionHelpers::GetAttitudeDescription(Pair.Value);

						WorldviewSection += FString::Printf(TEXT("%s: %s\n"),
							*Pair.Key.ToString(), *RelDesc);
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

	// --- Build Prompt ---
	FString Prompt = FString::Printf(
		TEXT("You are %s. You need to choose a target for: %s.\n\n"),
		*RoleDescription,
		*SelectionContext
	);

	// ✅ Inject Narrative Plot Context (CRITICAL for plot-driven target selection)
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
				Prompt += FString::Printf(TEXT("[CURRENT PLOT EVENT]\n%s\n\n"), *PlotContext);
			}
		}
	}

	// Add Memories
	if (MemoryComp)
	{
		Prompt += TEXT("Your recent memories:\n");
		// Use a broad query for memories related to interaction
		TArray<FMemoryItem> Memories = MemoryComp->RetrieveRelevantMemories("combat attack kill friend enemy interaction", 5);
		for (const FMemoryItem& Memory : Memories)
		{
			if (Memory.ImportanceScore > 5.0f)
			{
				Prompt += FString::Printf(TEXT("- %s (Importance: %.1f)\n"), *Memory.Description, Memory.ImportanceScore);
			}
		}
	}

	// Add Candidates
	Prompt += TEXT("\nAvailable candidates:\n");
	for (const FString& Name : CandidateNames)
	{
		Prompt += FString::Printf(TEXT("- %s\n"), *Name);
	}

	Prompt += FString::Printf(
		TEXT("\nWho should you choose? Reply with ONLY the name from the candidate list. No explanation.")
	);

	// --- Send Request ---
	LLMService->SendRequestRaw(Prompt, FOnLLMResponseRaw::CreateLambda(
		[this, SelectionContext](bool bSuccess, const FString& Response)
		{
			this->OnTargetSuggestionReceived(bSuccess, Response, SelectionContext);
		}
	));

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
		
		UE_LOG(LogAINPCBrain, Log, TEXT("[%s] LLM Suggests Target for %s: %s"), 
			*GetOwner()->GetName(), *Context, *CleanResponse);
	}
	else
	{
		UE_LOG(LogAINPCBrain, Warning, TEXT("[%s] LLM Target Selection Failed for %s"), 
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