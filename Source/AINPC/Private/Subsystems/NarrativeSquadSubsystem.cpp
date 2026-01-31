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

	ActiveSquads.Add(NewID, Squad);
	
	NARRATIVE_LOG(Log, TEXT("[NarrativeSquad] Created Squad %d: %s"), NewID, *PlotOutline);
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

		// Check for Death Tags to extract context
		for (const FName& RawTag : Event.Tags)
		{
			FString TagStr = RawTag.ToString();
			if (TagStr.StartsWith("Death_"))
			{
				FString PossibleName = TagStr.RightChop(6); // Remove "Death_" prefix
				
				// Is this actor in our squad?
				for (const auto& MemberPair : Squad.MemberRoles)
				{
					FString MemberSmartName = AINPCHelpers::GetSmartActorName(MemberPair.Key);
					NARRATIVE_LOG(Warning, TEXT("   - Inspecting Squad Member: '%s' (Smart: %s) (Role: %s) vs Event Actor: '%s'"), 
						*MemberPair.Key->GetName(), *MemberSmartName, *MemberPair.Value.ToString(), *PossibleName);

					if (MemberPair.Key && MemberSmartName == PossibleName)
					{
						DeadMemberActor = MemberPair.Key;
						DeadMemberName = PossibleName;
						DeadMemberRole = MemberPair.Value.ToString();
						
						if (UNPCDefinitionComponent* Def = DeadMemberActor->FindComponentByClass<UNPCDefinitionComponent>())
						{
							if (!Def->ProfessionID.IsNone())
							{
								DeadMemberProfession = Def->ProfessionID.ToString();
							}
						}
						break; // Found member
					}
				}
			}
			if (DeadMemberActor) break;
		}

		// Helper matcher lambda
		auto MatchesEvent = [&](const FNarrativeEventMatcher& Matcher) -> bool
		{
			if (!Matcher.IsValid()) return false;

			// 1. Tag Match (Must contain the specific tag, e.g. Event.Death)
			bool bTagFound = false;
			for (const FName& T : Event.Tags)
			{
				if (T == Matcher.Tag.GetTagName())
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

	// 2. Create Squad
	int32 SquadID = CreateSceneSquad(SceneDef->PlotOutline, SceneDef->CompletionConditions);
	if (SquadID == -1) return -1;

	// Set Initial Active State
	if (FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID))
	{
		Squad->bIsActive = bAutoActivate;
		
		// ✅ Copy Timeline from SceneDef to runtime Squad
		Squad->SceneTimeline = SceneDef->Timeline;
		Squad->bKeepPropsOnEnd = SceneDef->bKeepPropsOnEnd;
		Squad->PostSceneStimulus = SceneDef->PostSceneStimulus;
		Squad->PostSceneProfessionID = SceneDef->PostSceneProfessionID; // ✅ Copy Profession Transition ID
		Squad->PostSceneProfessionPool = SceneDef->PostSceneProfessionPool; // ✅ Copy Profession Pool
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
	GetWorld()->GetTimerManager().SetTimer(SpawnNPCsTimer, [this, SceneDef, NPCTable, Origin, SquadID]()
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

			APawn* NewPawn = GetWorld()->SpawnActor<APawn>(NPCDef->PawnClass, SpawnLoc, SpawnRot, SpawnParams);
			if (NewPawn)
			{
				if (UNPCDefinitionComponent* DefComp = NewPawn->FindComponentByClass<UNPCDefinitionComponent>())
				{
					DefComp->DefinitionTemplateID = Role.NPCTemplateID;
					DefComp->DefinitionTable = NPCTable;
					DefComp->LoadFromTemplate(); 
				}


				// ✅ With Lazy Fetch Pattern, we don't need to delay - CognitionComponent will query when needed
				AssignMemberRole(SquadID, NewPawn, Role.RoleOverride);
			}
		}
		
		NARRATIVE_LOG(Log, TEXT("NarrativeSquad: Delayed NPC spawn completed for Squad %d"), SquadID);
		
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
		NARRATIVE_LOG(Warning, TEXT("   - bEnableAmbientDialogue: %s"), Squad->bEnableAmbientDialogue ? TEXT("true") : TEXT("false"));
		
		if (!Squad->bIsActive)
		{
			NARRATIVE_LOG(Warning, TEXT("🎬 Activating Squad %d..."), SquadID);
			Squad->bIsActive = true;
			
			// ✅ Initialize Timeline System
			Squad->CurrentTimelineIndex = 0;
			Squad->AccumulatedSceneTime = 0.0f;
			Squad->PendingEventTriggers.Empty();
			
			// Start Timeline Tick (1 second interval for checking nodes)
			if (Squad->SceneTimeline.Num() > 0)
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
					NARRATIVE_LOG(Warning, TEXT("📜 Timeline System Started for Squad %d (%d nodes)"), SquadID, Squad->SceneTimeline.Num());
				}
			}
			
			// Note: NPCs will speak via Ambient Dialogue system, not all at once
			
			// Start Ambient Dialogue Timer
			if (Squad->bEnableAmbientDialogue)
			{
				NARRATIVE_LOG(Warning, TEXT("🎤 About to call StartAmbientDialogue for Squad %d"), SquadID);
				StartAmbientDialogue(SquadID);
				NARRATIVE_LOG(Warning, TEXT("ActivateScene: Squad %d Activated with Ambient Dialogue enabled."), SquadID);
			}
			else
			{
				NARRATIVE_LOG(Warning, TEXT("ActivateScene: Squad %d Activated (Ambient Dialogue disabled)."), SquadID);
			}
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
	
	// Stop Ambient Dialogue Timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Squad.AmbientDialogueTimer);
	}
	
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
					
					// ✅ Force immediate behavior change
					// Setting a new directive interrupts the current action (e.g., stop mining)
					// Idle directive allows NPC to transition back to normal behavior
					GoalComp->SetDirective(AINPCTags::Directive_Idle);
					
					NARRATIVE_LOG(Warning, TEXT("🎓 NPC %s: Profession updated to %s, Directive forced to Idle"), 
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
// ============================================================================
// AMBIENT DIALOGUE SYSTEM
// ============================================================================

void UNarrativeSquadSubsystem::ConfigureAmbientDialogue(int32 SquadID, bool bEnabled, float MinInterval, float MaxInterval, int32 SpeakersPerTrigger, float ActivationRadius)
{
	FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID);
	if (!Squad) return;

	Squad->bEnableAmbientDialogue = bEnabled;
	Squad->AmbientDialogueIntervalMin = MinInterval;
	Squad->AmbientDialogueIntervalMax = MaxInterval;
	Squad->AmbientSpeakersPerTrigger = SpeakersPerTrigger;
	Squad->PlayerActivationRadius = ActivationRadius;

	UE_LOG(LogTemp, Log, TEXT("[AmbientDialogue] Configured Squad %d: Enabled=%d, Interval=[%.1f-%.1f], Speakers=%d, Radius=%.0f"), 
		SquadID, bEnabled, MinInterval, MaxInterval, SpeakersPerTrigger, ActivationRadius);

	// Restart timer if scene is already active
	if (Squad->bIsActive && bEnabled)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(Squad->AmbientDialogueTimer);
		}
		StartAmbientDialogue(SquadID);
	}
}

void UNarrativeSquadSubsystem::StartAmbientDialogue(int32 SquadID)
{
	NARRATIVE_LOG(Warning, TEXT("[AmbientDialogue] 🔧 StartAmbientDialogue called for Squad %d"), SquadID);
	
	FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID);
	if (!Squad)
	{
		AINPC_LOG_ERROR(TEXT("[AmbientDialogue] ❌ Squad %d not found in ActiveSquads!"), SquadID);
		return;
	}
	
	if (!Squad->bIsActive)
	{
		NARRATIVE_LOG(Warning, TEXT("[AmbientDialogue] ⚠️ Squad %d is not active (bIsActive=false)"), SquadID);
		return;
	}
	
	if (!Squad->bEnableAmbientDialogue)
	{
		NARRATIVE_LOG(Warning, TEXT("[AmbientDialogue] ⚠️ Squad %d has Ambient Dialogue disabled (bEnableAmbientDialogue=false)"), SquadID);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		AINPC_LOG_ERROR(TEXT("[AmbientDialogue] ❌ GetWorld() returned nullptr!"));
		return;
	}

	// Calculate random interval
	float RandomInterval = FMath::RandRange(Squad->AmbientDialogueIntervalMin, Squad->AmbientDialogueIntervalMax);

	// Set timer
	World->GetTimerManager().SetTimer(
		Squad->AmbientDialogueTimer,
		FTimerDelegate::CreateUObject(this, &UNarrativeSquadSubsystem::TriggerAmbientDialogue, SquadID),
		RandomInterval,
		false // One-shot, we'll restart after triggering
	);

	NARRATIVE_LOG(Warning, TEXT("[AmbientDialogue] ✅ Timer set! Squad %d will trigger in %.1f seconds"), SquadID, RandomInterval);
}

void UNarrativeSquadSubsystem::TriggerAmbientDialogue(int32 SquadID)
{
	FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID);
	if (!Squad || !Squad->bIsActive)
	{
		return; // Scene ended, don't restart timer
	}

	// ✅ Check if player is nearby
	if (!IsPlayerNearScene(Squad))
	{
		NARRATIVE_LOG(Warning, TEXT("[AmbientDialogue] ⏭️ Squad %d: Player not nearby, skipping trigger"), SquadID);
		// Restart timer for next check
		StartAmbientDialogue(SquadID);
		return;
	}

	// Collect valid speakers (NPCs with CognitionComponent)
	TArray<AActor*> ValidSpeakers;
	for (const auto& Pair : Squad->MemberRoles)
	{
		if (IsValid(Pair.Key))
		{
			UCognitionComponent* CogComp = AINPCHelpers::GetCognitionComponent(Pair.Key);

			if (CogComp)
			{
				ValidSpeakers.Add(Pair.Key);
			}
		}
	}

	if (ValidSpeakers.Num() == 0)
	{
		NARRATIVE_LOG(Warning, TEXT("[AmbientDialogue] ❌ Squad %d: No valid speakers found"), SquadID);
		StartAmbientDialogue(SquadID);
		return;
	}

	// Randomly select 1-N speakers
	int32 NumSpeakers = FMath::Min(
		FMath::RandRange(1, Squad->AmbientSpeakersPerTrigger), 
		ValidSpeakers.Num()
	);

	NARRATIVE_LOG(Warning, TEXT("[AmbientDialogue] 🎬 Squad %d: Triggering %d/%d speakers (Player nearby)"), 
		SquadID, NumSpeakers, ValidSpeakers.Num());

	for (int32 i = 0; i < NumSpeakers; i++)
	{
		int32 RandomIndex = FMath::RandRange(0, ValidSpeakers.Num() - 1);
		AActor* SelectedSpeaker = ValidSpeakers[RandomIndex];
		ValidSpeakers.RemoveAt(RandomIndex); // Avoid duplicate selection

		// Trigger the NPC to speak
		RequestAmbientDialogue(SelectedSpeaker, Squad);
	}

	// Restart timer for next trigger
	StartAmbientDialogue(SquadID);
}

void UNarrativeSquadSubsystem::RequestAmbientDialogue(AActor* Speaker, const FNarrativeSceneSquad* Squad)
{
	if (!IsValid(Speaker) || !Squad) return;

	UCognitionComponent* CogComp = AINPCHelpers::GetCognitionComponent(Speaker);

	if (!CogComp) return;

	// Get NPC's current activity from UtilityAIComponent (actual executing action, not scheduled)
	FString CurrentActivity = TEXT("idle");
	
	// First try: Get actual executing action's ActivityTag from UtilityAIComponent
	if (AAIController* AIController = Cast<AAIController>(Speaker->GetInstigatorController()))
	{
		if (UUtilityAIComponent* UtilComp = AIController->FindComponentByClass<UUtilityAIComponent>())
		{
			if (UtilComp->CurrentAction && UtilComp->CurrentAction->ActivityTag.IsValid())
			{
				// Extract just the activity name from the tag (e.g., "Interaction.Mine" -> "mining")
				FString TagStr = UtilComp->CurrentAction->ActivityTag.ToString();
				if (TagStr.Contains(TEXT(".")))
				{
					TagStr.Split(TEXT("."), nullptr, &CurrentActivity);
				}
				else
				{
					CurrentActivity = TagStr;
				}
			}
		}
	}
	
	// Fallback: If no action, try scheduled activity from GoalComponent
	if (CurrentActivity == TEXT("idle"))
	{
		if (UGoalComponent* GoalComp = Speaker->FindComponentByClass<UGoalComponent>())
		{
			FGameplayTag ActivityTag = GoalComp->GetScheduledActivity();
			if (ActivityTag.IsValid())
			{
				// Extract just the activity name from the tag (e.g., "Interaction.Mine" -> "mining")
				FString TagStr = ActivityTag.ToString();
				if (TagStr.Contains(TEXT(".")))
				{
					TagStr.Split(TEXT("."), nullptr, &CurrentActivity);
				}
				else
				{
					CurrentActivity = TagStr;
				}
			}
		}
	}

	// Get NPC's emotional state (optional enhancement)
	FString EmotionContext = TEXT("");
	if (UPersonalityComponent* PersComp = AINPCHelpers::GetPersonalityComponent(Speaker))

	{
		// Note: You may need to implement GetCurrentEmotionalState() in PersonalityComponent
		// For now, we'll leave it as a placeholder
		EmotionContext = TEXT("neutral"); // Placeholder
	}

	// Build the ambient dialogue prompt
	FString AmbientPrompt = FString::Printf(
		TEXT("You are currently in this scene: %s\n"
			 "Your role in this scene: %s\n"
			 "Current activity: %s\n\n"
			 "Generate a brief, impactful line (1-2 sentences) that reflects your character's reaction to the immediate PLOT CONFLICT. "
			 "Do NOT engage in idle chitchat or talk about the weather. "
			 "Instead, reveal your stance, your fears, or your determination regarding the events unfolding (e.g. the uprising, the specific danger). "
			 "Your goal is to build tension and reinforce the narrative situation."),
		*Squad->PlotOutline,
		*Squad->MemberRoles.FindRef(Speaker).ToString(),
		*CurrentActivity
	);

	// Trigger CognitionComponent to generate response
	CogComp->ProcessStimulus(AmbientPrompt);

	NARRATIVE_LOG(Warning, TEXT("[AmbientDialogue] 💬 %s triggered ambient speech (Activity: %s)"), 
		*Speaker->GetName(), *CurrentActivity);
	NARRATIVE_LOG(Warning, TEXT("[AmbientDialogue] 🔔 Speech should appear above NPC's head in 1-2 seconds if EmotionDisplayComponent is configured."));
}

bool UNarrativeSquadSubsystem::IsPlayerNearScene(const FNarrativeSceneSquad* Squad) const
{
	if (!Squad) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	// Get player location
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return false;

	APawn* PlayerPawn = PC->GetPawn();
	if (!PlayerPawn) return false;

	FVector PlayerLocation = PlayerPawn->GetActorLocation();

	// Check distance from anchor (if assigned)
	if (Squad->AssignedAnchor)
	{
		float DistanceSq = FVector::DistSquared(PlayerLocation, Squad->AssignedAnchor->GetActorLocation());
		float RadiusSq = Squad->PlayerActivationRadius * Squad->PlayerActivationRadius;
		
		return DistanceSq <= RadiusSq;
	}

	// Fallback: Check distance from any squad member
	for (const auto& Pair : Squad->MemberRoles)
	{
		if (IsValid(Pair.Key))
		{
			float DistanceSq = FVector::DistSquared(PlayerLocation, Pair.Key->GetActorLocation());
			float RadiusSq = Squad->PlayerActivationRadius * Squad->PlayerActivationRadius;
			
			if (DistanceSq <= RadiusSq)
			{
				return true;
			}
		}
	}

	return false;
}

void UNarrativeSquadSubsystem::TriggerAmbientDialogueNow(int32 SquadID)
{
	// Manual trigger for testing - bypasses timer
	TriggerAmbientDialogue(SquadID);
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

void UNarrativeSquadSubsystem::TriggerTimelineNode(int32 SquadID, int32 NodeIndex)
{
	FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID);
	if (!Squad || NodeIndex >= Squad->SceneTimeline.Num()) return;

	const FNarrativeTimelineEntry& Node = Squad->SceneTimeline[NodeIndex];

	NARRATIVE_LOG(Warning, TEXT("📜 ⚡ Timeline Node %d Triggered (T+%.1fs): %s"), 
		NodeIndex, Squad->AccumulatedSceneTime, *Node.PlotUpdate);

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

	// TODO: Trigger Bark if BarkID is set
	// For now, we'll skip bark implementation
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
