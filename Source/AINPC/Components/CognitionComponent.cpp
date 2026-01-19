#include "CognitionComponent.h"

#include "UtilityAIComponent.h"
#include "Controller/UtilityAIController.h"
#include "Components/PersonalityComponent.h"
#include "Components/GoalComponent.h"
#include "Social/ProfessionTypes.h"
#include "LLM/LLMCommunicator.h"
#include "Components/NPCDefinitionComponent.h"
#include "UtilityAI/SentimentMapping.h"
#include "UtilityAI/MentalStateInterpolation.h"

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
			UE_LOG(LogTemp, Warning, TEXT("[Cognition] Warning: No MemoryComponent found on Owner! Memories will not be stored/retrieved."));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[Cognition] Connected to MemoryComponent."));
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
		UE_LOG(LogTemp, Error, TEXT("[Cognition] FATAL: API Key not found in DefaultGame.ini!"));
		return;
	}

	if (ConfigApiUrl.IsEmpty())
	{
		ConfigApiUrl = TEXT("https://api.deepseek.com/chat/completions");
	}

	// 3. 初始化 Service
	LLMService->Init(ConfigApiKey, ConfigApiUrl);
    
	UE_LOG(LogTemp, Log, TEXT("[Cognition] Brain Initialized via Config."));

	// 4. 初始化语义映射器 / Initialize Sentiment Mapper
	SentimentMapper = NewObject<USentimentMapper>(this);
	SentimentMapper->Initialize();  // 使用默认映射
	UE_LOG(LogTemp, Log, TEXT("[Cognition] SentimentMapper initialized"));

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
	
	UE_LOG(LogTemp, Log, TEXT("[Cognition] Interpolator initialized with custom speeds"));
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
		// UE_LOG(LogTemp, Log, TEXT("[Cognition] 🧠 Switched to %s"), *UEnum::GetValueAsString(CurrentLOD));
	}
}

void UCognitionComponent::ProcessStimulus(FString SituationDescription)
{
	// However, ProcessStimulus is called by Controller::RelaySensoryToCognition.
	// Wait, RelaySensoryToCognition is legacy?
	// The new path is: Event -> Sensory -> Memory.
	// The new path for "Thinking" is: ... when does thinking happen?
	// "Phase 2: Social Layer - 运作流程：触发：当生理层判断事件重要且非战斗时"
	// So MemoryComponent or Controller should trigger Cognition.
	
	// Let's assume ProcessStimulus is now the entry point for "Thinking about a situation".
	// So we DO need to retrieve memories here.

	// 2. 检索检索 (Retrieval)
	FString ContextMemory = "";
	if (MemoryComp)
	{
		TArray<FMemoryItem> Memories = MemoryComp->RetrieveRelevantMemories(SituationDescription, 5);
		for (const FMemoryItem& Item : Memories)
		{
			ContextMemory += FString::Printf(TEXT("- [%s] %s (Imp: %.1f)\n"), 
				*Item.Timestamp.ToString(), *Item.Description, Item.ImportanceScore);
		}
	}
	
	// If no memory found
	if (ContextMemory.IsEmpty())
	{
		ContextMemory = "No relevant memories.";
	}

	// ✅ 从 PersonalityComponent 获取角色信息 / Get role info from PersonalityComponent
	FString ActualRoleDescription = RoleDescription; // Default fallback
	FString ActualBehavioralGuidelines = BehavioralGuidelines;
	FString PersonalityIDStr = TEXT("Unknown");
	FString FactionStr = TEXT("Neutral");
	
	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		if (AUtilityAIController* UtilityController = Cast<AUtilityAIController>(AIController))
		{
			if (UPersonalityComponent* PersonalityComp = UtilityController->PersonalityComp)
			{
				// 使用 PersonalityComponent 中的配置
				// Use configuration from PersonalityComponent
				if (!PersonalityComp->Personality.RoleDescription.IsEmpty())
				{
					ActualRoleDescription = PersonalityComp->Personality.RoleDescription;
				}
				if (!PersonalityComp->Personality.BehavioralGuidelines.IsEmpty())
				{
					ActualBehavioralGuidelines = PersonalityComp->Personality.BehavioralGuidelines;
				}
				
				// 获取 PersonalityID 和 Faction
				// Get PersonalityID and Faction
				PersonalityIDStr = PersonalityComp->PersonalityID.ToString();
				FactionStr = UEnum::GetValueAsName(PersonalityComp->Personality.Faction).ToString();
				
				UE_LOG(LogTemp, Log, TEXT("[Cognition] Using personality from PersonalityComponent: ID=%s, Faction=%s"), 
					*PersonalityIDStr, *FactionStr);
			}
		}
	}
	
    // ✅ Retrieve Profession Description (Job) from GoalComponent
    FString ProfessionDescription = "";
    FString ProfessionNameStr = "";
    
    // Check Controller first
    if (AAIController* AICon = Cast<AAIController>(GetOwner()))
    {
        if (UGoalComponent* GoalComp = AICon->FindComponentByClass<UGoalComponent>())
        {
             if (!GoalComp->ProfessionConfig.Description.IsEmpty())
             {
                 ProfessionDescription = GoalComp->ProfessionConfig.Description;
                 ProfessionNameStr = GoalComp->ProfessionConfig.ProfessionName.ToString();
             }
        }
    }
	
	// ---------------------------------------------------------
	// ✅ Retrieve Backstory (History, Trauma, Social Status)
	// ---------------------------------------------------------
	FString BackstorySection = "";
	if (AAIController* AICon = Cast<AAIController>(GetOwner()))
	{
		// Definition Component acts as the "Passport", usually on the Pawn
		APawn* ControlledPawn = AICon->GetPawn();
		UNPCDefinitionComponent* DefComp = nullptr;
		
		if (ControlledPawn) DefComp = ControlledPawn->FindComponentByClass<UNPCDefinitionComponent>();
		if (!DefComp) DefComp = AICon->FindComponentByClass<UNPCDefinitionComponent>(); // Fallback to Controller

		if (DefComp)
		{
			 // --- 1. Modular Data Retrieval ---
			 FString CharName = "Unknown";
			 FString OceanTraits = "Balanced";
			 FString HistoryDesc = "None";
			 FString PhobiaStr = "None";
			 FString MentalScar = "";
			 
			 // A. OCEAN (from Personality Component)
			 if (AUtilityAIController* UtilCon = Cast<AUtilityAIController>(AICon))
			 {
				 if (UtilCon->PersonalityComp)
				 {
					 OceanTraits = UtilCon->PersonalityComp->Personality.GetOCEANDescription();
				 }
			 }

			 // B. Name (Modular)
			 FNPCNameDef NameDef;
			 if (DefComp->GetNameDef(NameDef))
			 {
				 CharName = FString::Printf(TEXT("%s %s"), *NameDef.FirstName, *NameDef.Surname);
			 }

			 // C. Past Event (Modular)
			 FPastEventDef EventDef;
			 if (DefComp->GetPastEventDef(EventDef))
			 {
				 HistoryDesc = EventDef.EventDescription;
				 MentalScar = EventDef.MentalScar;
				 if (EventDef.ResultingPhobias.Num() > 0)
				 {
					 PhobiaStr = FGameplayTagContainer::CreateFromArray(EventDef.ResultingPhobias).ToStringSimple();
				 }
			 }

			 // D. Social Profile (Status, Values)
			 FSocialProfileDef SocialProfile;
			 bool bHasProfile = DefComp->GetSocialProfileDef(SocialProfile);
			 
			 FString ValuesStr = "None";
			 FString StatusStr = "Unknown";

			 if (bHasProfile)
			 {
				 if (SocialProfile.KeyValues.Num() > 0) ValuesStr = FString::Join(SocialProfile.KeyValues, TEXT(", "));
				 StatusStr = UEnum::GetValueAsString(SocialProfile.SocialStatus);
				 int32 ScopeIndex = StatusStr.Find(TEXT("::"));
				 if (ScopeIndex != INDEX_NONE) StatusStr = StatusStr.RightChop(ScopeIndex + 2);
			 }

			 // --- 2. Assemble Section ---
			 BackstorySection = FString::Printf(TEXT(
				 "\n[IDENTITY]\n"
				 "Name: %s\n"
				 "Personality Traits (OCEAN): %s\n"
				 "Past Event: %s\n"
				 "Mental Scar: %s\n"
				 "Phobias/Traumas: %s\n"
				 "Core Values: %s\n"
				 "Social Class: %s\n"
			 ), 
			 *CharName,
			 *OceanTraits,
			 *HistoryDesc,
			 *MentalScar,
			 *PhobiaStr,
			 *ValuesStr,
			 *StatusStr
			 );
		}
	}
	
	// ⚠️ 关键修复：如果 Personality 还没初始化（ID为None），不要发送请求，避免 LLM 产生幻觉
	// Critical Fix: If Personality is not initialized (ID is None), do not send request to avoid LLM hallucinations
	if (PersonalityIDStr == "None" || PersonalityIDStr == "Default" || PersonalityIDStr.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Cognition] PersonalityID not set yet. Scheduling retry in 0.5s..."));
		
		PendingStimulus = SituationDescription;
		
		// 避免重复设置 Timer
		if (!GetWorld()->GetTimerManager().IsTimerActive(RetryStimulusTimerHandle))
		{
			UE_LOG(LogTemp, Log, TEXT("[Cognition] Timer started. Will retry in 0.5s."));
			GetWorld()->GetTimerManager().SetTimer(RetryStimulusTimerHandle, [this]()
			{
				if (!PendingStimulus.IsEmpty())
				{
					UE_LOG(LogTemp, Log, TEXT("[Cognition] Retrying pending stimulus..."));
					
					// 先复制并清空，防止 ProcessStimulus 再次 Pending 后被这里误删
					// Copy and clear first to prevent accidental deletion if ProcessStimulus pends again
					FString CheckStimulus = PendingStimulus;
					PendingStimulus.Empty();
					
					ProcessStimulus(CheckStimulus);
				}
			}, 0.5f, false);
		}
		
		return;
	}
	
	// 构建角色部分 / Build role section
	// ⚠️ 关键：明确告诉 LLM 这是什么类型的 NPC
	FString RoleSection = FString::Printf(TEXT(
		"You are: %s (Faction: %s)\n"
		"Role: %s\n"), *PersonalityIDStr, *FactionStr, *ActualRoleDescription);

    // Append Profession / Job Description
    if (!ProfessionDescription.IsEmpty())
    {
        RoleSection += FString::Printf(TEXT("Job (%s): %s\n"), *ProfessionNameStr, *ProfessionDescription);
    }
	
	// 如果有行为准则，添加到角色部分 / If there are behavioral guidelines, add to role section
	if (!ActualBehavioralGuidelines.IsEmpty())
	{
		RoleSection += FString::Printf(TEXT("Rules: %s\n"), *ActualBehavioralGuidelines);
	}
	
	// ⚠️ 根据 PersonalityID 添加性格驱动而非硬性规则
	// Add personality drives instead of hard rules to allow LLM emergence
	if (PersonalityIDStr.Contains(TEXT("Zombie")) || FactionStr.Contains(TEXT("Monster")))
	{
		// 僵尸：生理限制和原始驱动 / Physiological limits and primal drives
		RoleSection += TEXT("\n[INSTINCTS] Driven purely by insatiable hunger for living flesh. No fear, no pain, no higher logic.\n[LIMITATION] Brain rot preventing complex speech (can only grunt/hiss/say single broken words).\n");
	}
	/* 
	 * Removed hardcoded "Warrior" and "Merchant" logic.
	 * Role rules should come from Personality.BehavioralGuidelines or RoleDescription.
	 * Only "Zombie/Monster" hardcoding remains for physiological overrides.
	 */

	// --- PROMPT LOD OPTIMIZATION ---
	// If in Critical LOD (Survival/Combat), we strip away flavor text to focus LLM on survival.
	bool bFullContext = (CurrentLOD == EContextLOD::Standard || CurrentLOD == EContextLOD::Deep);
	
	// FString FinalRoleSection = bFullContext ? RoleSection : TEXT("You are in extreme danger. Focus ONLY on survival.");
	FString FinalRoleSection = RoleSection; // Keep role/instincts even in danger (e.g. Zombie hunger)
	FString FinalBackstorySection = bFullContext ? BackstorySection : TEXT(""); // No backstory in combat
	FString FinalSituation = SituationDescription;
	FString FinalMemories = bFullContext ? ContextMemory : TEXT(""); // No memories in combat unless critical? (Maybe keep basic)

	// 构造精简的 Prompt / Build concise prompt
	FString Prompt = FString::Printf(TEXT(
		"%s"
		"%s\n"
		"Situation: %s %s\n"
		"Memories: %s\n\n"
		"IMPORTANT Instructions:\n"
		"1. [DEFICIT MODEL] 'Boredom' and 'Loneliness' reflect unmet needs (GROW over time). 'Indignity' and 'Threat' are reactions (DECAY over time).\n"
		"2. [STRATEGY] You MUST output an 'Intention' that overrides your fear if necessary, OR respects it.\n"
		"3. [COWARDICE RULE] If you are a COWARD and Threat is 'Strong'/'Extreme', Intention MUST be 'Flee' or 'Beg', unless you are cornered.\n"
		"4. [JURISDICTION] Do NOT output Hunger/Fatigue (Engine manages them).\n"
		"5. [EMOTION STRICT] 'Emotion' MUST be EXACTLY one of: Neutral, Angry, Scared, Sad, Happy, Curious, Disgust. ABSOLUTELY NO OTHER VALUES (e.g., 'Suspicious', 'Confused', 'Anxious'). If you feel 'suspicious', use 'Curious'. If uncertain, use 'Neutral'.\n"
		"\n"
		"  Speech: string;      // approx 10 words, match personality\n"
		"}\n"
	), *FinalRoleSection, *FinalBackstorySection, *FinalSituation, *CurrentDecisionContext, *FinalMemories);
	
	UE_LOG(LogTemp, Log, TEXT("[Cognition] Sending to LLM..."));
	
	// 发送请求，并绑定内部回调 OnLLMReply
	LLMService->SendRequest(
		Prompt,
		FOnLLMResponse::CreateUObject(this, &UCognitionComponent::OnLLMReply)
	);
}

void UCognitionComponent::OnLLMReply(bool bSuccess, const FMentalState& NewState)
{
	if (bSuccess)
	{
		// 不要直接广播，而是使用 Interpolator 设置目标值
		// Don't broadcast directly, use Interpolator to set target values
		
		if (Interpolator)
		{
			// 使用宏自动为所有字段设置目标值
			// Use macro to automatically set target values for all fields
			#define SET_TARGET_VALUE(Name, DefaultValue, DisplayName, Description) \
				Interpolator->SetTargetValue(TEXT(#Name), NewState.Name);
			
			MENTAL_STATE_FIELDS(SET_TARGET_VALUE)
			
			#undef SET_TARGET_VALUE
			
			UE_LOG(LogTemp, Log, TEXT("[Cognition] Target values set from LLM"));
			UE_LOG(LogTemp, Log, TEXT("  Indignity target: %.2f, Boredom target: %.2f"), 
			       NewState.Indignity, NewState.Boredom);
			
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
			UE_LOG(LogTemp, Warning, TEXT("[Cognition] Interpolator not initialized! Broadcasting directly."));
			OnMentalStateChanged.Broadcast(NewState);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Cognition] Failed to process thought."));
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

	UE_LOG(LogTemp, Log, TEXT("[Dreaming] Sending logs to LLM..."));

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