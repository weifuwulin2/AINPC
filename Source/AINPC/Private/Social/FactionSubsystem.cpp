
#include "Social/FactionSubsystem.h"
#include "AINPC.h"

void UFactionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RuntimeFactionMatrix.Empty();
}

void UFactionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Note: InitializeFactions must be called explicitly, typically by the GameMode or Level Script,
	// because we need the DataTable reference which isn't hardcoded.
}

void UFactionSubsystem::InitializeFactions(UDataTable* FactionTable)
{
	if (!FactionTable)
	{
		AINPC_LOG_ERROR("FactionSubsystem: DataTable is NULL!");
		return;
	}

	RuntimeFactionMatrix.Empty();
	HostilityThresholds.Empty();

	FString ContextString;
	TArray<FName> RowNames = FactionTable->GetRowNames();

	for (const FName& RowName : RowNames)
	{
		FFactionDef* Row = FactionTable->FindRow<FFactionDef>(RowName, ContextString);
		if (Row)
		{
			// Initialize Matrix with default values
			RuntimeFactionMatrix.Add(RowName, Row->DefaultReputations);
			
			// Cache thresholds (we assume self-thresholds apply to how this faction views others)
			HostilityThresholds.Add(RowName, Row->HostilityThreshold);

			AINPC_LOG(Log, "Faction Loaded: %s (Relations: %d)", *RowName.ToString(), Row->DefaultReputations.Num());
		}
	}
}

float UFactionSubsystem::GetBaseAttitude(FName SourceFaction, FName TargetFaction) const
{
	if (SourceFaction.IsNone() || TargetFaction.IsNone()) return 50.0f; // Neutral
	if (SourceFaction == TargetFaction) return 100.0f; // Self is always friendly

	if (const TMap<FName, float>* Relations = RuntimeFactionMatrix.Find(SourceFaction))
	{
		if (const float* Val = Relations->Find(TargetFaction))
		{
			return *Val;
		}
	}

	// Default to Neutral if no relationship defined
	return 50.0f;
}

void UFactionSubsystem::SetFactionRelation(FName SourceFaction, FName TargetFaction, float NewValue)
{
	if (SourceFaction.IsNone() || TargetFaction.IsNone()) return;

	// Ensure Source map exists
	TMap<FName, float>& Relations = RuntimeFactionMatrix.FindOrAdd(SourceFaction);
	
	// Update or Add
	Relations.Add(TargetFaction, FMath::Clamp(NewValue, 0.0f, 100.0f));

	AINPC_LOG(Warning, "Faction Relation Changed: %s -> %s = %.1f", *SourceFaction.ToString(), *TargetFaction.ToString(), NewValue);
}

bool UFactionSubsystem::AreFactionsHostile(FName SourceFaction, FName TargetFaction) const
{
	float BaseAttitude = GetBaseAttitude(SourceFaction, TargetFaction);
	
	// Use cached threshold if available, else default 25
	float Threshold = 25.0f;
	if (const float* T = HostilityThresholds.Find(SourceFaction))
	{
		Threshold = *T;
	}

	return BaseAttitude < Threshold;
}

void UFactionSubsystem::DebugPrintRelations() const
{
	AINPC_LOG(Warning, "=== Global Faction Relations ===");
	for (const auto& SourcePair : RuntimeFactionMatrix)
	{
		FName Source = SourcePair.Key;
		for (const auto& TargetPair : SourcePair.Value)
		{
			FName Target = TargetPair.Key;
			float Val = TargetPair.Value;
			
			if (Val < 25.0f || Val > 75.0f) // Only print interesting ones
			{
				AINPC_LOG(Log, "  %s -> %s : %.1f", *Source.ToString(), *Target.ToString(), Val);
			}
		}
	}
}
