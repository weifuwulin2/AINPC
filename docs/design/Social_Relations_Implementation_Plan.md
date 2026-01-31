# Feature Plan: Social Relations Implementation

## Phase 1: Foundation (Data Structures)
**Goal**: Upgrade `FactionReputationComponent` to support complex bonds without breaking existing logic.

*   [ ] **Task 1.1**: Define `FSocialBond` struct in `SocialTypes.h` (or locally in FactionReputation).
*   [ ] **Task 1.2**: Update `UFactionReputationComponent`:
    *   Add `TMap<AActor*, FSocialBond> SocialBonds`.
    *   Refactor `GetAttitudeTowards(Target)` to look up `SocialBonds` first. If bond exists, return `Bond.Attitude`. If not, fall back to lookup table.
*   [ ] **Task 1.3**: Add Helper function `GetImpression(Target)` that returns the string description of the relationship.
*   [ ] **Integration Test**: Verify NPC still attacks Enemies and ignores Neutrals correctly using the new structure.

## Phase 2: Enhanced Memory Retrieval
**Goal**: Implement the "Retrieval Score" to fetch relevant context for LLM prompts.

*   [ ] **Task 2.1**: Update `FMemoryItem` struct:
    *   Add `int32 Importance` (0-10, default 1).
    *   Add `TArray<FString> Keywords`.
*   [ ] **Task 2.2**: Implement `RetrieveRelevantMemories` in `MemoryComponent`:
    *   Input: `Query` (string), `Count` (int).
    *   Algorithm: Calculate scores for all memories based on Recency, Importance, and textual overlap.
    *   Return: Top N memories.
*   [ ] **Unit Test**: Manually inject memories and verify that a query for "Combat" returns attack-related memories over recent trivial ones.

## Phase 3: The Reflection Action
**Goal**: Create the offline processing loop that synthesizes memories into insights.

*   [ ] **Task 3.1**: Create `Action_Reflect` (UUtilityActionBase).
    *   Consideration: `UnprocessedMemoryCount` (Curve: Linear).
    *   Priority: `Low` (only runs when Safe/Idle).
*   [ ] **Task 3.2**: Implement `Reflect` Logic:
    *   Find the person with the most unprocessed interaction memories.
    *   Construct Prompt: "Summarize relationship with [Name] based on: [List of Memories]".
    *   (Mock LLM response first for testing).
*   [ ] **Task 3.3**: Apply Reflection:
    *   Create new Memory (Insight).
    *   Update `SocialBond.CoreImpression`.
    *   Clear unprocessed flags.

## Phase 4: Integration & Seeding
**Goal**: Connect the Brain (LLM) to the Body (Actions).

*   [ ] **Task 4.1**: Update `CognitionComponent` Prompt Generation.
    *   Inject `SocialBond.CoreImpression` into the "Who is this person?" section of the prompt.
    *   Inject `RetrieveRelevantMemories` result into the "Context" section.
*   [ ] **Task 4.2**: Create Social Graph DataAsset (Seeding).
    *   Allow designers to configure initial `SocialBonds` (e.g., "A is B's Brother").
    *   Loader logic in `FactionReputationComponent::BeginPlay`.

## Phase 5: Verification (The "Valle" Test)
*   [ ] **Scenario**:
    1.  Spawn NPC A and B.
    2.  Set Relation: Strangers.
    3.  Player forces interaction (chat).
    4.  Wait for `Action_Reflect` to run.
    5.  Check `SocialBond`: Status changed? Impression updated?
    6.  Force interaction again: Does NPC A reference the previous chat?
