#include "Subsystems/NarrativeSquadSubsystem.h"
#include "AINPC.h"
#include "Subsystems/NarrativeDirectorSubsystem.h"
#include "Components/CognitionComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h" // Required for FOverlapResult
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Components/NPCDefinitionComponent.h"
#include "Components/GoalComponent.h"
#include "Controller/UtilityAIController.h"
#include "Components/PersonalityComponent.h"
#include "Components/UtilityAIComponent.h"
#include "World/NarrativeSceneAnchor.h"
#include "Social/SocialGameplayTags.h"
#include "Utilities/AINPCHelpers.h"
#include "Components/EmotionDisplayComponent.h"
#include "Components/SphereComponent.h"


void UNarrativeSquadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Bind to Director
	if (UNarrativeDirectorSubsystem* Director = GetWorld()->GetSubsystem<UNarrativeDirectorSubsystem>())
	{
		Director->OnEventRecorded.AddDynamic(this, &UNarrativeSquadSubsystem::OnNarrativeEventRecorded);
		NARRATIVE_LOG(Warning, TEXT("✅ [NarrativeSquadSubsystem] Successfully bound to NarrativeDirectorSubsystem::OnEventRecorded"));
	}
	else
	{
		NARRATIVE_LOG(Error, TEXT("❌ [NarrativeSquadSubsystem] Failed to find NarrativeDirectorSubsystem!"));
	}
}

void UNarrativeSquadSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld()) 
	{
		if (UNarrativeDirectorSubsystem* Director = World->GetSubsystem<UNarrativeDirectorSubsystem>())
		{
			Director->OnEventRecorded.RemoveDynamic(this, &UNarrativeSquadSubsystem::OnNarrativeEventRecorded);
		}
	}
	Super::Deinitialize();
}

int32 UNarrativeSquadSubsystem::CreateSceneSquad(FString PlotOutline, TArray<FNarrativeEventMatcher> CompletionConditions)
{
	int32 NewID = NextSquadID++;
	FNarrativeSceneSquad Squad;
	Squad.SquadID = NewID;
	Squad.PlotOutline = PlotOutline;
	Squad.CompletionConditions = CompletionConditions;
	Squad.bIsActive = false;  // Scene waits for player to activate

	// ✅ CRITICAL: Suppress action observation by default (even before activation)
	// 关键：默认禁止动作观察（即使在激活之前）
	// This prevents NPCs from observing each other during the spawn sequence
	// 这可以防止 NPC 在 spawn 过程中互相观察
	Squad.bCurrentlySuppressingActionObservation = true;

	ActiveSquads.Add(NewID, Squad);

	NARRATIVE_LOG(Log, TEXT("[NarrativeSquad] Created Squad %d (Action Observation SUPPRESSED by default): %s"),
		NewID, *PlotOutline);
	return NewID;
}

void UNarrativeSquadSubsystem::AssignMemberRole(int32 SquadID, AActor* NPC, FString RoleDescription)
{
	if (!ActiveSquads.Contains(SquadID)) return;
	if (!NPC) return;

	FNarrativeSceneSquad& Squad = ActiveSquads[SquadID];
	Squad.MemberRoles.Add(NPC, FName(*RoleDescription));
	ActorSquadMap.Add(NPC, SquadID);

	NARRATIVE_LOG(Error, TEXT("🛑 [NarrativeSquad] ASSIGNED ROLE: '%s' to Actor '%s' (Squad %d)"), 
		*RoleDescription, *NPC->GetName(), SquadID);
	
	// Mark as In-Scene (Suppresses Hostility)
	if (UGoalComponent* GoalComp = AINPCHelpers::GetGoalComponent(NPC))

	{
		GoalComp->AddContextTag(AINPCTags::Status_InScene);
	}
	NPC->Tags.AddUnique("Status.InScene"); // Legacy / Fallback

	// ✅ DEBUG: Log NPC configuration
	if (UNPCDefinitionComponent* DefComp = NPC->FindComponentByClass<UNPCDefinitionComponent>())
	{
		NARRATIVE_LOG(Warning, TEXT("[NarrativeSquad] Assigned Role '%s' to NPC (Profession: %s, Faction: %s)"), 
			*RoleDescription, *DefComp->ProfessionID.ToString(), *DefComp->FactionID.ToString());
	}

	// Inject into Brain immediately (CognitionComponent is on Controller!)
	if (UCognitionComponent* Cognition = AINPCHelpers::GetCognitionComponent(NPC))

	{
		// ✅ Get Base Personality Role (to avoid overwriting it)
		FString BaseRole = "";
		if (UPersonalityComponent* PersComp = AINPCHelpers::GetPersonalityComponent(NPC))

		{
			BaseRole = PersComp->Personality.RoleDescription;
		}

		// ✅ Combine Additively: "[Base]. Scene Role: [Role]. Context: [Plot]"
		FString CombinedRole;
		if (!BaseRole.IsEmpty() && !BaseRole.Equals(TEXT("You are a neutral NPC")))
		{
			CombinedRole = FString::Printf(TEXT("%s\n\n[Scene Role: %s]\n[Scene Context: %s]"), 
				*BaseRole, *RoleDescription, *Squad.PlotOutline);
		}
		else
		{
			CombinedRole = FString::Printf(TEXT("Role: %s. Scene Context: %s"), 
				*RoleDescription, *Squad.PlotOutline);
		}

		Cognition->RoleDescription = CombinedRole;
		NARRATIVE_LOG(Warning, TEXT("[NarrativeSquad] Updated RoleDescription for %s: %s"), *NPC->GetName(), *CombinedRole);
		
		// Note: No ProcessStimulus here - just update the role silently
		// The role will be used next time CognitionComponent::ProcessStimulus is called
	}
}

FString UNarrativeSquadSubsystem::GetMemberContext(AActor* NPC) const
{
	if (const int32* SquadIDPtr = ActorSquadMap.Find(NPC))
	{
		if (const FNarrativeSceneSquad* Squad = ActiveSquads.Find(*SquadIDPtr))
		{
			if (const FName* Role = Squad->MemberRoles.Find(NPC))
			{
				return FString::Printf(TEXT("Plot: %s. Role: %s."), 
					*Squad->PlotOutline, *Role->ToString());
			}
		}
	}
	return TEXT("");
}

void UNarrativeSquadSubsystem::AssignRolesToArea(int32 SquadID, FVector Origin, float Radius, FString RoleDescription)
{
	if (!ActiveSquads.Contains(SquadID)) return;
	
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	
	// Scan for Pawns/Characters
	World->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Pawn, Sphere);

	for (const FOverlapResult& Hit : Overlaps)
	{
		AActor* Actor = Hit.GetActor();
		// Use helper to check Controller for CognitionComponent
		if (Actor && AINPCHelpers::GetCognitionComponent(Actor))

		{
			AssignMemberRole(SquadID, Actor, RoleDescription);
		}
	}
}

void UNarrativeSquadSubsystem::OnNarrativeEventRecorded(const FNarrativeEvent& Event)
{
	NARRATIVE_LOG(Error, TEXT("🛑 [NarrativeSquad] w '%s'. Tags: %d"), *Event.Description, Event.Tags.Num());
	
	// Removed CleanupInvalidActors() here as it prevents processing Death events for PendingKill actors.
	// CleanupInvalidActors();
	
	TArray<int32> SquadsToEnd;

	for (auto& Elem : ActiveSquads)
	{
		FNarrativeSceneSquad& Squad = Elem.Value;
		if (!Squad.bIsActive) continue;

		// =========================================================================================
		// EXTRACT CONTEXT FROM EVENT (Dead Actor details)
		// =========================================================================================
		// We try to find if this event is about a member of THIS squad.

		FString DeadMemberName = "";
		FString DeadMemberRole = "";
		FString DeadMemberProfession = "";
		AActor* DeadMemberActor = nullptr;

		// ✅ FIX: Extract victim name from Event.Description instead of non-existent Death_ tags.
		// RecordNPCDeath format: "<SmartName> was killed by <Killer>"
		// The old code looked for "Death_<Name>" tags, but RecordNPCDeath never adds them.
		bool bIsDeathEvent = false;
		for (const FGameplayTag& T : Event.Tags)
		{
			if (T.MatchesTag(AINPCTags::Event_Death))
			{
				bIsDeathEvent = true;
				break;
			}
		}

		if (bIsDeathEvent)
		{
			// Extract victim name from description: "<Name> was killed by <Killer>"
			int32 KilledByIdx = Event.Description.Find(TEXT(" was killed by"), ESearchCase::IgnoreCase);
			FString VictimName = (KilledByIdx != INDEX_NONE)
				? Event.Description.Left(KilledByIdx)
				: Event.Description;

			// Match against squad members by SmartActorName
			for (const auto& MemberPair : Squad.MemberRoles)
			{
				if (!MemberPair.Key) continue;

				FString MemberSmartName = AINPCHelpers::GetSmartActorName(MemberPair.Key);
				NARRATIVE_LOG(Warning, TEXT("   - Inspecting Squad Member: '%s' (Smart: %s) (Role: %s) vs Victim: '%s'"),
					*MemberPair.Key->GetName(), *MemberSmartName, *MemberPair.Value.ToString(), *VictimName);

				if (MemberSmartName == VictimName)
				{
					DeadMemberActor = MemberPair.Key;
					DeadMemberName = VictimName;
					DeadMemberRole = MemberPair.Value.ToString();

					if (UNPCDefinitionComponent* Def = DeadMemberActor->FindComponentByClass<UNPCDefinitionComponent>())
					{
						if (!Def->ProfessionID.IsNone())
						{
							DeadMemberProfession = Def->ProfessionID.ToString();
						}
					}

					NARRATIVE_LOG(Warning, TEXT("   ✅ Found dead squad member! Name: %s, Role: %s"), *DeadMemberName, *DeadMemberRole);
					break;
				}
			}
		}

		// Helper matcher lambda
		auto MatchesEvent = [&](const FNarrativeEventMatcher& Matcher) -> bool
		{
			if (!Matcher.IsValid()) return false;

			// 1. Tag Match (Must contain the specific tag, e.g. Event.Death)
			bool bTagFound = false;
			for (const FGameplayTag& T : Event.Tags)
			{
				if (T.GetTagName() == Matcher.Tag.GetTagName())
				{
					bTagFound = true;
					break;
				}
			}
			if (!bTagFound) return false;

			// 2. Payload Match (Optional)
			if (Matcher.Payload.IsEmpty())
			{
				return true; // Tag match is enough if no payload specified
			}

			// 3. Payload Check Logic
			// Check against Role, Profession, Name
			if (DeadMemberActor)
			{
				if (DeadMemberRole.Contains(Matcher.Payload)) return true;
				if (DeadMemberProfession.Contains(Matcher.Payload)) return true;
				if (DeadMemberName.Contains(Matcher.Payload)) return true;
				
				// Special case: Exact match against Role string (e.g. "Tyrant Lord")
				if (DeadMemberRole == Matcher.Payload) return true;
			}
			
			return false;
		};

		// =========================================================================================
		// 1. TIMELINE TRIGGERS CHECK
		// =========================================================================================
		
		TArray<int32> TriggeredNodes;
		for (const TPair<int32, FGameplayTag>& Pending : Squad.PendingEventTriggers)
		{
			// Current Runtime Map only supports Tags. We need to look up the static definition for Payload.
			if (Squad.SceneTimeline.IsValidIndex(Pending.Key))
			{
				const FNarrativeTimelineEntry& Node = Squad.SceneTimeline[Pending.Key];
				// Use the static Trigger definition which has the Payload
				if (MatchesEvent(Node.Trigger))
				{
					NARRATIVE_LOG(Warning, TEXT("📜 Event Trigger Matched! Node %d triggered by %s (Payload: %s)"), 
						Pending.Key, *Node.Trigger.Tag.ToString(), *Node.Trigger.Payload);
					TriggeredNodes.Add(Pending.Key);
				}
			}
		}

		// Execute triggered timeline nodes
		for (int32 NodeIndex : TriggeredNodes)
		{
			TriggerTimelineNode(Squad.SquadID, NodeIndex);
			Squad.PendingEventTriggers.Remove(NodeIndex);
			
			if (NodeIndex == Squad.CurrentTimelineIndex)
			{
				Squad.CurrentTimelineIndex++;
			}
		}

		// =========================================================================================
		// 2. SCENE COMPLETION CHECK
		// =========================================================================================
		
		for (const FNarrativeEventMatcher& Condition : Squad.CompletionConditions)
		{
			if (MatchesEvent(Condition))
			{
				NARRATIVE_LOG(Warning, TEXT("[NarrativeSquad] 🎬 SCENE COMPLETE! Squad %d Ending. Trigger: %s (Payload: %s)"),
					Squad.SquadID, *Condition.Tag.ToString(), *Condition.Payload);
				SquadsToEnd.Add(Squad.SquadID);
				break;
			}
		}
	}

	for (int32 ID : SquadsToEnd)
	{
		EndScene(ID);
	}
}

int32 UNarrativeSquadSubsystem::SpawnSceneFromTemplate(UDataTable* SceneTable, FName TemplateID, FTransform Origin, UDataTable* NPCTable, bool bAutoActivate)
{
	if (!SceneTable || !NPCTable || !GetWorld()) return -1;

	static const FString Context = TEXT("NarrativeSquad_SpawnScene");

	// 1. Get Scene Definition
	const FNarrativeSceneDef* SceneDef = SceneTable->FindRow<FNarrativeSceneDef>(TemplateID, Context);
	if (!SceneDef)
	{
		NARRATIVE_LOG(Warning, TEXT("SpawnSceneFromTemplate: Failed to find scene '%s'"), *TemplateID.ToString());
		return -1;
	}

	// 3. Create Squad
	int32 SquadID = CreateSceneSquad(SceneDef->PlotOutline, SceneDef->CompletionConditions);
	if (SquadID == -1) return -1;

	// Copy Scene Definition Data to Squad
	if (FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID))
	{
		// ✅ Copy Timeline and Config from SceneDef to runtime Squad
		Squad->SceneTimeline = SceneDef->Timeline;
		Squad->bKeepPropsOnEnd = SceneDef->bKeepPropsOnEnd;
		Squad->PostSceneStimulus = SceneDef->PostSceneStimulus;
		Squad->PostSceneProfessionID = SceneDef->PostSceneProfessionID;
		Squad->PostSceneProfessionPool = SceneDef->PostSceneProfessionPool;
		
		// Note: bIsActive is NOT set here. relying on ActivateScene() later.
	}

	// 3. Spawn Props FIRST (so SmartObjects are registered before NPCs start searching)
	for (const FScenePropDef& PropDef : SceneDef->Props)
	{
		if (PropDef.PropClass)
		{
			FVector SpawnLoc = Origin.TransformPosition(PropDef.RelativeTransform.GetLocation());
			FRotator SpawnRot = Origin.GetRotation().Rotator() + PropDef.RelativeTransform.GetRotation().Rotator();
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			
			AActor* NewProp = GetWorld()->SpawnActor<AActor>(PropDef.PropClass, SpawnLoc, SpawnRot, SpawnParams);
			if (NewProp)
			{
				if (FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID))
				{
					Squad->SpawnedProps.Add(NewProp);
				}
			}
		}
	}

	// 4. Spawn Cast AFTER Props (with small delay to ensure SmartObjects are registered)
	FTimerHandle SpawnNPCsTimer;
	GetWorld()->GetTimerManager().SetTimer(SpawnNPCsTimer, [this, SceneDef, NPCTable, Origin, SquadID, bAutoActivate]()
	{
		static const FString NPCContext = TEXT("NarrativeSquad_SpawnNPCs");
		
		for (const FSceneRoleDef& Role : SceneDef->Cast)
		{
			const FNPCDefinitionRow* NPCDef = NPCTable->FindRow<FNPCDefinitionRow>(Role.NPCTemplateID, NPCContext);
			
			if (!NPCDef) continue;
			if (!NPCDef->PawnClass) continue;

			FVector SpawnLoc = Origin.TransformPosition(Role.RelativeTransform.GetLocation());
			FRotator SpawnRot = Origin.GetRotation().Rotator() + Role.RelativeTransform.GetRotation().Rotator();

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			APawn* NewPawn = GetWorld()->SpawnActorDeferred<APawn>(NPCDef->PawnClass, FTransform(SpawnRot, SpawnLoc), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (NewPawn)
			{
				if (UNPCDefinitionComponent* DefComp = NewPawn->FindComponentByClass<UNPCDefinitionComponent>())
				{
					DefComp->DefinitionTemplateID = Role.NPCTemplateID;
					DefComp->DefinitionTable = NPCTable;
					DefComp->LoadFromTemplate(); 
				}

				// ✅ Register in Squad BEFORE BeginPlay runs (prevents initial event leakage)
				AssignMemberRole(SquadID, NewPawn, Role.RoleOverride);

				// Now finish spawning (triggering BeginPlay -> AI Init)
				UGameplayStatics::FinishSpawningActor(NewPawn, FTransform(SpawnRot, SpawnLoc));
			}
		}
		
		NARRATIVE_LOG(Log, TEXT("NarrativeSquad: Delayed NPC spawn completed for Squad %d"), SquadID);
		
		// ✅ Activate FULL SCENE logic if requested (ensures Timers & Suppression are set correctly)
		if (bAutoActivate)
		{
			ActivateScene(SquadID);
		}
		
	}, 0.1f, false); // 100ms delay

	return SquadID;
}

void UNarrativeSquadSubsystem::RegisterAnchor(ANarrativeSceneAnchor* Anchor)
{
	if (Anchor && !RegisteredAnchors.Contains(Anchor))
	{
		RegisteredAnchors.Add(Anchor);
	}
}

void UNarrativeSquadSubsystem::UnregisterAnchor(ANarrativeSceneAnchor* Anchor)
{
	RegisteredAnchors.Remove(Anchor);
}

int32 UNarrativeSquadSubsystem::StartSceneGlobal(UDataTable* SceneTable, FName TemplateID, UDataTable* NPCTable)
{
    // Deprecated or redirect to generic logic if needed. Removing for clarity as we move to Anchor-based logic.
    return -1;
}

int32 UNarrativeSquadSubsystem::SpawnSceneAtAnchor(ANarrativeSceneAnchor* Anchor, UDataTable* SceneTable, FName TemplateID, UDataTable* NPCTable, bool bAutoActivate)
{
	if (!Anchor || Anchor->bIsOccupied) return -1;
	// SceneTable/NPCTable null checks handled in SpawnScene... but good to check early
	if (!SceneTable || !NPCTable) return -1;

	// Verify Compatibility (if TemplateID is provided)
	if (!TemplateID.IsNone() && !Anchor->SupportedSceneID.IsNone() && Anchor->SupportedSceneID != TemplateID)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartSceneAtAnchor: Anchor '%s' does not support scene '%s'"), *Anchor->GetName(), *TemplateID.ToString());
		return -1;
	}

	// Auto-Select Scene ID if None passed
	FName SelectedID = TemplateID;
	if (SelectedID.IsNone())
	{
		// First try Anchor's supported ID
		if (!Anchor->SupportedSceneID.IsNone())
		{
			SelectedID = Anchor->SupportedSceneID;
		}
		else
		{
			// Random
			TArray<FName> RowNames = SceneTable->GetRowNames();
			if (RowNames.Num() > 0)
			{
				int32 RandIdx = FMath::RandRange(0, RowNames.Num() - 1);
				SelectedID = RowNames[RandIdx];
			}
			else
			{
				return -1;
			}
		}
	}

	// Spawn
	int32 SquadID = SpawnSceneFromTemplate(SceneTable, SelectedID, Anchor->GetActorTransform(), NPCTable, bAutoActivate);
	
	if (SquadID != -1)
	{
		Anchor->bIsOccupied = true;
		if (FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID))
		{
			Squad->AssignedAnchor = Anchor;
		}
		UE_LOG(LogTemp, Log, TEXT("StartSceneAtAnchor: Started scene '%s' at specific anchor '%s'"), *SelectedID.ToString(), *Anchor->GetName());
	}
	
	return SquadID;
}

void UNarrativeSquadSubsystem::ActivateScene(int32 SquadID)
{
	NARRATIVE_LOG(Warning, TEXT("🔧 ActivateScene called for SquadID: %d"), SquadID);
	
	if (FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID))
	{
		NARRATIVE_LOG(Warning, TEXT("✅ Found Squad %d in ActiveSquads"), SquadID);
		NARRATIVE_LOG(Warning, TEXT("   - bIsActive: %s"), Squad->bIsActive ? TEXT("true") : TEXT("false"));
		
		if (!Squad->bIsActive)
		{
			NARRATIVE_LOG(Warning, TEXT("🎬 Activating Squad %d..."), SquadID);
			Squad->bIsActive = true;

			// ✅ Initialize Timeline System
			Squad->CurrentTimelineIndex = 0;
			Squad->AccumulatedSceneTime = 0.0f;
			Squad->PendingEventTriggers.Empty();

			// ✅ DEFAULT: Suppress action observation until Timeline explicitly enables it
			// 默认：禁止动作观察，直到 Timeline 明确启用
			Squad->bCurrentlySuppressingActionObservation = true;
			NARRATIVE_LOG(Warning, TEXT("   🔇 Action Observation SUPPRESSED by default (can be enabled by Timeline)"));

			// ✅ CRITICAL: Process TimeOffset=0 nodes IMMEDIATELY before NPCs start observing
			// 关键：在 NPC 开始互相观察之前，立即处理 TimeOffset=0 的节点
			if (Squad->SceneTimeline.Num() > 0)
			{
				while (Squad->CurrentTimelineIndex < Squad->SceneTimeline.Num())
				{
					const FNarrativeTimelineEntry& Node = Squad->SceneTimeline[Squad->CurrentTimelineIndex];

					// Only process nodes with TimeOffset=0 and no event trigger
					if (Node.TimeOffset == 0.0f && !Node.Trigger.IsValid())
					{
						NARRATIVE_LOG(Warning, TEXT("📜 ⚡ Immediately triggering Timeline Node %d (T=0)"), Squad->CurrentTimelineIndex);
						TriggerTimelineNode(SquadID, Squad->CurrentTimelineIndex);
						Squad->CurrentTimelineIndex++;
					}
					else
					{
						break; // Stop when we hit a non-zero offset or event-triggered node
					}
				}
			}

			// Start Timeline Tick (1 second interval for checking nodes)
			if (Squad->SceneTimeline.Num() > 0 && Squad->CurrentTimelineIndex < Squad->SceneTimeline.Num())
			{
				UWorld* World = GetWorld();
				if (World)
				{
					World->GetTimerManager().SetTimer(
						Squad->TimelineTickTimer,
						[this, SquadID]() { TickTimeline(SquadID); },
						1.0f, // Tick every 1 second
						true  // Loop
					);
					NARRATIVE_LOG(Warning, TEXT("📜 Timeline System Started for Squad %d (%d nodes, starting from index %d)"),
						SquadID, Squad->SceneTimeline.Num(), Squad->CurrentTimelineIndex);
				}
			}
			
			NARRATIVE_LOG(Warning, TEXT("ActivateScene: Squad %d Activated."), SquadID);
		}
		else
		{
			NARRATIVE_LOG(Warning, TEXT("⚠️ Squad %d is already active, skipping activation"), SquadID);
		}
	}
	else
	{
		AINPC_LOG_ERROR(TEXT("❌ ActivateScene: Squad %d not found in ActiveSquads!"), SquadID);
	}
}

void UNarrativeSquadSubsystem::EndScene(int32 SquadID)
{
	if (!ActiveSquads.Contains(SquadID)) return;

	FNarrativeSceneSquad& Squad = ActiveSquads[SquadID];
	Squad.bIsActive = false;
	
	// Release Anchor
	if (Squad.AssignedAnchor)
	{
		Squad.AssignedAnchor->bIsOccupied = false;
		Squad.AssignedAnchor = nullptr;
	}

	// Destroy Props (unless configured to keep them)
	if (!Squad.bKeepPropsOnEnd)
	{
		for (AActor* Prop : Squad.SpawnedProps)
		{
			if (IsValid(Prop))
			{
				Prop->Destroy();
			}
		}
		Squad.SpawnedProps.Empty();
	}

	// Reset roles for all members
	for (auto& Elem : Squad.MemberRoles)
	{
		AActor* Member = Elem.Key;
		ActorSquadMap.Remove(Member);
		
		if (Member)
		{
			// Remove Scene Tag and Directives
			if (UGoalComponent* GoalComp = AINPCHelpers::GetGoalComponent(Member))
			{
				GoalComp->RemoveContextTag(AINPCTags::Status_InScene);
				GoalComp->RemoveContextTag(AINPCTags::Directive_Combat);
			}
			
			// ✅ Robust Tag Removal
			bool bHadTag = Member->ActorHasTag(FName("Status.InScene"));
			Member->Tags.Remove(FName("Status.InScene")); 
			
			if (bHadTag)
			{
				NARRATIVE_LOG(Log, TEXT("🧹 [EndScene] Removed 'Status.InScene' tag from %s"), *Member->GetName());
			}

			// ✅ Profession Transition (Job Promotion)
			// Calculate New Profession ONCE per NPC to ensure consistency
			FName TargetProfession = Squad.PostSceneProfessionID;
			if (Squad.PostSceneProfessionPool.Num() > 0)
			{
				int32 RandIdx = FMath::RandRange(0, Squad.PostSceneProfessionPool.Num() - 1);
				TargetProfession = Squad.PostSceneProfessionPool[RandIdx];
			}

			if (!TargetProfession.IsNone())
			{
				// Apply to Goal (Behavior/Schedule)
				if (UGoalComponent* GoalComp = AINPCHelpers::GetGoalComponent(Member))
				{
					GoalComp->InitializeProfession(TargetProfession);
					
					// ✅ CRITICAL: After profession change, immediately refresh schedule and arbitration
					// This ensures the NPC resumes schedule-based behavior (Work/Rest/etc.) 
					// instead of staying in a confused Idle state.
					GoalComp->CheckSchedule();      // Update CachedScheduleDirective based on new profession
					GoalComp->UpdateArbitration();  // Apply the correct directive
					
					NARRATIVE_LOG(Warning, TEXT("🎓 NPC %s: Profession updated to %s, Schedule refreshed"), 
						*Member->GetName(), *TargetProfession.ToString());
				}
				else
				{
					NARRATIVE_LOG(Error, TEXT("❌ NPC %s: Could not find GoalComponent!"), *Member->GetName());
				}

				// Apply to Definition (Identity/Persistence)
				if (UNPCDefinitionComponent* DefComp = AINPCHelpers::GetNPCDefinitionComponent(Member))
				{
					DefComp->ProfessionID = TargetProfession;
					NARRATIVE_LOG(Warning, TEXT("🎓 NPC %s: Definition profession updated to %s"), *Member->GetName(), *TargetProfession.ToString());
					
					// ✅ Update Display (Nameplate)
					// Refresh the visual nameplate to show the new profession
					if (UEmotionDisplayComponent* DisplayComp = Member->FindComponentByClass<UEmotionDisplayComponent>())
					{
						DisplayComp->UpdateNameplate(
							DefComp->PersonalityID.ToString(),
							DefComp->PersonalityID.ToString(),
							TargetProfession.ToString(),
							DefComp->FactionID.ToString()
						);
						NARRATIVE_LOG(Warning, TEXT("🎓 NPC %s: Nameplate display refreshed"), *Member->GetName());
					}
				}
				else
				{
					NARRATIVE_LOG(Error, TEXT("❌ NPC %s: Could not find NPCDefinitionComponent!"), *Member->GetName());
				}
			}

			// Reset Cognition
			if (UCognitionComponent* Cognition = AINPCHelpers::GetCognitionComponent(Member))
			{
				Cognition->RoleDescription = TEXT(""); 
			
				FString FinalStimulus = Squad.PostSceneStimulus.IsEmpty() 
					? TEXT("The scene has ended. I return to my daily routine.") 
					: Squad.PostSceneStimulus;
			
				Cognition->ProcessStimulus(FinalStimulus);
			}
		}
	}
	
	ActiveSquads.Remove(SquadID);
	NARRATIVE_LOG(Log, TEXT("Scene Squad %d Ended and Disbanded."), SquadID);
}
AActor* UNarrativeSquadSubsystem::FindActorByRole(const FNarrativeSceneSquad& Squad, FName RoleName) const
{
	for (const auto& Pair : Squad.MemberRoles)
	{
		if (IsValid(Pair.Key) && Pair.Value == RoleName)
		{
			return Pair.Key;
		}
	}
	return nullptr;
}

// ============================================================================
// TIMELINE SYSTEM
// ============================================================================

void UNarrativeSquadSubsystem::TickTimeline(int32 SquadID)
{
	FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID);
	if (!Squad || !Squad->bIsActive) return;

	// Update accumulated time
	Squad->AccumulatedSceneTime += 1.0f;

	// Process timeline nodes
	while (Squad->CurrentTimelineIndex < Squad->SceneTimeline.Num())
	{
		const FNarrativeTimelineEntry& Node = Squad->SceneTimeline[Squad->CurrentTimelineIndex];

		// Check if time condition is met
		if (Squad->AccumulatedSceneTime < Node.TimeOffset)
		{
			break; // Time not reached yet
		}

		// Check if this node has an event trigger
		if (Node.Trigger.IsValid())
		{
			// ✅ STATE CHECK: Don't wait for event if the state is ALREADY true
			// This prevents race conditions where the player is already inside the trigger when the node activates
			if (CheckStateCondition(SquadID, Squad->CurrentTimelineIndex, Node.Trigger))
			{
				NARRATIVE_LOG(Warning, TEXT("📜 ⚡ State Condition Met Immediately: %s"), *Node.Trigger.Tag.ToString());
				TriggerTimelineNode(SquadID, Squad->CurrentTimelineIndex);
				Squad->CurrentTimelineIndex++;
				continue; // Continue to next node immediately
			}

			// Add to pending triggers (wait for event)
			if (!Squad->PendingEventTriggers.Contains(Squad->CurrentTimelineIndex))
			{
				Squad->PendingEventTriggers.Add(Squad->CurrentTimelineIndex, Node.Trigger.Tag);
				NARRATIVE_LOG(Warning, TEXT("📜 Timeline Node %d (T+%.1fs): Waiting for event %s (Payload: %s)"), 
					Squad->CurrentTimelineIndex, Node.TimeOffset, *Node.Trigger.Tag.ToString(), *Node.Trigger.Payload);
			}
			break; // Stop processing, wait for event
		}
		else
		{
			// Time-only trigger, execute immediately
			TriggerTimelineNode(SquadID, Squad->CurrentTimelineIndex);
			Squad->CurrentTimelineIndex++;
		}
	}

	// Stop timeline if all nodes processed
	if (Squad->CurrentTimelineIndex >= Squad->SceneTimeline.Num() && Squad->PendingEventTriggers.Num() == 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(Squad->TimelineTickTimer);
			NARRATIVE_LOG(Warning, TEXT("📜 Timeline Completed for Squad %d"), SquadID);
		}
	}
}

bool UNarrativeSquadSubsystem::CheckStateCondition(int32 SquadID, int32 NodeIndex, const FNarrativeEventMatcher& Matcher)
{
	FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID);
	if (!Squad) return false;

	// Case 1: Player Detected (Overlap Check)
	if (Matcher.Tag.MatchesTag(FGameplayTag::RequestGameplayTag("Event.PlayerDetected")))
	{
		// If we have an anchor with an EventTrigger, check if player is currently overlapping it
		if (Squad->AssignedAnchor && Squad->AssignedAnchor->EventTrigger)
		{
			if (UWorld* World = GetWorld())
			{
				if (APlayerController* PC = World->GetFirstPlayerController())
				{
					if (APawn* PlayerPawn = PC->GetPawn())
					{
						bool bOverlapping = Squad->AssignedAnchor->EventTrigger->IsOverlappingActor(PlayerPawn);
						if (bOverlapping)
						{
							NARRATIVE_LOG(Log, TEXT("   ✅ CheckStateCondition: Player is inside EventTrigger (Squad %d)"), SquadID);
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

void UNarrativeSquadSubsystem::TriggerTimelineNode(int32 SquadID, int32 NodeIndex)
{
	FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID);
	if (!Squad || NodeIndex >= Squad->SceneTimeline.Num()) return;

	const FNarrativeTimelineEntry& Node = Squad->SceneTimeline[NodeIndex];

	NARRATIVE_LOG(Warning, TEXT("📜 ⚡ Timeline Node %d Triggered (T+%.1fs): %s"),
		NodeIndex, Squad->AccumulatedSceneTime, *Node.PlotUpdate);

	// ✅ Update Action Observation Suppression Flag
	Squad->bCurrentlySuppressingActionObservation = Node.bSuppressActionObservation;
	if (Node.bSuppressActionObservation)
	{
		NARRATIVE_LOG(Warning, TEXT("   🔇 Action Observation SUPPRESSED for this timeline node"));
	}

	// Update PlotOutline (context for LLM)
	if (!Node.PlotUpdate.IsEmpty())
	{
		Squad->PlotOutline = Node.PlotUpdate;
		
		// Refresh context for all squad members (same logic as AssignMemberRole)
		for (const TPair<AActor*, FName>& MemberPair : Squad->MemberRoles)
		{
			// ✅ Safety: Ensure NPC is valid before accessing components
			if (!IsValid(MemberPair.Key))
			{
				NARRATIVE_LOG(Warning, TEXT("   ⚠️ Skipping invalid NPC in MemberRoles during PlotUpdate"));
				continue;
			}
			
			if (auto CogComp = AINPCHelpers::GetCognitionComponent(MemberPair.Key))
			{
				// Build combined role description (like AssignMemberRole does)
				FString BaseRole;
				if (auto PersComp = AINPCHelpers::GetPersonalityComponent(MemberPair.Key))
				{
					BaseRole = PersComp->Personality.RoleDescription;
				}

				FString CombinedRole;
				if (!BaseRole.IsEmpty() && !BaseRole.Equals(TEXT("You are a neutral NPC")))
				{
					CombinedRole = FString::Printf(TEXT("%s\n\n[Scene Role: %s]\n[Scene Context: %s]"), 
						*BaseRole, *MemberPair.Value.ToString(), *Squad->PlotOutline);
				}
				else
				{
					CombinedRole = FString::Printf(TEXT("Role: %s. Scene Context: %s"), 
						*MemberPair.Value.ToString(), *Squad->PlotOutline);
				}

				// ✅ Directly set RoleDescription field (no UpdateRoleDescription method exists)
				CogComp->RoleDescription = CombinedRole;
				NARRATIVE_LOG(Warning, TEXT("   → Updated Role for %s"), *MemberPair.Key->GetName());
			}
		}
	}

	// Update Directive (behavior override)
	if (Node.DirectiveOverride.IsValid())
	{
		for (const TPair<AActor*, FName>& MemberPair : Squad->MemberRoles)
		{
			AActor* NPC = MemberPair.Key;
			
			// ✅ Safety: Ensure NPC is valid before accessing
			if (!IsValid(NPC))
			{
				NARRATIVE_LOG(Warning, TEXT("   ⚠️ Skipping invalid NPC in MemberRoles during DirectiveOverride"));
				continue;
			}
			
			// 1. Apply to GoalComponent (for Utility AI)
			if (auto GoalComp = AINPCHelpers::GetGoalComponent(NPC))
			{
				GoalComp->AddContextTag(Node.DirectiveOverride);
			}
			
			// 2. Apply to Actor Tags (for FactionReputationComponent::EvaluateCombatPolicy)
			FName TagName = Node.DirectiveOverride.GetTagName();
			NPC->Tags.AddUnique(TagName);
			
			NARRATIVE_LOG(Warning, TEXT("   → Added Directive Tag for %s: %s (both GameplayTag and Actor Tag)"), 
				*NPC->GetName(), *Node.DirectiveOverride.ToString());
		}
	}

	// --- Forced Speaker ---
	if (!Node.ForcedSpeakerRole.IsNone() && !Node.ForcedSpeakerPrompt.IsEmpty())
	{
		AActor* Speaker = FindActorByRole(*Squad, Node.ForcedSpeakerRole);
		if (Speaker)
		{
			if (auto CogComp = AINPCHelpers::GetCognitionComponent(Speaker))
			{
				CogComp->ProcessStimulus(Node.ForcedSpeakerPrompt, true, 1.0f);
				NARRATIVE_LOG(Warning, TEXT("   -> Forced Speaker '%s' (Role: %s) to speak"),
					*Speaker->GetName(), *Node.ForcedSpeakerRole.ToString());
			}
		}
		else
		{
			NARRATIVE_LOG(Warning, TEXT("   -> Could not find actor with role '%s' for forced speech"),
				*Node.ForcedSpeakerRole.ToString());
		}
	}
	// --- Mass Reaction ---
	else if (Node.MassReactionPercentage > 0.0f)
	{
		TArray<AActor*> Candidates;
		Squad->MemberRoles.GetKeys(Candidates);
		int32 Count = FMath::Max(1, FMath::CeilToInt(Candidates.Num() * Node.MassReactionPercentage));

		// Shuffle and take first Count
		for (int32 i = 0; i < FMath::Min(Count, Candidates.Num()); i++)
		{
			int32 Rand = FMath::RandRange(i, Candidates.Num() - 1);
			Candidates.Swap(i, Rand);
			if (IsValid(Candidates[i]))
			{
				if (auto CogComp = AINPCHelpers::GetCognitionComponent(Candidates[i]))
				{
					CogComp->ProcessStimulus(Node.PlotUpdate, true, 1.0f);
				}
			}
		}
		NARRATIVE_LOG(Warning, TEXT("   -> Mass Reaction: %d/%d members triggered (%.0f%%)"),
			FMath::Min(Count, Candidates.Num()), Candidates.Num(), Node.MassReactionPercentage * 100.0f);
	}
}

bool UNarrativeSquadSubsystem::GetSquadMembers(const AActor* ContextActor, TArray<AActor*>& OutMembers) const
{
	OutMembers.Empty();
	if (!ContextActor) return false;

	// 1. Find which squad this actor belongs to
	const int32* SquadIDPtr = ActorSquadMap.Find(ContextActor);
	if (!SquadIDPtr) return false;

	int32 SquadID = *SquadIDPtr;

	// 2. Get the squad
	const FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID);
	if (!Squad || !Squad->bIsActive) return false;

	// 3. Extract all members from MemberRoles map key
	Squad->MemberRoles.GetKeys(OutMembers);

	// Remove self
	OutMembers.Remove(const_cast<AActor*>(ContextActor));

	return OutMembers.Num() > 0;
}

bool UNarrativeSquadSubsystem::ShouldSuppressActionObservation(const AActor* ObservedActor) const
{
	if (!ObservedActor) return false;

	// 1. Find which squad this actor belongs to
	const int32* SquadIDPtr = ActorSquadMap.Find(ObservedActor);
	if (!SquadIDPtr) return false;

	int32 SquadID = *SquadIDPtr;

	// 2. Get the squad
	const FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID);
	if (!Squad) return false;

	// ✅ CRITICAL FIX: Check suppression EVEN DURING SPAWN (bIsActive=false)
	// 关键修复：即使在 spawn 阶段（bIsActive=false）也检查 suppression
	// This prevents NPCs from observing each other during the spawn sequence
	// 这可以防止 NPC 在 spawn 过程中互相观察

	// 3. Return the current suppression state
	return Squad->bCurrentlySuppressingActionObservation;
}


void UNarrativeSquadSubsystem::ApplyTagToRole(int32 SquadID, FString RoleID, FGameplayTag Tag)
{
	if (!Tag.IsValid())
	{
		NARRATIVE_LOG(Warning, TEXT("ApplyTagToRole called with invalid tag for Role %s"), *RoleID);
		return;
	}

	if (FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID))
	{
		int32 Count = 0;
		for (auto& Pair : Squad->MemberRoles)
		{
			AActor* NPC = Pair.Key;
			FString NPCRole = Pair.Value.ToString(); 

			// Check if Role matches (Case insensitive)
			if (NPCRole.Equals(RoleID, ESearchCase::IgnoreCase))
			{
				// ✅ Safety: Ensure NPC is valid before accessing
				if (IsValid(NPC))
				{
					// 1. Apply as FName Tag (for FactionReputationComponent::EvaluateCombatPolicy)
					FName TagName = Tag.GetTagName();
					NPC->Tags.AddUnique(TagName);
					
					// 2. Apply as GameplayTag (for UtilityAI GoalComponent or others)
					if (auto GoalComp = AINPCHelpers::GetGoalComponent(NPC))
					{
						GoalComp->AddContextTag(Tag);
					}

					UE_LOG(LogAINPCSocial, Log, TEXT("[Narrative] Applied Tag '%s' to %s (Role: %s)"), 
						*TagName.ToString(), *NPC->GetName(), *RoleID);
					Count++;
				}
				else
				{
					NARRATIVE_LOG(Warning, TEXT("   ⚠️ Skipping invalid NPC '%s' in ApplyTagToRole"), *NPCRole);
				}
			}
		}
		
		if (Count > 0)
		{
			NARRATIVE_LOG(Log, TEXT("Applied tag %s to %d actors with role %s in Squad %d"), 
				*Tag.ToString(), Count, *RoleID, SquadID);
		}
		else
		{
			NARRATIVE_LOG(Warning, TEXT("ApplyTagToRole: No actors found with role %s in Squad %d"), *RoleID, SquadID);
		}
	}
	else
	{
		NARRATIVE_LOG(Warning, TEXT("ApplyTagToRole: Squad %d not found!"), SquadID);
	}
}

void UNarrativeSquadSubsystem::CleanupInvalidActors()
{
	int32 TotalCleaned = 0;
	
	// Iterate through all squads
	for (auto& SquadPair : ActiveSquads)
	{
		FNarrativeSceneSquad& Squad = SquadPair.Value;
		
		// Find invalid actors in this squad
		TArray<AActor*> InvalidActors;
		for (const auto& MemberPair : Squad.MemberRoles)
		{
			if (!IsValid(MemberPair.Key))
			{
				InvalidActors.Add(MemberPair.Key);
			}
		}
		
		// Remove invalid actors from the squad and reverse map
		for (AActor* InvalidActor : InvalidActors)
		{
			Squad.MemberRoles.Remove(InvalidActor);
			ActorSquadMap.Remove(InvalidActor);
			TotalCleaned++;
		}
	}
	
	if (TotalCleaned > 0)
	{
		NARRATIVE_LOG(Warning, TEXT("🧹 CleanupInvalidActors: Removed %d invalid actors from squads"), TotalCleaned);
	}
}
