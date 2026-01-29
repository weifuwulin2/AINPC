#include "World/NarrativeSceneAnchor.h"

#include "AINPC.h"
#include "Subsystems/NarrativeSquadSubsystem.h"
#include "Subsystems/NarrativeDirectorSubsystem.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ANarrativeSceneAnchor::ANarrativeSceneAnchor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Create a root component so this actor has a transform
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	// ✅ Outer Trigger: Activates scene when player enters (larger radius)
	SceneActivationTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("SceneActivationTrigger"));
	SceneActivationTrigger->SetupAttachment(RootComponent);
	SceneActivationTrigger->SetSphereRadius(SceneActivationRadius);  // Default 2000
	SceneActivationTrigger->SetCollisionProfileName(TEXT("Trigger"));

	// ✅ Inner Trigger: Broadcasts Event.PlayerDetected when player gets close (smaller radius)
	EventTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("EventTrigger"));
	EventTrigger->SetupAttachment(RootComponent);
	EventTrigger->SetSphereRadius(EventTriggerRadius);  // Default 800
	EventTrigger->SetCollisionProfileName(TEXT("Trigger"));
}

void ANarrativeSceneAnchor::BeginPlay()
{
	Super::BeginPlay();

	// Bind Overlaps for both triggers
	if (SceneActivationTrigger)
	{
		SceneActivationTrigger->OnComponentBeginOverlap.AddDynamic(this, &ANarrativeSceneAnchor::OnSceneActivationOverlap);
	}
	if (EventTrigger)
	{
		EventTrigger->OnComponentBeginOverlap.AddDynamic(this, &ANarrativeSceneAnchor::OnEventTriggerOverlap);
	}

	if (UWorld* World = GetWorld())
	{
		if (UNarrativeSquadSubsystem* SquadSystem = World->GetSubsystem<UNarrativeSquadSubsystem>())
		{
			SquadSystem->RegisterAnchor(this);

			// Auto-Spawn Scene (Inactive)
			if (SceneTable && NPCTable)
			{
				CurrentSquadID = SquadSystem->SpawnSceneAtAnchor(this, SceneTable, SupportedSceneID, NPCTable, false);
				if (CurrentSquadID != -1)
				{
					NARRATIVE_LOG( Log, TEXT("[NarrativeAnchor] Auto-spawned Squad %d for Scene '%s'"), CurrentSquadID, *SupportedSceneID.ToString());
				}
				else
				{
					NARRATIVE_LOG( Error, TEXT("[NarrativeAnchor] Failed to spawn scene! Check Output Log for Subsystem errors."));
				}
			}
			else
			{
				NARRATIVE_LOG( Warning, TEXT("[NarrativeAnchor] Missing DataTables! SceneTable: %s, NPCTable: %s. Cannot spawn."), 
					SceneTable ? TEXT("OK") : TEXT("MISSING"), 
					NPCTable ? TEXT("OK") : TEXT("MISSING"));
			}
		}
		else
		{
			NARRATIVE_LOG( Error, TEXT("[NarrativeAnchor] Failed to get NarrativeSquadSubsystem!"));
		}
	}
		
	
}

void ANarrativeSceneAnchor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeSquadSubsystem* SquadSystem = World->GetSubsystem<UNarrativeSquadSubsystem>())
		{
			SquadSystem->UnregisterAnchor(this);
		}
	}
	
	Super::EndPlay(EndPlayReason);
}

// ✅ Helper: Check if actor is a player
bool ANarrativeSceneAnchor::IsPlayerActor(AActor* Actor) const
{
	if (!Actor) return false;
	
	// Check for Player tag
	if (Actor->ActorHasTag("Player")) return true;
	
	// Check if this is a Pawn controlled by a PlayerController
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		return Cast<APlayerController>(Pawn->GetController()) != nullptr;
	}
	
	return false;
}

// ✅ Outer Trigger: Activates the scene
void ANarrativeSceneAnchor::OnSceneActivationOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bAutoTriggerOnOverlap) return;
	if (!IsPlayerActor(OtherActor)) return;
	if (CurrentSquadID == -1) return;
	
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeSquadSubsystem* SquadSystem = World->GetSubsystem<UNarrativeSquadSubsystem>())
		{
			NARRATIVE_LOG( Warning, TEXT("🎬 [NarrativeAnchor] Player entered OUTER trigger! Activating Squad %d"), CurrentSquadID);
			SquadSystem->ActivateScene(CurrentSquadID);
		}
	}
}

// ✅ Inner Trigger: Broadcasts Event.PlayerDetected
void ANarrativeSceneAnchor::OnEventTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bAutoTriggerOnOverlap) return;
	if (!IsPlayerActor(OtherActor)) return;
	
	if (UWorld* World = GetWorld())
	{
		if (UNarrativeDirectorSubsystem* Director = World->GetSubsystem<UNarrativeDirectorSubsystem>())
		{
			// ✅ RecordEvent takes (Description, Tags), not FNarrativeEvent
			FString Description = FString::Printf(TEXT("Player entered %s event trigger zone"), *SupportedSceneID.ToString());
			TArray<FName> EventTags;
			EventTags.Add("Event.PlayerDetected");
			
			Director->RecordEvent(Description, EventTags);
			
			NARRATIVE_LOG( Warning, TEXT("📡 [NarrativeAnchor] Player entered INNER trigger! Broadcast Event.PlayerDetected for Scene %s"), *SupportedSceneID.ToString());
		}
	}
}

void ANarrativeSceneAnchor::ApplyTagToRole(FString RoleID, FGameplayTag Tag)
{
	NARRATIVE_LOG(Warning, TEXT("🎯 [NarrativeAnchor] ApplyTagToRole called: RoleID='%s', Tag='%s', SquadID=%d"), 
		*RoleID, *Tag.ToString(), CurrentSquadID);

	if (CurrentSquadID == -1)
	{
		NARRATIVE_LOG(Warning, TEXT("❌ ApplyTagToRole FAILED: Scene is not active (SquadID == -1)"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UNarrativeSquadSubsystem* SquadSystem = World->GetSubsystem<UNarrativeSquadSubsystem>())
		{
			NARRATIVE_LOG(Warning, TEXT("✅ Delegating to NarrativeSquadSubsystem..."));
			SquadSystem->ApplyTagToRole(CurrentSquadID, RoleID, Tag);
		}
		else
		{
			NARRATIVE_LOG(Error, TEXT("❌ NarrativeSquadSubsystem not found!"));
		}
	}
}

void ANarrativeSceneAnchor::EnableCombatForRole(FString RoleID)
{
	NARRATIVE_LOG(Warning, TEXT("🔥 [NarrativeAnchor] EnableCombatForRole called for RoleID: '%s'"), *RoleID);
	
	// Resolve tag Event.Danger.Combat from SocialGameplayTags
	FGameplayTag CombatTag = FGameplayTag::RequestGameplayTag(FName("Event.Danger.Combat"));
	
	if (CombatTag.IsValid())
	{
		NARRATIVE_LOG(Warning, TEXT("✅ Tag 'Event.Danger.Combat' resolved successfully"));
		ApplyTagToRole(RoleID, CombatTag);
	}
	else
	{
		NARRATIVE_LOG(Error, TEXT("❌ EnableCombatForRole FAILED: Tag 'Event.Danger.Combat' not found!"));
	}
}
