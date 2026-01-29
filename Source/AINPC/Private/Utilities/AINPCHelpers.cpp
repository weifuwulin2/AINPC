#include "Utilities/AINPCHelpers.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Components/CognitionComponent.h"
#include "Components/GoalComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/UtilityAIComponent.h"
#include "Components/NPCDefinitionComponent.h"

namespace AINPCHelpers
{
	// Helper template to reduce code duplication
	template<typename TComponent>
	TComponent* FindComponentInHierarchy(AActor* Actor)
	{
		if (!Actor) return nullptr;

		// 1. Check on the Actor itself
		if (TComponent* Comp = Actor->FindComponentByClass<TComponent>())
		{
			return Comp;
		}

		// 2. If Actor is a Pawn, check its Controller
		if (APawn* Pawn = Cast<APawn>(Actor))
		{
			if (AController* Controller = Pawn->GetController())
			{
				if (TComponent* Comp = Controller->FindComponentByClass<TComponent>())
				{
					return Comp;
				}
			}
		}

		// 3. If Actor is a Controller, check its Pawn
		if (AController* Controller = Cast<AController>(Actor))
		{
			if (APawn* Pawn = Controller->GetPawn())
			{
				if (TComponent* Comp = Pawn->FindComponentByClass<TComponent>())
				{
					return Comp;
				}
			}
		}

		return nullptr;
	}

	UCognitionComponent* GetCognitionComponent(AActor* Actor)
	{
		return FindComponentInHierarchy<UCognitionComponent>(Actor);
	}

	UGoalComponent* GetGoalComponent(AActor* Actor)
	{
		return FindComponentInHierarchy<UGoalComponent>(Actor);
	}

	UPersonalityComponent* GetPersonalityComponent(AActor* Actor)
	{
		return FindComponentInHierarchy<UPersonalityComponent>(Actor);
	}

	UUtilityAIComponent* GetUtilityAIComponent(AActor* Actor)
	{
		return FindComponentInHierarchy<UUtilityAIComponent>(Actor);
	}

	UNPCDefinitionComponent* GetNPCDefinitionComponent(AActor* Actor)
	{
		return FindComponentInHierarchy<UNPCDefinitionComponent>(Actor);
	}

	FString GetSmartActorName(AActor* Actor)
	{
		FString TargetName = "Unknown";
		if (!Actor) return TargetName;

		// 1. Try NPC Definition (Highest Priority - Custom Names)
		if (UNPCDefinitionComponent* DefComp = GetNPCDefinitionComponent(Actor))
		{
			FString DisplayName = DefComp->GetDisplayName();
			if (!DisplayName.IsEmpty() && DisplayName != "Citizen" && DisplayName != "Zombie")
			{
				return DisplayName;
			}
		}

		// 2. Try Personality Component
		if (UPersonalityComponent* PersonalityComp = GetPersonalityComponent(Actor))
		{
			if (!PersonalityComp->PersonalityID.IsNone())
			{
				TargetName = PersonalityComp->PersonalityID.ToString();
			}
		}
		
		// 2. Try Player Tag
		if (TargetName == "Unknown" && Actor->ActorHasTag("Player"))
		{
			TargetName = "Player";
		}
		
		// 3. Fallback to Object Name
		if (TargetName == "Unknown")
		{
			TargetName = Actor->GetName();
		}
		
		return TargetName;
	}
}
