#include "Subsystems/WorldDirectorSubsystem.h"
#include "AINPC.h"
#include "Subsystems/NarrativeHistorySubsystem.h"
#include "Subsystems/NarrativeSquadSubsystem.h"
#include "Social/FactionSubsystem.h"
#include "Subsystems/PlayerSquadSubsystem.h"
#include "Subsystems/TimeManager.h"
#include "LLM/LLMCommunicator.h"
#include "Characters/NarrativeCompanion.h"
#include "Utilities/NPCQueryHelpers.h"
#include "Utilities/FactionHelpers.h"
#include "World/NarrativeSceneAnchor.h"
#include "Social/SocialGameplayTags.h"
#include "Engine/DataTable.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// ============================================================================
// Lifecycle
// ============================================================================

void UWorldDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Ensure dependencies initialize first
	Collection.InitializeDependency<UNarrativeHistorySubsystem>();
	Collection.InitializeDependency<UNarrativeSquadSubsystem>();
	Collection.InitializeDependency<UFactionSubsystem>();
	Collection.InitializeDependency<UPlayerSquadSubsystem>();

	UWorld* World = GetWorld();
	if (!World) return;

	// Cache subsystem pointers
	NarrativeHistory = World->GetSubsystem<UNarrativeHistorySubsystem>();
	SquadSubsystem = World->GetSubsystem<UNarrativeSquadSubsystem>();
	FactionSub = World->GetSubsystem<UFactionSubsystem>();
	PlayerSquad = World->GetSubsystem<UPlayerSquadSubsystem>();

	// TimeManager is a GameInstanceSubsystem
	if (UGameInstance* GI = World->GetGameInstance())
	{
		TimeMgr = GI->GetSubsystem<UTimeManager>();
	}

	// Initialize LLM Service
	LLMService = NewObject<ULLMCommunicator>(this);
	FString ConfigApiKey;
	FString ConfigApiUrl;
	GConfig->GetString(TEXT("LLM.Settings"), TEXT("ApiKey"), ConfigApiKey, GGameIni);
	GConfig->GetString(TEXT("LLM.Settings"), TEXT("ApiUrl"), ConfigApiUrl, GGameIni);

	if (ConfigApiKey.IsEmpty())
	{
		DIRECTOR_LOG(Warning, TEXT("LLM ApiKey not found in DefaultGame.ini. LLM scene generation disabled."));
	}
	else
	{
		if (ConfigApiUrl.IsEmpty())
		{
			ConfigApiUrl = TEXT("https://api.deepseek.com/chat/completions");
		}
		LLMService->Init(ConfigApiKey, ConfigApiUrl);
		DIRECTOR_LOG(Log, TEXT("LLM Service initialized for WorldDirector."));
	}

	// Bind to NarrativeHistory events (for tracking scene completion)
	if (NarrativeHistory)
	{
		NarrativeHistory->OnEventRecorded.AddDynamic(this, &UWorldDirectorSubsystem::OnDirectorEventRecorded);
	}

	// Bind to day change for world prediction
	if (TimeMgr)
	{
		TimeMgr->OnDayChanged.AddDynamic(this, &UWorldDirectorSubsystem::OnDayChanged);
	}

	DIRECTOR_LOG(Log, TEXT("WorldDirectorSubsystem initialized."));
}

void UWorldDirectorSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvaluationTimerHandle);
		World->GetTimerManager().ClearTimer(DuringHintTimerHandle);

		if (NarrativeHistory)
		{
			NarrativeHistory->OnEventRecorded.RemoveDynamic(this, &UWorldDirectorSubsystem::OnDirectorEventRecorded);
		}

		if (TimeMgr)
		{
			TimeMgr->OnDayChanged.RemoveDynamic(this, &UWorldDirectorSubsystem::OnDayChanged);
		}
	}

	Super::Deinitialize();
}

void UWorldDirectorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Start the periodic evaluation timer
	InWorld.GetTimerManager().SetTimer(
		EvaluationTimerHandle,
		this,
		&UWorldDirectorSubsystem::PeriodicEvaluation,
		EvaluationIntervalSeconds,
		true, // Loop
		EvaluationIntervalSeconds // Initial delay (skip first frame)
	);

	DIRECTOR_LOG(Log, TEXT("WorldDirector evaluation timer started (interval: %.0fs)."), EvaluationIntervalSeconds);
}

// ============================================================================
// Core Pipeline
// ============================================================================

void UWorldDirectorSubsystem::PeriodicEvaluation()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Always tick major events (not gated by cooldown)
	CheckMajorEvents();
	TickActiveEvents();

	float GameTime = World->GetTimeSeconds();

	// Step 1: Cooldown check
	if ((GameTime - LastBeatGameTime) < MinBeatCooldownSeconds)
	{
		DIRECTOR_LOG(Verbose, TEXT("Cooldown active (%.0fs remaining). Skipping."),
			MinBeatCooldownSeconds - (GameTime - LastBeatGameTime));
		return;
	}

	// Step 2: Capacity check
	if (ActiveDirectorScenes >= MaxConcurrentScenes)
	{
		DIRECTOR_LOG(Verbose, TEXT("At max concurrent scenes (%d). Skipping."), ActiveDirectorScenes);
		return;
	}

	// Step 3: Check world timeline (takes priority)
	EDramaticAction TimelineAction;
	FString TimelineContext;
	bool bTimelineEvent = CheckWorldTimeline(TimelineAction, TimelineContext);

	// Step 4: Evaluate tension
	LatestTension = EvaluateTension();
	DIRECTOR_LOG(Log, TEXT("Tension: Death=%.2f Faction=%.2f Player=%.2f Pop=%.2f Timeline=%.2f Mono=%.2f => Overall=%.2f"),
		LatestTension.DeathPressure, LatestTension.FactionVolatility,
		LatestTension.PlayerActivity, LatestTension.PopulationStress,
		LatestTension.TimelinePressure, LatestTension.Monotony,
		LatestTension.OverallTension);

	// Step 5: Select action
	EDramaticAction ChosenAction;
	FString Context;

	if (bTimelineEvent)
	{
		ChosenAction = TimelineAction;
		Context = TimelineContext;
		DIRECTOR_LOG(Log, TEXT("Timeline event override: Action=%d, Context=%s"),
			(int32)ChosenAction, *Context);
	}
	else
	{
		ChosenAction = SelectDramaticAction(LatestTension);
		Context = FString();
	}

	// Step 6: Request LLM scene generation
	RequestLLMSceneGeneration(ChosenAction, Context);
}

void UWorldDirectorSubsystem::ForceEvaluation()
{
	// Reset cooldown to allow immediate evaluation
	LastBeatGameTime = -999.0f;
	PeriodicEvaluation();
}

// ============================================================================
// World Timeline
// ============================================================================

bool UWorldDirectorSubsystem::CheckWorldTimeline(EDramaticAction& OutAction, FString& OutContext)
{
	if (!TimeMgr) return false;

	int32 Day = TimeMgr->GetCurrentDay();
	float Hour = TimeMgr->GetCurrentHour();

	// Helper lambda to check and fire a timeline event
	auto TryFireEvent = [&](FWorldTimelineEvent* Event, bool bFromDataTable) -> bool
	{
		if (!Event || Event->bHasFired) return false;

		if (Event->TriggerDay <= Day)
		{
			if (Event->TriggerDay < Day || Hour >= Event->TriggerHour)
			{
				Event->bHasFired = true;
				OutAction = Event->ForcedAction;
				OutContext = FString::Printf(TEXT("[TIMELINE EVENT: %s] %s"), *Event->EventName, *Event->EventDescription);

				FString Source = bFromDataTable ? TEXT("DataTable") : TEXT("LLM-Predicted");
				DIRECTOR_LOG(Warning, TEXT("Timeline event fired (%s): '%s' (Day %d, Hour %.1f)"),
					*Source, *Event->EventName, Event->TriggerDay, Event->TriggerHour);

				// If it has a pre-authored scene template, use that directly
				if (bFromDataTable && !Event->SceneTemplateID.IsNone() && SceneTemplateTable && SquadSubsystem)
				{
					int32 SquadID = SquadSubsystem->StartSceneGlobal(SceneTemplateTable, Event->SceneTemplateID, NPCDefinitionTable);
					if (SquadID != -1)
					{
						ActiveDirectorSquadIDs.Add(SquadID);
						ActiveDirectorScenes++;
						LastBeatGameTime = GetWorld()->GetTimeSeconds();

						if (NarrativeHistory)
						{
							FGameplayTagContainer Tags;
							Tags.AddTag(AINPCTags::WorldDirector_Event);
							Tags.AddTag(GetTagForAction(OutAction));
							NarrativeHistory->RecordEvent(
								FString::Printf(TEXT("[WorldDirector] %s"), *Event->EventName), Tags);
						}

						OnDramaticBeat.Broadcast(OutAction, Event->EventDescription);
						return false; // Scene handled directly, no need for LLM
					}
				}

				return true; // Use LLM with timeline context
			}
		}
		return false;
	};

	// Check DataTable events first (anchor priority)
	if (WorldTimelineTable)
	{
		TArray<FWorldTimelineEvent*> AllRows;
		WorldTimelineTable->GetAllRows<FWorldTimelineEvent>(TEXT("WorldDirector_Timeline"), AllRows);

		for (FWorldTimelineEvent* Event : AllRows)
		{
			bool bResult = TryFireEvent(Event, true);
			if (bResult || Event && Event->bHasFired) return bResult;
		}
	}

	// Check Dynamic Timeline events (LLM-generated)
	for (FWorldTimelineEvent& DynEvent : DynamicTimeline)
	{
		bool bResult = TryFireEvent(&DynEvent, false);
		if (bResult || DynEvent.bHasFired) return bResult;
	}

	// Clean up fired dynamic events
	DynamicTimeline.RemoveAll([](const FWorldTimelineEvent& E) { return E.bHasFired; });

	return false;
}

// ============================================================================
// Tension Evaluation
// ============================================================================

FTensionSnapshot UWorldDirectorSubsystem::EvaluateTension() const
{
	FTensionSnapshot T;

	// --- DeathPressure: based on DeadVIPs count ---
	if (NarrativeHistory)
	{
		// Access HistoryLog indirectly: count death-tagged events in world state
		FString WorldState = NarrativeHistory->GetWorldStateDescription(20);
		int32 DeathMentions = 0;
		int32 SearchFrom = 0;
		while (true)
		{
			int32 Found = WorldState.Find(TEXT("killed"), ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchFrom);
			if (Found == INDEX_NONE) break;
			DeathMentions++;
			SearchFrom = Found + 6;
		}
		T.DeathPressure = FMath::Clamp(DeathMentions * 0.15f, 0.0f, 1.0f);
	}

	// --- FactionVolatility: ratio of hostile pairs ---
	if (FactionSub)
	{
		int32 TotalPairs = 0;
		int32 HostilePairs = 0;

		for (const auto& OuterPair : FactionSub->RuntimeFactionMatrix)
		{
			for (const auto& InnerPair : OuterPair.Value)
			{
				if (OuterPair.Key != InnerPair.Key)
				{
					TotalPairs++;
					if (FactionHelpers::IsAttitudeHostile(InnerPair.Value))
					{
						HostilePairs++;
					}
				}
			}
		}

		T.FactionVolatility = (TotalPairs > 0)
			? FMath::Clamp((float)HostilePairs / (float)TotalPairs, 0.0f, 1.0f)
			: 0.0f;
	}

	// --- PlayerActivity: recent player-tagged events ---
	if (NarrativeHistory)
	{
		FString WorldState = NarrativeHistory->GetWorldStateDescription(20);
		int32 PlayerMentions = 0;
		int32 SearchFrom = 0;
		while (true)
		{
			int32 Found = WorldState.Find(TEXT("Player"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
			if (Found == INDEX_NONE) break;
			PlayerMentions++;
			SearchFrom = Found + 6;
		}
		T.PlayerActivity = FMath::Clamp(PlayerMentions * 0.1f, 0.0f, 1.0f);
	}

	// --- PopulationStress: absolute population deltas from starting values ---
	if (NarrativeHistory)
	{
		// Use the faction population tracking from NarrativeHistory
		// Large swings (deaths, migrations) increase stress
		int32 TotalDelta = 0;
		// We can approximate by checking common factions
		TArray<FName> Factions = {FName("Human"), FName("Orc"), FName("Zombie")};
		for (const FName& F : Factions)
		{
			int32 Pop = NarrativeHistory->GetFactionPopulation(F);
			// Negative population means deaths; use abs
			TotalDelta += FMath::Abs(Pop);
		}
		T.PopulationStress = FMath::Clamp(TotalDelta * 0.05f, 0.0f, 1.0f);
	}

	// --- TimelinePressure: proximity to next unfired timeline event ---
	if (WorldTimelineTable && TimeMgr)
	{
		int32 CurrentDay = TimeMgr->GetCurrentDay();
		float CurrentHour = TimeMgr->GetCurrentHour();
		float MinDaysUntil = 999.0f;

		TArray<FWorldTimelineEvent*> AllRows;
		WorldTimelineTable->GetAllRows<FWorldTimelineEvent>(TEXT("WorldDirector_Tension"), AllRows);

		for (const FWorldTimelineEvent* Event : AllRows)
		{
			if (!Event || Event->bHasFired) continue;
			float DaysUntil = (float)(Event->TriggerDay - CurrentDay) + (Event->TriggerHour - CurrentHour) / 24.0f;
			if (DaysUntil < MinDaysUntil)
			{
				MinDaysUntil = DaysUntil;
			}
		}

		// Closer events = more pressure. 0 days = 1.0, 7+ days = 0.0
		if (MinDaysUntil < 999.0f)
		{
			T.TimelinePressure = FMath::Clamp(1.0f - (MinDaysUntil / 7.0f), 0.0f, 1.0f);
		}
	}

	// --- Monotony: time since last dramatic beat ---
	if (GetWorld())
	{
		float GameTime = GetWorld()->GetTimeSeconds();
		float TimeSinceBeat = GameTime - LastBeatGameTime;
		// Normalize: 0 at beat, 1.0 at 10 minutes (600s)
		T.Monotony = FMath::Clamp(TimeSinceBeat / 600.0f, 0.0f, 1.0f);
	}

	// --- Overall Tension (weighted composite) ---
	T.OverallTension =
		0.25f * T.DeathPressure +
		0.20f * T.FactionVolatility +
		0.15f * T.PlayerActivity +
		0.10f * T.PopulationStress +
		0.15f * T.TimelinePressure +
		0.15f * T.Monotony;

	// Add tension boost from active major events
	for (const FActiveWorldEvent& ActiveEvt : ActiveMajorEvents)
	{
		T.OverallTension += ActiveEvt.TensionBoost;
	}

	T.OverallTension = FMath::Clamp(T.OverallTension, 0.0f, 1.0f);

	return T;
}

// ============================================================================
// Action Selection
// ============================================================================

EDramaticAction UWorldDirectorSubsystem::SelectDramaticAction(const FTensionSnapshot& Tension) const
{
	// Low tension -> relieve (celebrate, gift)
	if (Tension.OverallTension < RelieveThreshold)
	{
		return EDramaticAction::RELIEVE;
	}

	// High tension -> escalate
	if (Tension.OverallTension > EscalateThreshold)
	{
		return EDramaticAction::ESCALATE;
	}

	// Medium tension -> pick based on highest dimension
	float MaxDim = 0.0f;
	EDramaticAction BestAction = EDramaticAction::DISRUPT;

	if (Tension.FactionVolatility > MaxDim)
	{
		MaxDim = Tension.FactionVolatility;
		BestAction = EDramaticAction::CONVERGE;
	}
	if (Tension.Monotony > MaxDim)
	{
		MaxDim = Tension.Monotony;
		BestAction = EDramaticAction::DISRUPT;
	}
	if (Tension.DeathPressure > MaxDim)
	{
		MaxDim = Tension.DeathPressure;
		BestAction = EDramaticAction::REVEAL;
	}
	if (Tension.PlayerActivity > MaxDim)
	{
		MaxDim = Tension.PlayerActivity;
		BestAction = EDramaticAction::CONVERGE;
	}
	if (Tension.PopulationStress > MaxDim)
	{
		MaxDim = Tension.PopulationStress;
		BestAction = EDramaticAction::ESCALATE;
	}

	return BestAction;
}

// ============================================================================
// World Prediction (Dynamic Timeline Generation)
// ============================================================================

void UWorldDirectorSubsystem::OnDayChanged(int32 NewDay)
{
	DIRECTOR_LOG(Log, TEXT("Day changed to %d. Requesting world prediction."), NewDay);
	RequestWorldPrediction();
}

void UWorldDirectorSubsystem::ForcePrediction()
{
	RequestWorldPrediction();
}

void UWorldDirectorSubsystem::RequestWorldPrediction()
{
	if (!LLMService)
	{
		DIRECTOR_LOG(Warning, TEXT("LLM Service not available. Skipping world prediction."));
		return;
	}

	if (bPredictionInFlight)
	{
		DIRECTOR_LOG(Warning, TEXT("World prediction already in flight. Skipping."));
		return;
	}

	bPredictionInFlight = true;

	int32 CurrentDay = TimeMgr ? TimeMgr->GetCurrentDay() : 0;

	// --- System Prompt ---
	FString SystemPrompt = TEXT(
		"You are a world simulation AI for a medieval fantasy RPG.\n"
		"Based on the world state provided, predict 1-3 events that should happen in the next 1-3 game days.\n\n"
		"Output EXACTLY this JSON schema (no extra keys, no markdown):\n"
		"{\n"
		"  \"predicted_events\": [\n"
		"    {\n"
		"      \"trigger_day\": <integer, must be > current day>,\n"
		"      \"trigger_hour\": <float 0-23>,\n"
		"      \"event_name\": \"short name\",\n"
		"      \"event_description\": \"what happens and why\",\n"
		"      \"dramatic_action\": \"ESCALATE|DISRUPT|CONVERGE|REVEAL|RELIEVE\"\n"
		"    }\n"
		"  ]\n"
		"}\n\n"
		"HARD RULES:\n"
		"- NEVER use NPCs listed in [DEAD NPCs] - they are permanently dead\n"
		"- NEVER contradict events in [CRITICAL EVENTS] or [RECENT EVENTS]\n"
		"- NEVER repeat plots from [RECENT PLOT THREADS]\n"
		"- Events must logically follow from the current world state\n"
		"- trigger_day MUST be greater than the current day\n"
		"- Keep events grounded in the existing factions and NPC types\n"
	);

	// --- User Prompt ---
	FString UserPrompt = FString::Printf(TEXT("[CURRENT DAY]: %d\n\n"), CurrentDay);

	if (NarrativeHistory)
	{
		UserPrompt += NarrativeHistory->GetStructuredWorldSnapshot(10);
		UserPrompt += TEXT("\n");
	}

	// Add tension
	UserPrompt += FString::Printf(
		TEXT("[TENSION VALUES]: Death=%.2f, Faction=%.2f, Player=%.2f, Pop=%.2f, Overall=%.2f\n\n"),
		LatestTension.DeathPressure, LatestTension.FactionVolatility,
		LatestTension.PlayerActivity, LatestTension.PopulationStress,
		LatestTension.OverallTension);

	// Add active world events
	FString EventsDesc = GetActiveEventsDescription();
	if (!EventsDesc.IsEmpty())
	{
		UserPrompt += FString::Printf(TEXT("[ACTIVE WORLD EVENTS]:\n%s\n"), *EventsDesc);
	}

	// Add pending dynamic events so LLM doesn't duplicate them
	if (DynamicTimeline.Num() > 0)
	{
		UserPrompt += TEXT("[ALREADY SCHEDULED DYNAMIC EVENTS]:\n");
		for (const FWorldTimelineEvent& DynEvt : DynamicTimeline)
		{
			UserPrompt += FString::Printf(TEXT("- Day %d Hour %.0f: %s\n"), DynEvt.TriggerDay, DynEvt.TriggerHour, *DynEvt.EventName);
		}
		UserPrompt += TEXT("\n");
	}

	DIRECTOR_LOG(Log, TEXT("=== World Prediction Request ===\n%s"), *UserPrompt);

	FOnLLMResponseRaw Callback;
	Callback.BindLambda([this](bool bSuccess, const FString& Response)
	{
		OnWorldPredictionResponse(bSuccess, Response);
	});

	LLMService->SendFunctionalRequest(
		SystemPrompt,
		UserPrompt,
		Callback,
		0.6f,   // Slightly higher temperature for creative prediction
		true,   // JSON mode
		512     // MaxTokens
	);
}

void UWorldDirectorSubsystem::OnWorldPredictionResponse(bool bSuccess, const FString& Response)
{
	bPredictionInFlight = false;

	if (!bSuccess || Response.IsEmpty())
	{
		DIRECTOR_LOG(Warning, TEXT("World prediction LLM call failed."));
		return;
	}

	DIRECTOR_LOG(Log, TEXT("=== World Prediction Response ===\n%s"), *Response);

	TArray<FWorldTimelineEvent> NewEvents;
	if (!ParsePredictionEvents(Response, NewEvents))
	{
		DIRECTOR_LOG(Warning, TEXT("Failed to parse world prediction response."));
		return;
	}

	int32 CurrentDay = TimeMgr ? TimeMgr->GetCurrentDay() : 0;

	// Validate and add events
	for (FWorldTimelineEvent& Evt : NewEvents)
	{
		// Sanity check: must be in the future
		if (Evt.TriggerDay <= CurrentDay)
		{
			DIRECTOR_LOG(Warning, TEXT("Predicted event '%s' has invalid day %d (current: %d). Skipping."),
				*Evt.EventName, Evt.TriggerDay, CurrentDay);
			continue;
		}

		// Clamp hour
		Evt.TriggerHour = FMath::Clamp(Evt.TriggerHour, 0.0f, 23.99f);

		DynamicTimeline.Add(Evt);
		DIRECTOR_LOG(Log, TEXT("Dynamic event added: '%s' (Day %d, Hour %.1f, Action: %s)"),
			*Evt.EventName, Evt.TriggerDay, Evt.TriggerHour, *Evt.EventDescription);
	}

	DIRECTOR_LOG(Warning, TEXT("World prediction complete: %d new events added. Total dynamic: %d"),
		NewEvents.Num(), DynamicTimeline.Num());
}

bool UWorldDirectorSubsystem::ParsePredictionEvents(const FString& JsonString, TArray<FWorldTimelineEvent>& OutEvents) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* EventsArray;
	if (!JsonObject->TryGetArrayField(TEXT("predicted_events"), EventsArray))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Val : *EventsArray)
	{
		const TSharedPtr<FJsonObject>* EventObj;
		if (!Val->TryGetObject(EventObj)) continue;

		FWorldTimelineEvent Evt;
		double DayVal = 0;
		if ((*EventObj)->TryGetNumberField(TEXT("trigger_day"), DayVal))
		{
			Evt.TriggerDay = (int32)DayVal;
		}

		double HourVal = 12.0;
		(*EventObj)->TryGetNumberField(TEXT("trigger_hour"), HourVal);
		Evt.TriggerHour = (float)HourVal;

		(*EventObj)->TryGetStringField(TEXT("event_name"), Evt.EventName);
		(*EventObj)->TryGetStringField(TEXT("event_description"), Evt.EventDescription);

		// Parse dramatic action
		FString ActionStr;
		if ((*EventObj)->TryGetStringField(TEXT("dramatic_action"), ActionStr))
		{
			if (ActionStr == TEXT("ESCALATE"))      Evt.ForcedAction = EDramaticAction::ESCALATE;
			else if (ActionStr == TEXT("DISRUPT"))   Evt.ForcedAction = EDramaticAction::DISRUPT;
			else if (ActionStr == TEXT("CONVERGE"))  Evt.ForcedAction = EDramaticAction::CONVERGE;
			else if (ActionStr == TEXT("REVEAL"))    Evt.ForcedAction = EDramaticAction::REVEAL;
			else if (ActionStr == TEXT("RELIEVE"))   Evt.ForcedAction = EDramaticAction::RELIEVE;
		}

		if (!Evt.EventName.IsEmpty())
		{
			OutEvents.Add(Evt);
		}
	}

	return OutEvents.Num() > 0;
}

// ============================================================================
// Scene Validation
// ============================================================================

bool UWorldDirectorSubsystem::ValidateSceneBlueprint(const FLLMSceneBlueprint& Blueprint, EDramaticAction Action) const
{
	if (!NarrativeHistory) return true; // No history = no constraints

	// 1. Dead NPC check: ensure cast doesn't reference dead NPCs
	FString DeadList = NarrativeHistory->GetDeadNPCList();
	if (!DeadList.IsEmpty())
	{
		for (const FString& CastDesc : Blueprint.CastDescriptions)
		{
			FString LowerDesc = CastDesc.ToLower();
			// Check each dead NPC name against cast
			TArray<FString> DeadNames;
			DeadList.ParseIntoArray(DeadNames, TEXT(", "));
			for (const FString& DeadName : DeadNames)
			{
				if (LowerDesc.Contains(DeadName.ToLower()))
				{
					DIRECTOR_LOG(Warning, TEXT("Scene validation FAILED: Cast '%s' references dead NPC '%s'"),
						*CastDesc, *DeadName);
					return false;
				}
			}
		}
	}

	// 2. Plot deduplication: check against recent plot threads
	FString RecentPlots = NarrativeHistory->GetRecentPlotThreads(5);
	FString LowerOutline = Blueprint.PlotOutline.ToLower();

	// Simple keyword overlap check for plot similarity
	// Extract words from plot outline and check how many appear in recent plots
	TArray<FString> OutlineWords;
	LowerOutline.ParseIntoArray(OutlineWords, TEXT(" "));

	FString LowerRecentPlots = RecentPlots.ToLower();
	int32 MatchingWords = 0;
	for (const FString& Word : OutlineWords)
	{
		if (Word.Len() > 4 && LowerRecentPlots.Contains(Word)) // Only count significant words
		{
			MatchingWords++;
		}
	}

	// If more than 60% of significant words match recent plots, it's too similar
	int32 SignificantWords = 0;
	for (const FString& Word : OutlineWords)
	{
		if (Word.Len() > 4) SignificantWords++;
	}

	if (SignificantWords > 0 && MatchingWords > 0)
	{
		float SimilarityRatio = (float)MatchingWords / (float)SignificantWords;
		if (SimilarityRatio > 0.6f)
		{
			DIRECTOR_LOG(Warning, TEXT("Scene validation FAILED: Plot too similar to recent threads (%.0f%% overlap). Plot: %s"),
				SimilarityRatio * 100.0f, *Blueprint.PlotOutline);
			return false;
		}
	}

	// 3. Faction logic: hostile factions cooperating requires CONVERGE
	if (Action != EDramaticAction::CONVERGE && FactionSub && Blueprint.CastDescriptions.Num() >= 2)
	{
		// Extract faction keywords from cast descriptions and check if they're hostile
		TArray<FName> MentionedFactions;
		for (const FString& CastDesc : Blueprint.CastDescriptions)
		{
			FString LowerDesc = CastDesc.ToLower();
			for (const auto& Pair : FactionSub->RuntimeFactionMatrix)
			{
				if (LowerDesc.Contains(Pair.Key.ToString().ToLower()))
				{
					MentionedFactions.AddUnique(Pair.Key);
				}
			}
		}

		// Check if any pair of mentioned factions is hostile but plot suggests cooperation
		FString LowerPlot = Blueprint.PlotOutline.ToLower();
		bool bPlotSuggestsCooperation = LowerPlot.Contains(TEXT("together")) ||
			LowerPlot.Contains(TEXT("alliance")) || LowerPlot.Contains(TEXT("cooperat")) ||
			LowerPlot.Contains(TEXT("negotiate")) || LowerPlot.Contains(TEXT("peace"));

		if (bPlotSuggestsCooperation)
		{
			for (int32 i = 0; i < MentionedFactions.Num(); i++)
			{
				for (int32 j = i + 1; j < MentionedFactions.Num(); j++)
				{
					if (FactionSub->AreFactionsHostile(MentionedFactions[i], MentionedFactions[j]))
					{
						DIRECTOR_LOG(Warning, TEXT("Scene validation FAILED: Hostile factions %s and %s cooperating without CONVERGE action"),
							*MentionedFactions[i].ToString(), *MentionedFactions[j].ToString());
						return false;
					}
				}
			}
		}
	}

	return true;
}

// ============================================================================
// LLM Integration
// ============================================================================

void UWorldDirectorSubsystem::RequestLLMSceneGeneration(EDramaticAction Action, const FString& TimelineContext)
{
	if (!LLMService)
	{
		DIRECTOR_LOG(Warning, TEXT("LLM Service not available. Aborting beat."));
		return;
	}

	// --- Build System Prompt ---
	FString SystemPrompt = TEXT(
		"You are a narrative director AI for a medieval fantasy RPG world.\n"
		"Generate a short dramatic scene blueprint as JSON.\n\n"
		"Output EXACTLY this JSON schema (no extra keys, no markdown):\n"
		"{\n"
		"  \"plot_outline\": \"One paragraph describing what happens in this scene\",\n"
		"  \"cast\": [\"description of NPC 1\", \"description of NPC 2\"],\n"
		"  \"location_type\": \"tavern|town_square|market|forest_edge|mine|temple|barracks\",\n"
		"  \"completion_hints\": [\"what event ends this scene\"],\n"
		"  \"companion_hint_before\": \"What the companion says to foreshadow\",\n"
		"  \"companion_hint_during\": \"What the companion observes mid-scene\",\n"
		"  \"companion_hint_after\": \"What the companion reflects on after\"\n"
		"}\n\n"
		"HARD RULES:\n"
		"- NEVER use NPCs listed in [DEAD NPCs] - they are permanently dead and cannot appear\n"
		"- NEVER contradict events in [CRITICAL EVENTS] or [RECENT EVENTS]\n"
		"- NEVER repeat plots from [RECENT PLOT THREADS] - every scene must be unique\n"
		"- Cast descriptions MUST match NPC types from [LIVING NPCs] (e.g. 'an orc merchant', 'a human guard')\n"
		"- Hostile factions should NOT cooperate unless the dramatic action is CONVERGE\n"
		"- Keep cast to 2-4 NPCs maximum, only use NPCs marked as available (not IN SCENE)\n"
		"- Plot should be self-contained and completable within a few minutes\n"
		"- Companion hints should be short (1-2 sentences), in-character as a curious fairy companion\n"
	);

	// --- Build User Prompt ---
	FString ActionName;
	switch (Action)
	{
	case EDramaticAction::ESCALATE: ActionName = TEXT("ESCALATE (raise stakes, conflict, betrayal)"); break;
	case EDramaticAction::DISRUPT:  ActionName = TEXT("DISRUPT (unexpected event, break routine)"); break;
	case EDramaticAction::CONVERGE: ActionName = TEXT("CONVERGE (bring factions together, negotiate)"); break;
	case EDramaticAction::REVEAL:   ActionName = TEXT("REVEAL (expose secret, hidden truth)"); break;
	case EDramaticAction::RELIEVE:  ActionName = TEXT("RELIEVE (celebration, peace, gift)"); break;
	}

	FString UserPrompt = FString::Printf(TEXT("[DRAMATIC ACTION]: %s\n\n"), *ActionName);

	// Add timeline context if present
	if (!TimelineContext.IsEmpty())
	{
		UserPrompt += FString::Printf(TEXT("[TIMELINE CONTEXT]: %s\n\n"), *TimelineContext);
	}

	// Add structured world snapshot (replaces old GetWorldStateDescription)
	if (NarrativeHistory)
	{
		UserPrompt += NarrativeHistory->GetStructuredWorldSnapshot(10);
		UserPrompt += TEXT("\n");
	}

	// Add tension values
	UserPrompt += FString::Printf(
		TEXT("[TENSION VALUES]: Death=%.2f, Faction=%.2f, Player=%.2f, Pop=%.2f, Timeline=%.2f, Monotony=%.2f, Overall=%.2f\n\n"),
		LatestTension.DeathPressure, LatestTension.FactionVolatility,
		LatestTension.PlayerActivity, LatestTension.PopulationStress,
		LatestTension.TimelinePressure, LatestTension.Monotony,
		LatestTension.OverallTension);

	// Add available factions with relationships
	if (FactionSub)
	{
		UserPrompt += TEXT("[FACTION RELATIONSHIPS]:\n");
		for (const auto& OuterPair : FactionSub->RuntimeFactionMatrix)
		{
			for (const auto& InnerPair : OuterPair.Value)
			{
				if (OuterPair.Key != InnerPair.Key)
				{
					FString Attitude = InnerPair.Value < 30.0f ? TEXT("Hostile") :
						InnerPair.Value > 70.0f ? TEXT("Friendly") : TEXT("Neutral");
					UserPrompt += FString::Printf(TEXT("- %s -> %s: %.0f (%s)\n"),
						*OuterPair.Key.ToString(), *InnerPair.Key.ToString(), InnerPair.Value, *Attitude);
				}
			}
		}
		UserPrompt += TEXT("\n");
	}

	// Add active world events context
	FString EventsDesc = GetActiveEventsDescription();
	if (!EventsDesc.IsEmpty())
	{
		UserPrompt += FString::Printf(TEXT("[ACTIVE WORLD EVENTS]:\n%s\n"), *EventsDesc);
	}

	DIRECTOR_LOG(Log, TEXT("=== LLM Scene Request ===\nAction: %s\n%s"), *ActionName, *UserPrompt);

	// --- Send Request ---
	FOnLLMResponseRaw Callback;
	Callback.BindLambda([this, Action](bool bSuccess, const FString& Response)
	{
		OnLLMSceneResponse(bSuccess, Response, Action);
	});

	LLMService->SendFunctionalRequest(
		SystemPrompt,
		UserPrompt,
		Callback,
		0.4f,   // Temperature
		true,   // JSON mode
		512     // MaxTokens
	);
}

void UWorldDirectorSubsystem::OnLLMSceneResponse(bool bSuccess, const FString& Response, EDramaticAction Action)
{
	if (!bSuccess || Response.IsEmpty())
	{
		DIRECTOR_LOG(Warning, TEXT("LLM scene generation failed. Monotony will increase for retry next cycle."));
		return;
	}

	DIRECTOR_LOG(Log, TEXT("=== LLM Scene Response ===\n%s"), *Response);

	// Parse the response
	FLLMSceneBlueprint Blueprint;
	if (!ParseLLMSceneBlueprint(Response, Blueprint))
	{
		DIRECTOR_LOG(Warning, TEXT("Failed to parse LLM scene blueprint. Aborting beat."));
		return;
	}

	// Validate before assembly
	if (!ValidateSceneBlueprint(Blueprint, Action))
	{
		DIRECTOR_LOG(Warning, TEXT("Scene blueprint failed validation. Aborting beat."));
		return;
	}

	// Assemble scene
	AssembleAndSubmitScene(Blueprint, Action);

	// Record plot thread for deduplication
	if (NarrativeHistory)
	{
		FString ActionStr;
		switch (Action)
		{
		case EDramaticAction::ESCALATE: ActionStr = TEXT("ESCALATE"); break;
		case EDramaticAction::DISRUPT:  ActionStr = TEXT("DISRUPT"); break;
		case EDramaticAction::CONVERGE: ActionStr = TEXT("CONVERGE"); break;
		case EDramaticAction::REVEAL:   ActionStr = TEXT("REVEAL"); break;
		case EDramaticAction::RELIEVE:  ActionStr = TEXT("RELIEVE"); break;
		}
		NarrativeHistory->RecordPlotThread(Blueprint.PlotOutline, ActionStr);
	}

	// Deliver companion hints
	DeliverCompanionHints(Blueprint);

	// Update beat tracking
	if (GetWorld())
	{
		LastBeatGameTime = GetWorld()->GetTimeSeconds();
	}
}

bool UWorldDirectorSubsystem::ParseLLMSceneBlueprint(const FString& JsonString, FLLMSceneBlueprint& OutBlueprint) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		DIRECTOR_LOG(Warning, TEXT("JSON parse failed for scene blueprint."));
		return false;
	}

	// plot_outline (required)
	if (!JsonObject->TryGetStringField(TEXT("plot_outline"), OutBlueprint.PlotOutline))
	{
		DIRECTOR_LOG(Warning, TEXT("Missing 'plot_outline' in LLM response."));
		return false;
	}

	// cast (required, array of strings)
	const TArray<TSharedPtr<FJsonValue>>* CastArray;
	if (JsonObject->TryGetArrayField(TEXT("cast"), CastArray))
	{
		for (const TSharedPtr<FJsonValue>& Val : *CastArray)
		{
			FString Desc;
			if (Val->TryGetString(Desc))
			{
				OutBlueprint.CastDescriptions.Add(Desc);
			}
		}
	}

	if (OutBlueprint.CastDescriptions.Num() == 0)
	{
		DIRECTOR_LOG(Warning, TEXT("Empty 'cast' array in LLM response."));
		return false;
	}

	// location_type
	JsonObject->TryGetStringField(TEXT("location_type"), OutBlueprint.LocationType);

	// completion_hints
	const TArray<TSharedPtr<FJsonValue>>* HintsArray;
	if (JsonObject->TryGetArrayField(TEXT("completion_hints"), HintsArray))
	{
		for (const TSharedPtr<FJsonValue>& Val : *HintsArray)
		{
			FString Hint;
			if (Val->TryGetString(Hint))
			{
				OutBlueprint.CompletionHints.Add(Hint);
			}
		}
	}

	// companion hints
	JsonObject->TryGetStringField(TEXT("companion_hint_before"), OutBlueprint.CompanionHintBefore);
	JsonObject->TryGetStringField(TEXT("companion_hint_during"), OutBlueprint.CompanionHintDuring);
	JsonObject->TryGetStringField(TEXT("companion_hint_after"), OutBlueprint.CompanionHintAfter);

	return true;
}

// ============================================================================
// Scene Assembly
// ============================================================================

void UWorldDirectorSubsystem::AssembleAndSubmitScene(const FLLMSceneBlueprint& Blueprint, EDramaticAction Action)
{
	if (!SquadSubsystem)
	{
		DIRECTOR_LOG(Warning, TEXT("SquadSubsystem not available. Cannot assemble scene."));
		return;
	}

	// 1. Find anchor by location type
	ANarrativeSceneAnchor* Anchor = SquadSubsystem->FindAnchorByLocationType(Blueprint.LocationType);
	if (!Anchor)
	{
		DIRECTOR_LOG(Warning, TEXT("No available anchor for location '%s'. Aborting scene."), *Blueprint.LocationType);
		return;
	}

	// 2. Find matching NPCs
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> AllNPCs = NPCQueryHelpers::GetAllLivingNPCs(World);
	TArray<AActor*> Available = NPCQueryHelpers::FilterAvailable(AllNPCs);

	TArray<AActor*> MatchedCast;
	for (const FString& Desc : Blueprint.CastDescriptions)
	{
		AActor* Match = NPCQueryHelpers::FindNPCByDescription(Available, Desc);
		if (Match)
		{
			MatchedCast.Add(Match);
			// Remove from available so we don't double-assign
			Available.Remove(Match);
			DIRECTOR_LOG(Log, TEXT("Cast match: '%s' -> %s"), *Desc, *NPCQueryHelpers::BuildNPCBrief(Match));
		}
		else
		{
			DIRECTOR_LOG(Warning, TEXT("No NPC match for cast description: '%s'"), *Desc);
		}
	}

	if (MatchedCast.Num() == 0)
	{
		DIRECTOR_LOG(Warning, TEXT("No NPCs matched any cast description. Aborting scene."));
		return;
	}

	// 3. Build completion conditions from hints
	TArray<FNarrativeEventMatcher> CompletionConditions;
	// Use a generic scene-end condition: listen for WorldDirector.Event
	FNarrativeEventMatcher GenericEnd;
	GenericEnd.Tag = AINPCTags::WorldDirector_Event;
	CompletionConditions.Add(GenericEnd);

	// 4. Create the squad
	int32 SquadID = SquadSubsystem->CreateSceneSquad(Blueprint.PlotOutline, CompletionConditions);
	if (SquadID == -1)
	{
		DIRECTOR_LOG(Warning, TEXT("Failed to create scene squad."));
		return;
	}

	// 5. Assign cast members
	for (int32 i = 0; i < MatchedCast.Num(); i++)
	{
		FString RoleDesc = (i < Blueprint.CastDescriptions.Num())
			? Blueprint.CastDescriptions[i]
			: FString::Printf(TEXT("Participant %d"), i);
		SquadSubsystem->AssignMemberRole(SquadID, MatchedCast[i], RoleDesc);
	}

	// 6. Link to anchor and activate
	Anchor->bIsOccupied = true;
	Anchor->CurrentSquadID = SquadID;
	SquadSubsystem->ActivateScene(SquadID);

	// 7. Track the squad
	ActiveDirectorSquadIDs.Add(SquadID);
	ActiveDirectorScenes++;

	// 8. Record event to history
	if (NarrativeHistory)
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(AINPCTags::WorldDirector_Event);
		Tags.AddTag(GetTagForAction(Action));

		FString EventDesc = FString::Printf(TEXT("[WorldDirector Beat] %s"), *Blueprint.PlotOutline);
		NarrativeHistory->RecordEvent(EventDesc, Tags);
	}

	// 9. Broadcast delegate
	OnDramaticBeat.Broadcast(Action, Blueprint.PlotOutline);

	DIRECTOR_LOG(Warning, TEXT("Scene assembled: Squad %d, %d NPCs, Anchor '%s'. Plot: %s"),
		SquadID, MatchedCast.Num(), *Anchor->GetName(), *Blueprint.PlotOutline);
}

// ============================================================================
// Companion Hints
// ============================================================================

void UWorldDirectorSubsystem::DeliverCompanionHints(const FLLMSceneBlueprint& Blueprint)
{
	ANarrativeCompanion* Companion = FindCompanion();
	if (!Companion) return;

	// Before: immediate
	if (!Blueprint.CompanionHintBefore.IsEmpty())
	{
		FNarrativeHint BeforeHint;
		BeforeHint.Phase = ENarrativeHintPhase::Before;
		BeforeHint.HintText = Blueprint.CompanionHintBefore;
		BeforeHint.Priority = 0.6f;
		Companion->ReceiveNarrativeHint(BeforeHint);
	}

	// During: 30s delay
	if (!Blueprint.CompanionHintDuring.IsEmpty())
	{
		FString DuringText = Blueprint.CompanionHintDuring;
		TWeakObjectPtr<ANarrativeCompanion> WeakCompanion = Companion;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				DuringHintTimerHandle,
				[WeakCompanion, DuringText]()
				{
					if (ANarrativeCompanion* Comp = WeakCompanion.Get())
					{
						FNarrativeHint DuringHint;
						DuringHint.Phase = ENarrativeHintPhase::During;
						DuringHint.HintText = DuringText;
						DuringHint.Priority = 0.5f;
						Comp->ReceiveNarrativeHint(DuringHint);
					}
				},
				30.0f,
				false
			);
		}
	}

	// After: stored for delivery on scene end (handled in OnDirectorEventRecorded)
	// We store it as a tag on ourselves for later retrieval — simplest approach
	// is to just capture it in a member variable
	if (!Blueprint.CompanionHintAfter.IsEmpty())
	{
		// Store for later delivery — using a simple approach
		PendingAfterHint = Blueprint.CompanionHintAfter;
	}
}

ANarrativeCompanion* UWorldDirectorSubsystem::FindCompanion() const
{
	if (!PlayerSquad) return nullptr;

	TArray<AActor*> Members = PlayerSquad->GetPartyMembers();
	for (AActor* Member : Members)
	{
		if (ANarrativeCompanion* Companion = Cast<ANarrativeCompanion>(Member))
		{
			return Companion;
		}
	}
	return nullptr;
}

// ============================================================================
// Event Handling
// ============================================================================

void UWorldDirectorSubsystem::OnDirectorEventRecorded(const FNarrativeEvent& Event)
{
	// Check if any of our director-managed scenes have ended
	// We detect this by checking if the squad IDs are still active
	TArray<int32> EndedSquads;
	for (int32 SquadID : ActiveDirectorSquadIDs)
	{
		// If the squad is no longer in the subsystem's active map, it ended
		if (SquadSubsystem)
		{
			TArray<AActor*> Members;
			if (!SquadSubsystem->GetSquadMembers(nullptr, Members))
			{
				// Can't easily check by ID from outside; we rely on scene count tracking
			}
		}
	}

	// Deliver "After" hint when a WorldDirector event is recorded (indicates scene activity)
	// This is a simplified approach; in production you'd track specific squad IDs
	bool bIsDirectorEvent = false;
	for (const FGameplayTag& Tag : Event.Tags)
	{
		if (Tag.MatchesTag(AINPCTags::WorldDirector_Event))
		{
			bIsDirectorEvent = true;
			break;
		}
	}

	// If the event description mentions scene end, deliver after hint
	if (bIsDirectorEvent && Event.Description.Contains(TEXT("ended")))
	{
		if (!PendingAfterHint.IsEmpty())
		{
			if (ANarrativeCompanion* Companion = FindCompanion())
			{
				FNarrativeHint AfterHint;
				AfterHint.Phase = ENarrativeHintPhase::After;
				AfterHint.HintText = PendingAfterHint;
				AfterHint.Priority = 0.5f;
				Companion->ReceiveNarrativeHint(AfterHint);
			}
			PendingAfterHint.Empty();
		}

		// Decrement scene count
		if (ActiveDirectorScenes > 0)
		{
			ActiveDirectorScenes--;
		}
	}
}

// ============================================================================
// Helpers
// ============================================================================

FGameplayTag UWorldDirectorSubsystem::GetTagForAction(EDramaticAction Action) const
{
	switch (Action)
	{
	case EDramaticAction::ESCALATE: return AINPCTags::WorldDirector_Escalate;
	case EDramaticAction::DISRUPT:  return AINPCTags::WorldDirector_Disrupt;
	case EDramaticAction::CONVERGE: return AINPCTags::WorldDirector_Converge;
	case EDramaticAction::REVEAL:   return AINPCTags::WorldDirector_Reveal;
	case EDramaticAction::RELIEVE:  return AINPCTags::WorldDirector_Relieve;
	default:                        return AINPCTags::WorldDirector_Event;
	}
}

// ============================================================================
// World Major Events
// ============================================================================

void UWorldDirectorSubsystem::CheckMajorEvents()
{
	if (!MajorEventsTable || !TimeMgr) return;

	int32 Day = TimeMgr->GetCurrentDay();
	float Hour = TimeMgr->GetCurrentHour();

	TArray<FName> RowNames = MajorEventsTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FWorldMajorEventRow* Row = MajorEventsTable->FindRow<FWorldMajorEventRow>(RowName, TEXT("WorldDirector_MajorEvents"));
		if (!Row || Row->bHasStarted) continue;

		bool bShouldTrigger = false;
		if (Row->TriggerDay < Day)
		{
			bShouldTrigger = true;
		}
		else if (Row->TriggerDay == Day && Hour >= Row->TriggerHour)
		{
			bShouldTrigger = true;
		}

		if (!bShouldTrigger) continue;

		Row->bHasStarted = true;

		// Build FActiveWorldEvent with pre-computed end time
		FActiveWorldEvent Active;
		Active.RowName = RowName;
		Active.EventType = Row->EventType;
		Active.EventName = Row->EventName;
		Active.EventDescription = Row->EventDescription;
		Active.StartDay = Day;
		Active.StartHour = Hour;
		Active.TensionBoost = Row->TensionBoost;
		Active.EventTag = GetTagForEventType(Row->EventType);

		// Pre-compute end time
		float TotalEndHours = Hour + Row->DurationDays * 24.0f;
		Active.EndDay = Day + FMath::FloorToInt32(TotalEndHours / 24.0f);
		Active.EndHour = FMath::Fmod(TotalEndHours, 24.0f);

		ActiveMajorEvents.Add(Active);

		// Record to history
		if (NarrativeHistory)
		{
			FGameplayTagContainer Tags;
			Tags.AddTag(AINPCTags::WorldDirector_Event);
			Tags.AddTag(Active.EventTag);
			NarrativeHistory->RecordEvent(
				FString::Printf(TEXT("[WorldDirector] Major Event Started: %s - %s"), *Active.EventName, *Active.EventDescription), Tags);
		}

		// Broadcast delegate
		OnWorldMajorEventStarted.Broadcast(Active.EventType, Active);

		// Broadcast dramatic beat with the forced action
		OnDramaticBeat.Broadcast(Row->ForcedStartAction, Active.EventName);

		DIRECTOR_LOG(Warning, TEXT("Major Event STARTED: '%s' (Type=%d, Day %d Hour %.1f, Duration %.1f days, EndDay %d EndHour %.1f)"),
			*Active.EventName, (int32)Active.EventType, Day, Hour, Row->DurationDays, Active.EndDay, Active.EndHour);
	}
}

void UWorldDirectorSubsystem::TickActiveEvents()
{
	if (!TimeMgr || ActiveMajorEvents.Num() == 0) return;

	int32 Day = TimeMgr->GetCurrentDay();
	float Hour = TimeMgr->GetCurrentHour();

	// Reverse iterate for safe removal
	for (int32 i = ActiveMajorEvents.Num() - 1; i >= 0; --i)
	{
		const FActiveWorldEvent& Active = ActiveMajorEvents[i];

		bool bExpired = false;
		if (Day > Active.EndDay)
		{
			bExpired = true;
		}
		else if (Day == Active.EndDay && Hour >= Active.EndHour)
		{
			bExpired = true;
		}

		if (!bExpired) continue;

		// Record to history
		if (NarrativeHistory)
		{
			FGameplayTagContainer Tags;
			Tags.AddTag(AINPCTags::WorldDirector_Event);
			Tags.AddTag(Active.EventTag);
			NarrativeHistory->RecordEvent(
				FString::Printf(TEXT("[WorldDirector] Major Event Ended: %s"), *Active.EventName), Tags);
		}

		// Broadcast ended delegate
		OnWorldMajorEventEnded.Broadcast(Active.EventType, Active);

		DIRECTOR_LOG(Warning, TEXT("Major Event ENDED: '%s' (Day %d Hour %.1f)"),
			*Active.EventName, Day, Hour);

		ActiveMajorEvents.RemoveAt(i);
	}
}

FString UWorldDirectorSubsystem::GetActiveEventsDescription() const
{
	if (ActiveMajorEvents.Num() == 0) return FString();

	FString Result;
	for (const FActiveWorldEvent& Active : ActiveMajorEvents)
	{
		Result += FString::Printf(TEXT("- [Day %d-Day %d] %s: %s\n"),
			Active.StartDay, Active.EndDay, *Active.EventName, *Active.EventDescription);
	}
	return Result;
}

FGameplayTag UWorldDirectorSubsystem::GetTagForEventType(EWorldMajorEventType EventType) const
{
	switch (EventType)
	{
	case EWorldMajorEventType::BeastTide:       return AINPCTags::WorldEvent_BeastTide;
	case EWorldMajorEventType::DemonInvasion:   return AINPCTags::WorldEvent_DemonInvasion;
	case EWorldMajorEventType::FactionWar:      return AINPCTags::WorldEvent_FactionWar;
	case EWorldMajorEventType::NaturalDisaster: return AINPCTags::WorldEvent_NaturalDisaster;
	case EWorldMajorEventType::Festival:        return AINPCTags::WorldEvent_Festival;
	case EWorldMajorEventType::Plague:          return AINPCTags::WorldEvent_Plague;
	case EWorldMajorEventType::Custom:          return AINPCTags::WorldEvent_Custom;
	default:                                    return AINPCTags::WorldEvent_Custom;
	}
}


