#include "World/NarrativeSceneAnchor.h"
#include "Subsystems/NarrativeSquadSubsystem.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ANarrativeSceneAnchor::ANarrativeSceneAnchor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Create a root component so this actor has a transform
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	TriggerComponent = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerComponent->SetupAttachment(RootComponent);
	TriggerComponent->SetSphereRadius(ActivationRadius);  // Use configurable radius (default 2000)
	TriggerComponent->SetCollisionProfileName(TEXT("Trigger"));
}

void ANarrativeSceneAnchor::BeginPlay()
{
	Super::BeginPlay();

	// Bind Overlap
	if (TriggerComponent)
	{
		TriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &ANarrativeSceneAnchor::OnOverlapBegin);
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
					UE_LOG(LogTemp, Log, TEXT("[NarrativeAnchor] Auto-spawned Squad %d for Scene '%s'"), CurrentSquadID, *SupportedSceneID.ToString());
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[NarrativeAnchor] Failed to spawn scene! Check Output Log for Subsystem errors."));
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[NarrativeAnchor] Missing DataTables! SceneTable: %s, NPCTable: %s. Cannot spawn."), 
					SceneTable ? TEXT("OK") : TEXT("MISSING"), 
					NPCTable ? TEXT("OK") : TEXT("MISSING"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[NarrativeAnchor] Failed to get NarrativeSquadSubsystem!"));
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

void ANarrativeSceneAnchor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bAutoTriggerOnOverlap) return;
	
	// Check if already active? (SquadID valid + Squad Active?)
	// ActivateScene checks IsActive internally, so it's safe to call repeatedly.
	
	if (OtherActor)
	{
		// ✅ More robust player detection: Check for PlayerController or Player tag
		bool bIsPlayer = OtherActor->ActorHasTag("Player");
		
		// Also check if this is a Pawn controlled by a PlayerController
		if (!bIsPlayer)
		{
			if (APawn* Pawn = Cast<APawn>(OtherActor))
			{
				bIsPlayer = Cast<APlayerController>(Pawn->GetController()) != nullptr;
			}
		}
		
		if (bIsPlayer && CurrentSquadID != -1)
		{
			if (UWorld* World = GetWorld())
			{
				if (UNarrativeSquadSubsystem* SquadSystem = World->GetSubsystem<UNarrativeSquadSubsystem>())
				{
					UE_LOG(LogTemp, Warning, TEXT("[NarrativeAnchor] Player entered trigger! Activating Squad %d"), CurrentSquadID);
					SquadSystem->ActivateScene(CurrentSquadID);
				}
			}
		}
	}
}
