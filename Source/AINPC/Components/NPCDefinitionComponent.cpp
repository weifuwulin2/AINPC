#include "Components/NPCDefinitionComponent.h"
#include "Components/UtilityAIComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/GoalComponent.h"
#include "AIController.h"

UNPCDefinitionComponent::UNPCDefinitionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// Default IDs
	PersonalityID = NAME_None;
	ProfessionID = NAME_None;
}

void UNPCDefinitionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Optional: Auto-apply if Owner is defined? 
	// Usually better to let the Owner call this when Controller is ready.
}

void UNPCDefinitionComponent::ApplyDefinition(AAIController* Controller)
{
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("[NPCDefinition] ApplyDefinition failed: Controller is null."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[NPCDefinition] Applying Profile for %s..."), *GetOwner()->GetName());

	ApplyPersonality(Controller);
	ApplyProfession(Controller);
}

void UNPCDefinitionComponent::ApplyPersonality(AAIController* Controller)
{
	if (PersonalityID.IsNone()) return;

	if (UPersonalityComponent* PersonalityComp = Controller->FindComponentByClass<UPersonalityComponent>())
	{
		PersonalityComp->SetPersonalityByID(PersonalityID);
		UE_LOG(LogTemp, Log, TEXT("  - Applied Personality: %s"), *PersonalityID.ToString());
		
		// Fallback Faction Tags (Legacy logic, helpful for simple systems)
		AActor* Owner = GetOwner();
		if (Owner)
		{
			FString PName = PersonalityID.ToString();
			if (PName.Contains("Zombie") || PName.Contains("Monster"))
			{
				Owner->Tags.AddUnique(TEXT("Zombie"));
			}
			else if (PName.Contains("Guard") || PName.Contains("Warrior") || PName.Contains("Merchant"))
			{
				Owner->Tags.AddUnique(TEXT("Human"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("  - Failed to apply Personality: PersonalityComponent not found on Controller."));
	}
}

void UNPCDefinitionComponent::ApplyProfession(AAIController* Controller)
{
	if (ProfessionID.IsNone()) return;

	// 1. Goal Component (Schedule)
	if (UGoalComponent* GoalComp = Controller->FindComponentByClass<UGoalComponent>())
	{
		GoalComp->InitializeProfession(ProfessionID);
		UE_LOG(LogTemp, Log, TEXT("  - Applied Profession (Goal): %s"), *ProfessionID.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("  - No GoalComponent found. Profession (Goal) ignored."));
	}

    // 2. Utility AI Component (Action Filtering)
    if (UUtilityAIComponent* UtilityComp = Controller->FindComponentByClass<UUtilityAIComponent>())
    {
        UtilityComp->SetProfession(ProfessionID);
        UE_LOG(LogTemp, Log, TEXT("  - Applied Profession (Utility): %s"), *ProfessionID.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("  - No UtilityAIComponent found. Profession (Actions) ignored."));
    }
}
