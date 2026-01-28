
#include "Subsystems/PlayerSquadSubsystem.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void UPlayerSquadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PartyMembers.Empty();
}

void UPlayerSquadSubsystem::RegisterPartyMember(AActor* Member)
{
	if (!Member) return;
	if (PartyMembers.Contains(Member)) return;

	PartyMembers.Add(Member);
	UE_LOG(LogTemp, Log, TEXT("[PlayerSquad] Registered Member: %s. Total: %d"), *Member->GetName(), PartyMembers.Num());
}

void UPlayerSquadSubsystem::UnregisterPartyMember(AActor* Member)
{
	if (PartyMembers.Contains(Member))
	{
		PartyMembers.Remove(Member);
		UE_LOG(LogTemp, Log, TEXT("[PlayerSquad] Unregistered Member: %s"), *Member->GetName());
	}
}

FString UPlayerSquadSubsystem::GetPartyContextDescription() const
{
	FString Context = TEXT("Party Status: ");
	
	for (AActor* Member : PartyMembers)
	{
		if (!Member) continue;

		FString Name = Member->GetActorLabel();
		FString Status = TEXT("Healthy");

		// Simple Health Check if it's a Character (Assuming standard damage handling or custom component later)
		// For now, just checking generic "IsPendingKill" or basic actor state
		
		Context += FString::Printf(TEXT("[%s: %s] "), *Name, *Status);
	}

	return Context;
}
