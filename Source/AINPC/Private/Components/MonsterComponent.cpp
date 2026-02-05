
#include "Components/MonsterComponent.h"
#include "Components/NPCDefinitionComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/FactionReputationComponent.h"
#include "Components/CognitionComponent.h"

UMonsterComponent::UMonsterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMonsterComponent::BeginPlay()
{
	Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (!Owner) return;

    // 0. Tag: Bypass scene safety rules (EvaluateCombatPolicy / GetFactionID)
    Owner->Tags.AddUnique(FName("Combat.AlwaysHostile"));

    // 1. Force Faction ID on Pawn's components
    if (UNPCDefinitionComponent* DefComp = Owner->FindComponentByClass<UNPCDefinitionComponent>())
    {
        DefComp->FactionID = MonsterFactionID;
    }
    if (UFactionReputationComponent* FacComp = Owner->FindComponentByClass<UFactionReputationComponent>())
    {
        FacComp->FactionID = MonsterFactionID;
    }

    // 1b. Also sync to Controller's FactionReputationComponent (fix BeginPlay timing)
    if (APawn* Pawn = Cast<APawn>(Owner))
    {
        if (AController* Controller = Pawn->GetController())
        {
            if (UFactionReputationComponent* CtrlFacComp = Controller->FindComponentByClass<UFactionReputationComponent>())
            {
                CtrlFacComp->FactionID = MonsterFactionID;
                UE_LOG(LogTemp, Log, TEXT("[MonsterComponent] Synced FactionID '%s' to Controller's FactionReputationComponent"), *MonsterFactionID.ToString());
            }
        }
    }

    // 2. Force Personality to "Brainless Feral"
    // This overrides any previous Personality settings to ensure consistent "Zombie" behavior
    if (AController* Controller = Cast<AController>(Owner->GetOwner())) // Usually Component is on Pawn? Or Controller? Assume Actor is Pawn currently
    {
        // Try getting controller from Pawn
         if (APawn* Pawn = Cast<APawn>(Owner))
         {
             Controller = Pawn->GetController();
         }
    }

    // Try finding component on Owner or Controller
    UPersonalityComponent* PersComp = Owner->FindComponentByClass<UPersonalityComponent>();
    if (!PersComp && Cast<APawn>(Owner) && Cast<APawn>(Owner)->GetController())
    {
        PersComp = Cast<APawn>(Owner)->GetController()->FindComponentByClass<UPersonalityComponent>();
    }

    if (PersComp)
    {
        // Set Feral OCEAN stats (Low Openness/Conscientiousness, High Neuroticism?)
        // Or just let the prompt handle it via "RoleDescription"
        PersComp->Personality.RoleDescription = 
            TEXT("[INSTINCTS] You are a MINDLESS MONSTER. Driven purely by hunger. No logic, no mercy. Attack on sight.");
        
        PersComp->Personality.BehavioralGuidelines = 
            TEXT("Attack any non-Monster instantly. Never retreat. Make guttural noises.");

        // Adjust OCEAN to be 'Reactive'
        PersComp->Personality.Openness = 0.1f;
        PersComp->Personality.Conscientiousness = 0.1f;
        PersComp->Personality.Extraversion = 0.1f;
        PersComp->Personality.Agreeableness = 0.0f;
        PersComp->Personality.Neuroticism = 0.1f; 
    }

    // 3. Disable Higher Cognition (Save tokens/performance)
    // Zombies don't need to "think" via LLM, they just react via Utility Actions.
    UCognitionComponent* CogComp = Owner->FindComponentByClass<UCognitionComponent>();
    if (CogComp)
    {
        CogComp->bEnableReasoning = false;
        UE_LOG(LogTemp, Log, TEXT("[MonsterComponent] Disabled Cognition Reasoning for %s"), *Owner->GetName());
    }
    else if (AController* Controller = Cast<AController>(Owner->GetOwner()))
    {
         UCognitionComponent* CtrlCogComp = Controller->FindComponentByClass<UCognitionComponent>();
         if (CtrlCogComp)
         {
             CtrlCogComp->bEnableReasoning = false;
             UE_LOG(LogTemp, Log, TEXT("[MonsterComponent] Disabled Cognition Reasoning for Controller of %s"), *Owner->GetName());
         }
    }
}
