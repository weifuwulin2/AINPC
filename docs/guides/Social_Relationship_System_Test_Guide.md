# Social Relationship System Test Guide

This guide validates the NPC-to-NPC relationship MVP introduced in:
- `SocialTypes` (`ESocialBondType`, `FSocialBond`)
- `SocialTypes` (`FRelationshipSeedRow`)
- `FactionReputationComponent` (social bond map + threshold reflection + delegate)
- `FactionSubsystem` (seed matrix + seeded lookup)
- `CognitionComponent` (relationship context prompt injection)

For village-level registration/bootstrap validation, see:
- `docs/guides/Village_Social_Initialization_Test_Guide.md`

## Scope

Feature under test:
- Semantic social bonds built on top of numeric reputation.
- Stable relationship key resolution (`NameID` first, actor name fallback).
- Threshold-based reflection events.
- Prompt context injection from active target relationship summary.

Out of scope:
- Long-term save/load validation across sessions.
- Dreaming-driven LLM reflection rewrite (not implemented in this MVP).

## Preconditions

1. Project compiles and launches in PIE.
2. Two NPCs exist with:
   - `UFactionReputationComponent`
   - `UNPCDefinitionComponent` (recommended with valid `NameID`)
3. Optional but recommended:
   - `UNarrativeHistorySubsystem` enabled in world
   - `UCognitionComponent` attached to controller for prompt verification

## Test Matrix

| ID | Scenario | Steps to Reproduce | Expected Result |
|----|----------|-------------------|-----------------|
| T1 | Default relationship fallback | 1. Spawn NPC A and NPC B with no personal reputation and no social bond.<br>2. Call `GetAttitudeTowards(B)` from A. | Returns faction baseline attitude from `UFactionSubsystem` (or neutral fallback if not configured). |
| T2 | Create bond from reputation change | 1. From A, call `ModifyReputation(B, +20)` once.<br>2. Inspect `PersonalReputations` and `SocialBonds`. | `PersonalReputations[B]` updated; `SocialBonds[StableID(B)]` created/updated with non-empty type/salience/time. |
| T3 | Threshold crossing triggers reflection | 1. Force A->B from low to high tier (for example call `ModifyReputation` enough to cross Enemy/Rival/Friend boundary).<br>2. Observe NarrativeHistory logs/events. | A reflection event is recorded with `[Reflection] ...` text and `Social.Relationship` tag. |
| T4 | No threshold crossing does not reflect | 1. Apply small delta within same bond tier (for example +1 around existing value).<br>2. Check NarrativeHistory. | Relationship change is recorded; reflection event is NOT added. |
| T5 | Temporary combat policy does not corrupt stored relationship | 1. Put A and B in a state where `EvaluateCombatPolicy` would force neutral (e.g. scene safety tags).<br>2. Call `ModifyReputation(B, delta)`.<br>3. Remove scene safety and inspect stored value. | Stored relationship progression still follows persistent value logic, not temporary forced-neutral output. |
| T6 | Stable ID resolution via NameID | 1. Ensure B has valid `NameID`.<br>2. Call `GetStableSocialID(B)` from A. | Returns `NameID`, not transient runtime actor name. |
| T7 | Fallback ID path | 1. Remove/clear B `NameID`.<br>2. Call `GetStableSocialID(B)`. | Returns actor fallback `GetFName()` without crash. |
| T8 | Delegate emission | 1. Bind to `OnRelationshipChanged` on A's `UFactionReputationComponent`.<br>2. Trigger `ModifyReputation(B, delta)`. | Delegate fires once with correct source/target IDs, old/new attitude, and threshold flag. |
| T9 | Cognition prompt relationship injection | 1. Ensure A controller focus points to B.<br>2. Trigger `ProcessStimulus` on A.<br>3. Check logged prompt block. | Prompt contains `Relationship: ...` line derived from `GetRelationshipSummaryTowards(B)`. |
| T10 | Null safety | 1. Call `GetAttitudeTowards(nullptr)`, `GetRelationshipSummaryTowards(nullptr)`, `ModifyReputation(nullptr, +10)`. | No crash; returns safe defaults / no-op behavior. |
| T11 | Rapid updates stress | 1. Run loop `ModifyReputation(B, +1)` 10 times quickly.<br>2. Monitor logs and event count. | System remains stable; no crash; reflection events only appear on tier boundaries, not every call. |
| T12 | Fixed seed is applied on first query | 1. Add seed row A->B in relationship seed table.<br>2. Ensure no personal relation exists.<br>3. Call `GetAttitudeTowards(B)` from A. | Returned attitude equals seed value; bond can be observed in `SocialBonds` after first query. |
| T13 | Bidirectional seed behavior | 1. Add one row with `bBidirectional=true` for A->B.<br>2. Query A->B and B->A. | Both directions resolve to seeded values. |
| T14 | Same-faction default fallback | 1. Ensure same-faction pair has no explicit seed and no personal relation.<br>2. Query attitude. | Value matches `DefaultIntraFactionAttitude` (default `60`) or explicit self-entry if authored. |

## Suggested Debug Checks

1. Watch these runtime fields in debugger/details panel:
   - `PersonalReputations`
   - `SocialBonds`
2. Watch these output channels:
   - `NarrativeHistory` event log
   - cognition request logs (for relationship prompt injection)
3. Validate thresholds by sampling around values:
   - 15 / 35 / 55 / 70 / 85 / 95

## Pass Criteria

- All test cases T1-T14 pass.
- No runtime crash or invalid pointer access.
- Reflection event frequency matches threshold crossing rules.
- Prompt relationship line appears when focus target exists.
