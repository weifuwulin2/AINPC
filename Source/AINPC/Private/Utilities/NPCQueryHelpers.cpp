#include "Utilities/NPCQueryHelpers.h"
#include "Utilities/AINPCHelpers.h"
#include "Utilities/FactionHelpers.h"
#include "Components/CognitionComponent.h"
#include "Components/NPCDefinitionComponent.h"
#include "Components/GoalComponent.h"
#include "Social/SocialGameplayTags.h"
#include "EngineUtils.h" // TActorIterator
#include "GameFramework/Pawn.h"

namespace NPCQueryHelpers
{

TArray<AActor*> GetAllLivingNPCs(UWorld* World)
{
	TArray<AActor*> Result;
	if (!World) return Result;

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!IsValid(Pawn)) continue;

		// Must have CognitionComponent (indicates an AI NPC)
		if (!AINPCHelpers::GetCognitionComponent(Pawn)) continue;

		// Must not be dead
		if (UGoalComponent* GoalComp = AINPCHelpers::GetGoalComponent(Pawn))
		{
			if (GoalComp->HasContextTag(AINPCTags::Status_Dead)) continue;
		}

		// Also check actor tag fallback
		if (Pawn->ActorHasTag(FName("Status.Dead"))) continue;

		Result.Add(Pawn);
	}

	return Result;
}

TArray<AActor*> FilterByFaction(const TArray<AActor*>& NPCs, FName FactionID)
{
	TArray<AActor*> Result;
	for (AActor* NPC : NPCs)
	{
		if (!IsValid(NPC)) continue;
		if (FactionHelpers::GetFactionID(NPC) == FactionID)
		{
			Result.Add(NPC);
		}
	}
	return Result;
}

TArray<AActor*> FilterByProfession(const TArray<AActor*>& NPCs, FName ProfessionID)
{
	TArray<AActor*> Result;
	for (AActor* NPC : NPCs)
	{
		if (!IsValid(NPC)) continue;
		if (UNPCDefinitionComponent* DefComp = AINPCHelpers::GetNPCDefinitionComponent(NPC))
		{
			if (DefComp->ProfessionID == ProfessionID)
			{
				Result.Add(NPC);
			}
		}
	}
	return Result;
}

TArray<AActor*> FilterAvailable(const TArray<AActor*>& NPCs)
{
	TArray<AActor*> Result;
	for (AActor* NPC : NPCs)
	{
		if (!IsValid(NPC)) continue;

		bool bInScene = false;
		if (UGoalComponent* GoalComp = AINPCHelpers::GetGoalComponent(NPC))
		{
			bInScene = GoalComp->HasContextTag(AINPCTags::Status_InScene);
		}

		if (!bInScene)
		{
			Result.Add(NPC);
		}
	}
	return Result;
}

FString BuildNPCBrief(AActor* NPC)
{
	if (!IsValid(NPC)) return FString();

	FString Name = AINPCHelpers::GetSmartActorName(NPC);
	FName Faction = FactionHelpers::GetFactionID(NPC);
	FName Profession = NAME_None;

	if (UNPCDefinitionComponent* DefComp = AINPCHelpers::GetNPCDefinitionComponent(NPC))
	{
		Profession = DefComp->ProfessionID;
	}

	return FString::Printf(TEXT("%s (%s, %s)"),
		*Name,
		*Faction.ToString(),
		Profession.IsNone() ? TEXT("Unknown") : *Profession.ToString());
}

AActor* FindNPCByDescription(const TArray<AActor*>& Candidates, const FString& Description)
{
	if (Description.IsEmpty() || Candidates.Num() == 0) return nullptr;

	// Tokenize description into lowercase keywords
	FString LowerDesc = Description.ToLower();

	AActor* BestMatch = nullptr;
	int32 BestScore = 0;

	for (AActor* NPC : Candidates)
	{
		if (!IsValid(NPC)) continue;

		int32 Score = 0;

		// Check faction match
		FName Faction = FactionHelpers::GetFactionID(NPC);
		if (!Faction.IsNone() && LowerDesc.Contains(Faction.ToString().ToLower()))
		{
			Score += 10;
		}

		// Check profession match
		if (UNPCDefinitionComponent* DefComp = AINPCHelpers::GetNPCDefinitionComponent(NPC))
		{
			if (!DefComp->ProfessionID.IsNone() && LowerDesc.Contains(DefComp->ProfessionID.ToString().ToLower()))
			{
				Score += 10;
			}
		}

		// Check name match
		FString Name = AINPCHelpers::GetSmartActorName(NPC);
		if (!Name.IsEmpty() && LowerDesc.Contains(Name.ToLower()))
		{
			Score += 5;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestMatch = NPC;
		}
	}

	return BestMatch;
}

} // namespace NPCQueryHelpers
