#include "Subsystems/NarrativeSquadSubsystem.h"
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
#include "World/NarrativeSceneAnchor.h"

// Helper: Find CognitionComponent - it lives on Controller, not Pawn
static UCognitionComponent* GetCognitionFromActor(AActor* Actor)
{
	if (!Actor) return nullptr;
	
	// 1. Try direct (if it's a Controller)
	if (UCognitionComponent* Direct = Actor->FindComponentByClass<UCognitionComponent>())
	{
		return Direct;
	}
	
	// 2. If it's a Pawn, check its Controller
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		if (AController* Con = Pawn->GetController())
		{
			return Con->FindComponentByClass<UCognitionComponent>();
		}
	}
	
	return nullptr;
}

// Helper: Get PersonalityComponent
static UPersonalityComponent* GetPersonalityFromActor(AActor* Actor)
{
	if (!Actor) return nullptr;

	// Try Controller
	AController* Controller = nullptr;
	if (APawn* P = Cast<APawn>(Actor)) Controller = P->GetController();
	else Controller = Cast<AController>(Actor);

	if (AUtilityAIController* UAICon = Cast<AUtilityAIController>(Controller))
	{
		return UAICon->PersonalityComp;
	}
	
	return Actor->FindComponentByClass<UPersonalityComponent>();
}

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
	Squad.bIsActive = true;

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
	NPC->Tags.AddUnique("Status.InScene");

	// ✅ DEBUG: Log NPC configuration
	if (UNPCDefinitionComponent* DefComp = NPC->FindComponentByClass<UNPCDefinitionComponent>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NarrativeSquad] Assigned Role '%s' to NPC (Profession: %s, Faction: %s)"), 
			*RoleDescription, *DefComp->ProfessionID.ToString(), *DefComp->FactionID.ToString());
	}

	// Inject into Brain immediately (CognitionComponent is on Controller!)
	if (UCognitionComponent* Cognition = GetCognitionFromActor(NPC))
	{
		// ✅ Get Base Personality Role (to avoid overwriting it)
		FString BaseRole = "";
		if (UPersonalityComponent* PersComp = GetPersonalityFromActor(NPC))
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
		UE_LOG(LogTemp, Log, TEXT("[NarrativeSquad] Updated RoleDescription for %s (Additive Context)"), *NPC->GetName());
		
		// Note: No ProcessStimulus here - just update the role silently
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
		if (Actor && GetCognitionFromActor(Actor))
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
	int32 SquadID = SpawnSceneFromTemplate(SceneTable, SelectedID, Anchor->GetActorTransform(), NPCTable);
	
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
	if (FNarrativeSceneSquad* Squad = ActiveSquads.Find(SquadID))
	{
		if (!Squad->bIsActive)
		{
			Squad->bIsActive = true;
			// Notify Members "Action!"
			for (auto& Elem : Squad->MemberRoles)
			{
				if (UCognitionComponent* Cog = GetCognitionFromActor(Elem.Key))
				{
					Cog->ProcessStimulus(TEXT("The scene has started. Act according to your role."));
				}
			}
			UE_LOG(LogTemp, Log, TEXT("ActivateScene: Squad %d Activated."), SquadID);
		}
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
			Member->Tags.Remove("Status.InScene");

			// Reset Cognition
			if (UCognitionComponent* Cognition = GetCognitionFromActor(Member))
			{
				Cognition->RoleDescription = TEXT(""); 
				Cognition->ProcessStimulus(TEXT("The scene has ended. I return to my daily routine."));
			}
		}
	}
	
	ActiveSquads.Remove(SquadID);
	UE_LOG(LogTemp, Log, TEXT("Scene Squad %d Ended and Disbanded."), SquadID);
}
