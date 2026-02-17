
#include "Social/FactionSubsystem.h"
#include "AINPC.h"

void UFactionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RuntimeFactionMatrix.Empty();
	RelationshipSeedMatrix.Empty();
}

void UFactionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Note: InitializeFactions must be called explicitly, typically by the GameMode or Level Script,
	// because we need the DataTable reference which isn't hardcoded.

	if (RelationshipSeedTable)
	{
		InitializeRelationshipSeeds(RelationshipSeedTable);
	}
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

void UFactionSubsystem::InitializeRelationshipSeeds(UDataTable* SeedTable)
{
	RelationshipSeedMatrix.Empty();

	if (!SeedTable)
	{
		AINPC_LOG(Warning, "FactionSubsystem: Relationship SeedTable is NULL, using faction baseline only.");
		return;
	}

	FString ContextString = TEXT("FactionSubsystem_InitializeRelationshipSeeds");
	TArray<FName> RowNames = SeedTable->GetRowNames();

	auto AddDirectionalSeed = [this](FName SourceID, FName TargetID, const FRelationshipSeedRow& Row)
	{
		if (SourceID.IsNone() || TargetID.IsNone())
		{
			return;
		}

		FRelationshipSeedRuntime Runtime;
		Runtime.Attitude = FMath::Clamp(Row.InitialAttitude, 0.0f, 100.0f);
		Runtime.BondType = Row.InitialBondType;
		Runtime.Summary = Row.InitialSummary;
		Runtime.Salience = FMath::Clamp(Row.InitialSalience, 0, 10);

		RelationshipSeedMatrix.FindOrAdd(SourceID).Add(TargetID, Runtime);
	};

	for (const FName& RowName : RowNames)
	{
		const FRelationshipSeedRow* Row = SeedTable->FindRow<FRelationshipSeedRow>(RowName, ContextString);
		if (!Row)
		{
			continue;
		}

		if (Row->SourceNameID.IsNone() || Row->TargetNameID.IsNone())
		{
			AINPC_LOG(Warning, "Relationship seed '%s' skipped due to missing SourceNameID/TargetNameID.",
				*RowName.ToString());
			continue;
		}

		AddDirectionalSeed(Row->SourceNameID, Row->TargetNameID, *Row);
		if (Row->bBidirectional)
		{
			AddDirectionalSeed(Row->TargetNameID, Row->SourceNameID, *Row);
		}
	}

	AINPC_LOG(Log, "Relationship seeds initialized: %d source entries.", RelationshipSeedMatrix.Num());
}

float UFactionSubsystem::GetBaseAttitude(FName SourceFaction, FName TargetFaction) const
{
	if (SourceFaction.IsNone() || TargetFaction.IsNone()) return 50.0f; // Neutral

	if (const TMap<FName, float>* Relations = RuntimeFactionMatrix.Find(SourceFaction))
	{
		if (SourceFaction == TargetFaction)
		{
			// Prefer explicit self-entry from DataTable if authored.
			if (const float* SelfVal = Relations->Find(SourceFaction))
			{
				return *SelfVal;
			}
			return DefaultIntraFactionAttitude;
		}

		if (const float* Val = Relations->Find(TargetFaction))
		{
			return *Val;
		}
	}

	if (SourceFaction == TargetFaction)
	{
		return DefaultIntraFactionAttitude;
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

bool UFactionSubsystem::TryGetSeedRelationship(
	FName SourceNameID,
	FName TargetNameID,
	float& OutAttitude,
	ESocialBondType& OutBondType,
	FString& OutSummary,
	int32& OutSalience) const
{
	if (SourceNameID.IsNone() || TargetNameID.IsNone())
	{
		return false;
	}

	const TMap<FName, FRelationshipSeedRuntime>* Targets = RelationshipSeedMatrix.Find(SourceNameID);
	if (!Targets)
	{
		return false;
	}

	const FRelationshipSeedRuntime* Seed = Targets->Find(TargetNameID);
	if (!Seed)
	{
		return false;
	}

	OutAttitude = Seed->Attitude;
	OutBondType = Seed->BondType;
	OutSummary = Seed->Summary;
	OutSalience = Seed->Salience;
	return true;
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
