#include "Components/NPCDefinitionComponent.h"
#include "Components/UtilityAIComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/GoalComponent.h"
#include "AIController.h"
#include "AINPC.h"

UNPCDefinitionComponent::UNPCDefinitionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// Default IDs
	PersonalityID = NAME_None;
	ProfessionID = NAME_None;
	SocialProfileID = NAME_None;
	
	NameID = NAME_None;
	PastEventID = NAME_None;
	FactionID = NAME_None;
	DefinitionTemplateID = NAME_None;
}

void UNPCDefinitionComponent::LoadFromTemplate()
{
	// If no template specified, skip
	if (DefinitionTemplateID.IsNone() || !DefinitionTable)
	{
		return;
	}

	// Find the row in DT_NPCDefinitions
	FString ContextString = TEXT("NPCDefinition_LoadFromTemplate");
	const FNPCDefinitionRow* Row = DefinitionTable->FindRow<FNPCDefinitionRow>(DefinitionTemplateID, ContextString);
	
	if (!Row)
	{
		AINPC_LOG_WARNING("Failed to find template '%s' in DefinitionTable!", *DefinitionTemplateID.ToString());
		return;
	}

	// Auto-populate properties from template
	PersonalityID = Row->PersonalityID;
	ProfessionID = Row->ProfessionID;
	FactionID = Row->FactionID;

	AINPC_LOG(Log, "Loaded NPC Definition from template '%s': Personality=%s, Profession=%s, Faction=%s",
		*DefinitionTemplateID.ToString(),
		*PersonalityID.ToString(),
		*ProfessionID.ToString(),
		*FactionID.ToString());
}

void UNPCDefinitionComponent::RandomizeModularIdentity()
{
	// Randomize Name if not set
	if (NameID.IsNone() && NameTable)
	{
		TArray<FName> RowNames = NameTable->GetRowNames();
		if (RowNames.Num() > 0)
		{
			int32 RandomIndex = FMath::RandRange(0, RowNames.Num() - 1);
			NameID = RowNames[RandomIndex];
			AINPC_LOG(Log, "Randomized NameID: %s", *NameID.ToString());
		}
	}

	// Randomize Social Profile/Backstory if not set
	if (SocialProfileID.IsNone() && SocialProfileTable)
	{
		TArray<FName> RowNames = SocialProfileTable->GetRowNames();
		if (RowNames.Num() > 0)
		{
			int32 RandomIndex = FMath::RandRange(0, RowNames.Num() - 1);
			SocialProfileID = RowNames[RandomIndex];
			AINPC_LOG(Log, "Randomized SocialProfileID: %s", *SocialProfileID.ToString());
		}
	}

	// Randomize Past Event if not set
	if (PastEventID.IsNone() && PastEventTable)
	{
		TArray<FName> RowNames = PastEventTable->GetRowNames();
		if (RowNames.Num() > 0)
		{
			int32 RandomIndex = FMath::RandRange(0, RowNames.Num() - 1);
			PastEventID = RowNames[RandomIndex];
			AINPC_LOG(Log, "Randomized PastEventID: %s", *PastEventID.ToString());
		}
	}
}

bool UNPCDefinitionComponent::GetSocialProfileDef(FSocialProfileDef& OutDef) const
{
	if (SocialProfileID.IsNone() || !SocialProfileTable) return false;
	const FSocialProfileDef* Row = SocialProfileTable->FindRow<FSocialProfileDef>(SocialProfileID, TEXT("NPCDefinition_GetSocialProfileDef"));
	if (!Row) return false;
	OutDef = *Row;
	return true;
}

FString UNPCDefinitionComponent::GetDisplayName() const
{
	// 1. Try to get configured Name (First + Last)
	FNPCNameDef NameDef;
	if (GetNameDef(NameDef))
	{
		FString FullName = NameDef.FirstName;
		if (!NameDef.Surname.IsEmpty())
		{
			FullName += " " + NameDef.Surname;
		}
		return FullName;
	}

	// 2. Fallback: Profession (e.g., "Merchant", "Guard")
	if (!ProfessionID.IsNone() && ProfessionID != TEXT("None"))
	{
		return ProfessionID.ToString();
	}

	// 3. Fallback: Faction-based Identity (e.g., "Zombie", "Citizen")
	FString FacStr = FactionID.ToString();
	if (FacStr.Contains("Monster") || FacStr.Contains("Zombie"))
	{
		return TEXT("Zombie"); // Or specific monster type
	}
	else if (FacStr.Contains("Bandit"))
	{
		return TEXT("Bandit");
	}

	// 4. Ultimate Fallback
	return TEXT("Citizen");
}

bool UNPCDefinitionComponent::GetNameDef(FNPCNameDef& OutDef) const
{
	if (NameID.IsNone() || !NameTable) return false;
	const FNPCNameDef* Row = NameTable->FindRow<FNPCNameDef>(NameID, TEXT("NPCDefinition_GetNameDef"));
	if (!Row) return false;
	OutDef = *Row;
	return true;
}

bool UNPCDefinitionComponent::GetPastEventDef(FPastEventDef& OutDef) const
{
	if (PastEventID.IsNone() || !PastEventTable) return false;
	const FPastEventDef* Row = PastEventTable->FindRow<FPastEventDef>(PastEventID, TEXT("NPCDefinition_GetPastEventDef"));
	if (!Row) return false;
	OutDef = *Row;
	return true;
}

void UNPCDefinitionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Load from template if specified
	LoadFromTemplate();
	
	// Auto-randomize Name, Backstory, Past Event if not set
	RandomizeModularIdentity();
	
	// Optional: Auto-apply if Owner is defined? 
	// Usually better to let the Owner call this when Controller is ready.
}

void UNPCDefinitionComponent::ApplyDefinition(AAIController* Controller)
{
	if (!Controller)
	{
		AINPC_LOG_WARNING("ApplyDefinition failed: Controller is null.");
		return;
	}

	AINPC_LOG(Log, "Applying Profile for %s...", *GetOwner()->GetName());

	ApplyPersonality(Controller);
	ApplyProfession(Controller);
}

void UNPCDefinitionComponent::ApplyPersonality(AAIController* Controller)
{
	if (PersonalityID.IsNone()) return;

	if (UPersonalityComponent* PersonalityComp = Controller->FindComponentByClass<UPersonalityComponent>())
	{
		PersonalityComp->SetPersonalityByID(PersonalityID);
		AINPC_LOG(Log, "Applied Personality: %s", *PersonalityID.ToString());
		
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
		AINPC_LOG_WARNING("Failed to apply Personality: PersonalityComponent not found on Controller.");
	}
}

void UNPCDefinitionComponent::ApplyProfession(AAIController* Controller)
{
	if (ProfessionID.IsNone()) return;

	// 1. Goal Component (Schedule)
	if (UGoalComponent* GoalComp = Controller->FindComponentByClass<UGoalComponent>())
	{
		GoalComp->InitializeProfession(ProfessionID);
		AINPC_LOG(Log, "Applied Profession (Goal): %s", *ProfessionID.ToString());
	}
	else
	{
		AINPC_LOG(Log, "No GoalComponent found. Profession (Goal) ignored.");
	}

    // 2. Utility AI Component (Action Filtering)
    if (UUtilityAIComponent* UtilityComp = Controller->FindComponentByClass<UUtilityAIComponent>())
    {
        UtilityComp->SetProfession(ProfessionID);
        AINPC_LOG(Log, "Applied Profession (Utility): %s", *ProfessionID.ToString());
    }
    else
    {
        AINPC_LOG(Log, "No UtilityAIComponent found. Profession (Actions) ignored.");
    }
}
