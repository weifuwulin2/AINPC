# Village Social Initialization Test Guide

This guide validates the village-level NPC registry and initial relationship bootstrap flow.

For scenario-template-driven spawning and profession startup diagnostics, see:
- `docs/guides/Village_Scenario_Spawner_Test_Guide.md`

## Systems Under Test

- `UNPCDefinitionComponent`:
  - template load (`VillageID`)
  - modular identity randomization (`NameID`, backstory, past event)
  - auto-register to village subsystem on `BeginPlay`
- `UNPCVillageSubsystem`:
  - village membership registry
  - same-village bootstrap relationship generation
  - dynamic territory claim (`ClaimTerritory`, `ResolveVillageByLocation`)
  - deterministic attitude perturbation by stable IDs
- `UFactionReputationComponent`:
  - `EnsureInitialRelationshipWith` (seed-first initialization)
  - `SocialBonds` initialization behavior

## Preconditions

1. Project compiles and launches in PIE.
2. NPC Pawn has:
   - `UNPCDefinitionComponent`
   - `UFactionReputationComponent`
3. Optional but recommended:
   - `UFactionSubsystem.RelationshipSeedTable` assigned.
4. For deterministic checks:
   - Keep `VillageID`, `NameID`/stable IDs unchanged between runs.

## Test Matrix

| ID | Scenario | Steps | Expected |
|----|----------|-------|----------|
| V1 | Auto registration on BeginPlay | 1. Spawn NPC A with `VillageID=Village.Test`.<br>2. Call `GetVillageIDForNPC(A)`. | Returns `Village.Test`. |
| V2 | Empty VillageID is ignored | 1. Spawn NPC with empty `VillageID`.<br>2. Query village membership. | NPC is NOT registered to any village; no village bootstrap relationship is generated for this NPC. |
| V2b | Optional fallback mode | 1. Set `UNPCVillageSubsystem.bUseDefaultVillageIDWhenMissing=true`.<br>2. Spawn NPC with empty `VillageID`. | NPC is registered to `DefaultVillageID`. |
| V3 | Template-driven village assignment | 1. In NPC definition table row, set `VillageID=Village.Market`.<br>2. Spawn via template. | Runtime NPC village is `Village.Market`. |
| V4 | Same village bootstrap creates bond | 1. Spawn A then B into same village.<br>2. Inspect `A.SocialBonds` and `B.SocialBonds`. | Both have initialized bond entries for each other (if no prior entry). |
| V5 | Cross-village isolation | 1. Spawn A in `Village.A`, B in `Village.B`.<br>2. Inspect bonds after registration. | No auto-created village bootstrap bond between A and B. |
| V6 | Seed priority over fallback | 1. Configure seed A->B.<br>2. Spawn A/B in same village.<br>3. Inspect A->B bond summary/type. | Seed-defined bond/summary is used; village fallback does not override seed. |
| V7 | Deterministic bootstrap attitude | 1. Keep same IDs and village.<br>2. Run PIE twice.<br>3. Compare initial A->B relationship category/summary. | Result is stable across runs for same ID inputs. |
| V8 | Tier influence (leader/commoner) | 1. Add A's `ProfessionID` into `LeaderProfessionIDs` in `UNPCVillageSubsystem`.<br>2. Keep B outside leader/noble lists (commoner).<br>3. Spawn in same village. | A->B and B->A initial attitudes are asymmetric and reflect tier bias. |
| V9 | Dynamic late spawn | 1. Spawn A/B in village first.<br>2. Later spawn C in same village.<br>3. Inspect A/B/C bonds. | C gets initialized bonds with existing members; existing members get C initialized as well. |
| V10 | Missing component safety | 1. Spawn actor in village without `UFactionReputationComponent`.<br>2. Observe runtime. | No crash; subsystem skips social bootstrap for that actor. |
| V11 | Manual rebuild | 1. Call `RebuildVillageRelationships(Village.Test)` at runtime.<br>2. Inspect unresolved pairs. | Missing initial bonds are backfilled without duplicating existing bond entries. |
| V12 | Territory claim from new house | 1. Spawn house actor H at runtime.<br>2. Call `ClaimTerritoryFromActor(Village.A, H, 2000)`.<br>3. Spawn NPC with empty `VillageID` inside range. | NPC is auto-assigned to `Village.A` through territory resolution. |
| V13 | Territory revoke on house destroy | 1. Run V12 and verify assigned NPC.<br>2. Destroy house actor H.<br>3. Call `RefreshNPCVillageByLocation(NPC)` (or rely on auto refresh). | Territory claim is removed; NPC village assignment is re-evaluated and removed/changed if no other claim covers it. |
| V14 | Overlapping claims priority | 1. Add two claims covering same location with different `Priority`.<br>2. Resolve village for point/NPC in overlap. | Higher-priority claim wins. |
| V15 | Territory migration refresh | 1. Move NPC from claim area A into claim area B.<br>2. Call `RefreshNPCVillageByLocation(NPC)`. | NPC village assignment switches from A to B and relationships re-bootstrap to new village context. |

## Debug Checklist

1. Inspect runtime maps:
   - `UNPCVillageSubsystem::VillageMembers`
   - `UNPCVillageSubsystem::ActorVillageMap`
   - `UNPCVillageSubsystem::TerritoryClaims`
2. Inspect each NPC:
   - `UNPCDefinitionComponent::VillageID`
   - `UFactionReputationComponent::SocialBonds`
3. Check logs:
   - `[NPCVillage] Registered ...`

## Pass Criteria

- All cases V1-V15 and V2b pass.
- No crash on dynamic spawn/despawn.
- Seeded relationships remain higher priority than generated fallback.
- Village bootstrap only affects same-village members.
