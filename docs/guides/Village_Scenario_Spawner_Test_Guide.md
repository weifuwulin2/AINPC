# Village Scenario Spawner Test Guide

This guide validates the `DT_VillageScenarios + AVillageSpawner` pipeline:
- scenario-template driven NPC spawn
- fixed key relationship seeds plus fallback social initialization
- startup profession diagnostics output

## Systems Under Test

- `FVillageScenarioRow` (`Source/AINPC/Public/World/VillageScenarioTypes.h`)
- `AVillageSpawner` (`Source/AINPC/Public/World/VillageSpawner.h`)
- `UNPCDefinitionComponent` deferred template assignment (`bSkipTemplateLoadOnBeginPlay`)
- `UNPCVillageSubsystem` village relationship fallback bootstrap
- `UFactionReputationComponent` seeded relationship materialization

## Preconditions

1. `DT_NPCDefinitions` contains valid rows with `PawnClass` assigned.
2. `DT_VillageScenarios` contains at least one scenario row.
3. NPC pawn class has:
   - `UNPCDefinitionComponent`
   - `UFactionReputationComponent`
4. World has `UNPCVillageSubsystem` enabled (world subsystem auto-load is fine).

## Minimal Authoring Setup

1. Create or reuse a row in `DT_VillageScenarios`:
   - set `VillageID`
   - add `RequiredRoles` (lord/guard/commoner etc.)
   - add optional `OptionalRoles`
   - add key `RelationshipSeeds` for family/superior/rival edges
2. Place one `AVillageSpawner` in level:
   - assign `VillageScenarioTable` = `DT_VillageScenarios`
   - assign `NPCDefinitionTable` = `DT_NPCDefinitions`
   - set `ScenarioRowID`
   - keep `bSpawnOnBeginPlay = true`
3. (Optional) set `RandomSeed` for deterministic spawn layout.

## Relationship Seed Authoring Rules

1. Key residents should be authored in `RelationshipSeeds`:
   - family edges
   - superior/subordinate edges
   - rival edges
2. Ordinary residents should not need full pair authoring:
   - leave them to village fallback initialization
3. Use `ValidateScenarioTemplate` before runtime:
   - checks template references
   - checks role slot references in relationship seeds
   - reports unknown slot IDs and invalid count ranges

## Test Matrix

| ID | Scenario | Steps | Expected |
|----|----------|-------|----------|
| S1 | Scenario validation pass | Call `ValidateScenarioTemplate(ScenarioRowID, true)` before play. | Log shows `PASS`; no missing template errors. |
| S2 | Required roles spawn count | Press Play. Count NPCs tagged `VillageResident` and per slot tag `VillageSlot.*`. | Spawn count matches `RequiredRoles` plus rolled optional roles. |
| S3 | Village assignment override | Set `bForceVillageIDOverride=true` and scenario `VillageID=Village.Demo`. | Spawned NPC `UNPCDefinitionComponent::VillageID` equals scenario village ID. |
| S4 | Fixed name injection | Add `FixedNameIDs` to one required role. Press Play. | Matching spawned NPC has assigned `NameID` from fixed list (not randomized). |
| S5 | Key seed relationship applied | Author one seed `Lord -> Guard` with custom summary/type. Press Play. | Source NPC has seeded attitude + bond summary/type for target. |
| S6 | Seed bidirectional behavior | Set one seed with `bBidirectional=true`. Press Play. | Both directions materialize with expected initial settings. |
| S7 | Fallback for unseeded pairs | Do not author commoner-commoner seed. Press Play. | Unseeded pairs still get initial relation via village fallback, not empty. |
| S8 | Seed with slot prefix | Use `SourceSlotID=Guard`, `TargetSlotID=Commoner`. Press Play. | Applies to all `Guard_XX -> Commoner_XX` combinations. |
| S9 | Respawn behavior | Call `SpawnScenario` twice with `bDestroyPreviousSpawnedNPCsOnRespawn=true`. | Previous spawned residents are destroyed and replaced cleanly. |
| S10 | Profession diagnostics | Keep `bRunProfessionSanityCheckOnSpawn=true`, then Play. | Logs print each NPC with `Profession`, `GoalDirective`, `GoalActivity`, `UtilityActions`, `CurrentAction`. |

## Profession Self-Check Log Pattern

Expected line pattern:

```text
[VillageSpawner][Diag] NPC=<name> Template=<template> Profession=<id> GoalDirective=<tag> GoalActivity=<tag> UtilityActions=<num> CurrentAction=<action>
```

Failure signals:
- `Profession=None` when template expected a valid profession
- `UtilityActions=-1` (utility component missing)
- `UtilityActions=0` for professions that should expose actions

## Debug Checklist

1. Inspect runtime arrays/maps in spawner:
   - `SpawnedNPCs`
   - `SpawnedSlotMap`
2. Inspect each key NPC:
   - `UNPCDefinitionComponent::DefinitionTemplateID`
   - `UNPCDefinitionComponent::VillageID`
   - `UNPCDefinitionComponent::ProfessionID`
3. Inspect social state:
   - `UFactionReputationComponent::PersonalReputations`
   - `UFactionReputationComponent::SocialBonds`

## Pass Criteria

- All S1-S10 pass.
- No crash on spawn, respawn, or seed application.
- Key authored relationships are deterministic and fallback behavior remains active for non-key pairs.
