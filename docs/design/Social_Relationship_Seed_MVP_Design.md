# Social Relationship Seed MVP Design

## 1. Goal

Provide a lightweight social bootstrap layer for NPC-to-NPC relationships without building a full social graph engine.

Design targets:
- No `N^2` pre-generation.
- Works with dynamic spawn/despawn.
- Deterministic authored core relationships (lord, noble, family, rivals).
- Easy to debug and test.

## 2. Scope

Included in MVP:
- Fixed relationship seeds via DataTable (`FRelationshipSeedRow`).
- Lazy initialization on first relationship query.
- Seed result cached into `SocialBonds` for prompt/context reuse.
- Same-faction default baseline changed from hardcoded `100` to configurable fallback (`DefaultIntraFactionAttitude`, default `60`).
- Village registry bootstrap via `UNPCVillageSubsystem` (NPC grouped by `VillageID`, bootstrap after identity initialization).
- Dynamic village territory claims (building-driven): `ClaimTerritory` / `ClaimTerritoryFromActor`.

Not included in MVP:
- Full tree/graph social simulation.
- Gossip diffusion.
- Automatic long-term social planning.

## 3. Data Model

### 3.1 Seed Row
`FRelationshipSeedRow` fields:
- `SourceNameID`
- `TargetNameID`
- `InitialAttitude` (0-100)
- `InitialBondType` (`ESocialBondType`)
- `InitialSummary`
- `InitialSalience` (0-10)
- `bBidirectional`

### 3.2 Runtime Cache
`UFactionSubsystem` builds:
- `RelationshipSeedMatrix[SourceNameID][TargetNameID] -> FRelationshipSeedRuntime`

`UNPCVillageSubsystem` builds:
- `VillageMembers[VillageID] -> [NPC actors]`
- `ActorVillageMap[NPC actor] -> VillageID`
- `TerritoryClaims[]` (VillageID + center/radius/priority + optional source actor)

## 4. Resolution Order

Bootstrap timing:
1. NPC `BeginPlay` loads template + randomizes modular identity.
2. NPC auto-registers into `UNPCVillageSubsystem` by `VillageID` (when `VillageID` is empty, village bootstrap is skipped by default).
3. Subsystem initializes A<->B bonds inside the same village:
   - Priority 1: explicit seed.
   - Priority 2: deterministic fallback from faction baseline + tier bias + stable hash noise.
   - Tier bias uses existing `ProfessionID` membership lists (`LeaderProfessionIDs`, `NobleProfessionIDs`) rather than keyword parsing.

Dynamic territory flow:
1. A house/building is completed by gameplay.
2. Building system calls `UNPCVillageSubsystem::ClaimTerritoryFromActor(VillageID, HouseActor, Radius, Priority)`.
3. NPCs with empty `VillageID` can be assigned by `ResolveVillageByLocation` and refreshed via `RefreshNPCVillageByLocation` / `RefreshAllNPCVillageAssignments`.
4. If the building is destroyed, territory claim is revoked automatically.

When A queries attitude toward B (`GetAttitudeTowards`):
1. `PersonalReputations` override.
2. Existing `SocialBonds`.
3. Fixed seed lookup (`TryResolveSeedRelationship`).
   - If hit: returns seeded value.
   - Lazy cache: writes seed bond into `SocialBonds` once.
4. Faction baseline (`UFactionSubsystem::GetBaseAttitude`).

This keeps authored social structure as first-class data while preserving existing faction fallback.

## 5. Why This Fits Village-Style Worlds

You can explicitly author important social edges:
- Lord <-> Steward
- Noble House A <-> Noble House B
- Family / spouse / siblings
- Long-term rivalries

Everything else falls back to faction baseline, so setup cost remains low.

## 6. Debugability

Key debug points:
- `UFactionSubsystem::InitializeRelationshipSeeds`
- `UFactionSubsystem::TryGetSeedRelationship`
- `UFactionReputationComponent::TryResolveSeedRelationship`
- `UFactionReputationComponent::GetAttitudeTowards` (seed hit path)

Expected behavior:
- First seed hit: result comes from seed + bond cached.
- Later queries: hit `SocialBonds` directly.

## 7. Extension Path

If future complexity is needed, add layers in order:
1. Household defaults.
2. Role/status rules (`lord->commoner`, `commoner->lord`).
3. Deterministic fallback perturbation.
4. Full social graph + planning.

MVP stays valid as layer 1 and does not block future expansion.
