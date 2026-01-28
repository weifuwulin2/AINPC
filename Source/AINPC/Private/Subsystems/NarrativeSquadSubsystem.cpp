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


void UNarrativeSquadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Bind to Director
	if (UNarrativeDirectorSubsystem* Director = GetWorld()->GetSubsystem<UNarrativeDirectorSubsystem>())
	{
		Director->OnEventRecorded.AddDynamic(this, &UNarrativeSquadSubsystem::OnNarrativeEventRecorded);
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

int32 UNarrativeSquadSubsystem::CreateSceneSquad(FString PlotOutline, TArray<FName> CompletionTags)
{
	int32 NewID = NextSquadID++;
	FNarrativeSceneSquad Squad;
	Squad.SquadID = NewID;
	Squad.PlotOutline = PlotOutline;
	Squad.CompletionTags = CompletionTags;
	Squad.bIsActive = false;  // Scene waits for player to activate

	ActiveSquads.Add(NewID, Squad);
	
	UE_LOG(LogTemp, Log, TEXT("[NarrativeSquad] Created Squad %d: %s"), NewID, *PlotOutline);
	return NewID;
}

void UNarrativeSquadSubsystem::AssignMemberRole(int32 SquadID, AActor* NPC, FString RoleDescription)
{
	if (!ActiveSquads.Contains(SquadID)) return;
	if (!NPC) return;

	FNarrativeSceneSquad& Squad = ActiveSquads[SquadID];
	Squad.MemberRoles.Add(NPC, RoleDescription);
	ActorSquadMap.Add(NPC, SquadID);
	
	// Mark as In-Scene (Suppresses Hostility)
	if (UGoalComponent* GoalComp = AINPCHelpers::GetGoalComponent(NPC))

	{
		GoalComp->AddContextTag(AINPCTags::Status_InScene);
	}
	NPC->Tags.AddUnique("Status.InScene"); // Legacy / Fallback

	// ✅ DEBUG: Log NPC configuration
	if (UNPCDefinitionComponent* DefComp = NPC->FindComponentByClass<UNPCDefinitionComponent>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NarrativeSquad] Assigned Role '%s' to NPC (Profession: %s, Faction: %s)"), 
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
		UE_LOG(LogTemp, Warning, TEXT("[NarrativeSquad] Updated RoleDescription for %s: %s"), *NPC->GetName(), *CombinedRole);
		
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
			if (const FString* Role = Squad->MemberRoles.Find(NPC))
			{
				return FString::Printf(TEXT("Current Scene: %s. My Role: %s"), *Squad->PlotOutline, **Role);
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
	// Check all squads
	TArray<int32> SquadsToEnd;

	for (auto& Elem : ActiveSquads)
	{
		FNarrativeSceneSquad& Squad = Elem.Value;
		if (!Squad.bIsActive) continue;

		for (const FName& Tag : Event.Tags)
		{
			if (Squad.CompletionTags.Contains(Tag))
			{
				UE_LOG(LogTemp, Log, TEXT("[NarrativeSquad] Squad %d Completed by Event Tag: %s"), Squad.SquadID, *Tag.ToString());
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
		UE_LOG(LogTemp, Warning, TEXT("SpawnSceneFromTemplate: Failed to find scene '%s'"), *TemplateID.ToString());
		return -1;
	}

	// 2. Create Squad
	int32 SquadID = CreateSceneSquad(SceneDef->PlotOutline, SceneDef->CompletionTags);
	if (SquadID == -1) return -1;

	// Set Initial Active State
	if (FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID))
	{
		Squad->bIsActive = bAutoActivate;
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
		
		UE_LOG(LogTemp, Log, TEXT("NarrativeSquad: Delayed NPC spawn completed for Squad %d"), SquadID);
		
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
	AINPC_LOG(Warning, TEXT("🔧 ActivateScene called for SquadID: %d"), SquadID);
	
	if (FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID))
	{
		AINPC_LOG(Warning, TEXT("✅ Found Squad %d in ActiveSquads"), SquadID);
		AINPC_LOG(Warning, TEXT("   - bIsActive: %s"), Squad->bIsActive ? TEXT("true") : TEXT("false"));
		AINPC_LOG(Warning, TEXT("   - bEnableAmbientDialogue: %s"), Squad->bEnableAmbientDialogue ? TEXT("true") : TEXT("false"));
		
		if (!Squad->bIsActive)
		{
			AINPC_LOG(Warning, TEXT("🎬 Activating Squad %d..."), SquadID);
			Squad->bIsActive = true;
			
			// Note: NPCs will speak via Ambient Dialogue system, not all at once
			
			// Start Ambient Dialogue Timer
			if (Squad->bEnableAmbientDialogue)
			{
				AINPC_LOG(Warning, TEXT("🎤 About to call StartAmbientDialogue for Squad %d"), SquadID);
				StartAmbientDialogue(SquadID);
				AINPC_LOG(Warning, TEXT("ActivateScene: Squad %d Activated with Ambient Dialogue enabled."), SquadID);
			}
			else
			{
				AINPC_LOG(Warning, TEXT("ActivateScene: Squad %d Activated (Ambient Dialogue disabled)."), SquadID);
			}
		}
		else
		{
			AINPC_LOG(Warning, TEXT("⚠️ Squad %d is already active, skipping activation"), SquadID);
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

	// Destroy Props
	for (AActor* Prop : Squad.SpawnedProps)
	{
		if (IsValid(Prop))
		{
			Prop->Destroy();
		}
	}
	Squad.SpawnedProps.Empty();

	// Reset roles for all members
	for (auto& Elem : Squad.MemberRoles)
	{
		AActor* Member = Elem.Key;
		ActorSquadMap.Remove(Member);
		
		if (Member)
		{
			// Remove Scene Tag
			if (UGoalComponent* GoalComp = AINPCHelpers::GetGoalComponent(Member))

			{
				GoalComp->RemoveContextTag(AINPCTags::Status_InScene);
			}
			Member->Tags.Remove("Status.InScene"); // Legacy / Fallback


			// Reset Cognition
			if (UCognitionComponent* Cognition = AINPCHelpers::GetCognitionComponent(Member))

			{
				Cognition->RoleDescription = TEXT(""); 
				Cognition->ProcessStimulus(TEXT("The scene has ended. I return to my daily routine."));
			}
		}
	}
	
	ActiveSquads.Remove(SquadID);
	UE_LOG(LogTemp, Log, TEXT("Scene Squad %d Ended and Disbanded."), SquadID);
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
	AINPC_LOG(Warning, TEXT("[AmbientDialogue] 🔧 StartAmbientDialogue called for Squad %d"), SquadID);
	
	FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID);
	if (!Squad)
	{
		AINPC_LOG_ERROR(TEXT("[AmbientDialogue] ❌ Squad %d not found in ActiveSquads!"), SquadID);
		return;
	}
	
	if (!Squad->bIsActive)
	{
		AINPC_LOG(Warning, TEXT("[AmbientDialogue] ⚠️ Squad %d is not active (bIsActive=false)"), SquadID);
		return;
	}
	
	if (!Squad->bEnableAmbientDialogue)
	{
		AINPC_LOG(Warning, TEXT("[AmbientDialogue] ⚠️ Squad %d has Ambient Dialogue disabled (bEnableAmbientDialogue=false)"), SquadID);
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

	AINPC_LOG(Warning, TEXT("[AmbientDialogue] ✅ Timer set! Squad %d will trigger in %.1f seconds"), SquadID, RandomInterval);
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
		AINPC_LOG(Warning, TEXT("[AmbientDialogue] ⏭️ Squad %d: Player not nearby, skipping trigger"), SquadID);
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
		AINPC_LOG(Warning, TEXT("[AmbientDialogue] ❌ Squad %d: No valid speakers found"), SquadID);
		StartAmbientDialogue(SquadID);
		return;
	}

	// Randomly select 1-N speakers
	int32 NumSpeakers = FMath::Min(
		FMath::RandRange(1, Squad->AmbientSpeakersPerTrigger), 
		ValidSpeakers.Num()
	);

	AINPC_LOG(Warning, TEXT("[AmbientDialogue] 🎬 Squad %d: Triggering %d/%d speakers (Player nearby)"), 
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
			 "Generate a brief ambient line (1-2 sentences) that reflects your character and current situation. "
			 "This is NOT to advance the plot, just to add atmosphere and immersion. "
			 "Feel free to comment on your surroundings, your feelings, or what you're doing. "
			 "Keep it natural and in-character."),
		*Squad->PlotOutline,
		*Squad->MemberRoles.FindRef(Speaker),
		*CurrentActivity
	);

	// Trigger CognitionComponent to generate response
	CogComp->ProcessStimulus(AmbientPrompt);

	AINPC_LOG(Warning, TEXT("[AmbientDialogue] 💬 %s triggered ambient speech (Activity: %s)"), 
		*Speaker->GetName(), *CurrentActivity);
	AINPC_LOG(Warning, TEXT("[AmbientDialogue] 🔔 Speech should appear above NPC's head in 1-2 seconds if EmotionDisplayComponent is configured."));
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
