# Token Optimization Changelog

## Summary

Refactored the LLM communication layer and prompt building logic to reduce per-request token cost by ~550-700 tokens on routine calls and ~250 tokens on high-priority calls. Also separated the LLM API into two distinct modes for cleaner architecture.

---

## Changes

### 1. LLMCommunicator: Dual-Mode API

**Files**: `LLM/LLMCommunicator.h`, `LLM/LLMCommunicator.cpp`

**Before**: Two methods with no semantic distinction:
- `SendRequest()` — hardcoded roleplay system prompt + JSON mode
- `SendRequestRaw()` — generic system prompt, same temperature, no token limits

**After**: Two purpose-built methods sharing a common HTTP layer:

| Method | Use Case | System Prompt | Temp | JSON Mode | MaxTokens |
|---|---|---|---|---|---|
| `SendRoleplayRequest()` | CognitionComponent ProcessStimulus | Static: Instructions + MentalState schema (cacheable) | 0.7 | ON | Unlimited |
| `SendFunctionalRequest()` | Dreaming, Target Selection, Social Reflection | Caller-provided (task-specific) | 0.3 default | Configurable | 256 default |

**Key changes**:
- `SendHTTPRequest()` — shared internal method handles all HTTP/JSON boilerplate
- `ExtractContentFromResponse()` — shared response parsing (was duplicated before)
- `BuildRoleplaySystemPrompt()` — static system prompt with Instructions + MentalState JSON schema
- Old `SendRequest`/`SendRequestRaw` kept as deprecated wrappers (no breaking change)

---

### 2. Instructions Moved to System Prompt (~250 tokens saved per call)

**File**: `Components/CognitionComponent.cpp`

**Before**: The 7-rule `InstructionsBlock` (~250 tokens) was assembled in `ProcessStimulus` and sent as part of the **user message** every call. This prevented prefix caching since user messages change every call.

**After**: Instructions are now part of `LLMCommunicator::BuildRoleplaySystemPrompt()`, placed in the **system message**. The system prompt is fully static (Instructions + JSON Schema) and cacheable by providers that support prefix caching.

The user message now only contains: Identity + Worldview + Context + Volatile.

---

### 3. Context LOD on Identity Block (~150 tokens saved on routine calls)

**File**: `Components/CognitionComponent.cpp` — `BuildIdentityBlock()`

**Before**: Full backstory always included (OCEAN traits, past event, mental scar, phobias, core values, social class).

**After**: `BuildIdentityBlock` takes a `bFullDetail` parameter:

- **Full detail** (`bFullDetail=true`): High-priority events, player speech, or `EContextLOD::Standard`. Includes complete backstory as before.
- **Simplified** (`bFullDetail=false`): Routine stimuli. Only includes:
  - NPC name + faction + role description
  - Behavioral guidelines (always needed for decision-making)
  - Zombie override (fundamental identity)

Skips: OCEAN traits, past event, mental scar, phobias, core values, social class, faction description.

---

### 4. Worldview Filtered to Relevant Factions (~50-100 tokens saved)

**File**: `Components/CognitionComponent.cpp` — `BuildWorldviewBlock()`

**Before**: Listed ALL faction relationships from `RuntimeFactionMatrix` (e.g., 8 lines for 8 factions).

**After**: Parses `SituationDescription` for `(Faction: X)` patterns (from `SensoryComponent::DescribeActorWithRelationship`). Only includes:
- Own faction ("Your Faction")
- Factions mentioned in the current stimulus

Falls back to listing all factions if no factions are extracted from the text (safety net).

**Signature change**: `BuildWorldviewBlock(FactionStr)` -> `BuildWorldviewBlock(FactionStr, SituationDescription)`

---

### 5. Conditional World State (~100-200 tokens saved on routine calls)

**File**: `Components/CognitionComponent.cpp` — `ProcessStimulus()`

**Before**: `NarrativeDirectorSubsystem::GetWorldStateDescription(3)` always fetched (3 history events + dead NPCs + faction population counts).

**After**: Only fetched when `bFullDetail` is true:
- High-priority stimulus (HOSTILE/DANGER keywords)
- Player speech
- `EContextLOD::Standard`

Routine stimuli get an empty `GlobalHistory` string.

---

### 6. Dreaming: Capped Memories + Functional API

**Files**: `Components/MemoryComponent.h/cpp`, `Components/CognitionComponent.cpp` — `StartDreaming()`

**Before**: `GetAllRecentMemoriesAsString()` dumped ALL memories (up to 30). Used `SendRequestRaw` with no token cap.

**After**:
- New method `GetTopMemoriesAsString(15)` returns top 15 memories sorted by importance score.
- Uses `SendFunctionalRequest` with:
  - Task-specific system prompt: "You are a memory consolidation AI..."
  - Temperature: 0.3 (deterministic)
  - MaxTokens: 256

**Savings**: ~200-400 input tokens (15 vs 30 memories) + capped output.

---

### 7. Target Selection: Functional API

**File**: `Components/CognitionComponent.cpp` — `SuggestTarget()`

**Before**: Used `SendRequestRaw` with no system prompt, no token cap, default temperature 0.7.

**After**: Uses `SendFunctionalRequest` with:
- System prompt: "You are a target selection AI. Return ONLY a name from the candidate list."
- Temperature: 0.1 (near-deterministic)
- MaxTokens: 32 (only need a name)
- Removed importance score from memory output (unnecessary for target selection)

---

### 8. Rate Limiting Moved Earlier

**File**: `Components/CognitionComponent.cpp` — `ProcessStimulus()`

**Before**: Rate limit check happened AFTER building all prompt blocks (wasted computation).

**After**: Rate limit check happens BEFORE any prompt construction. If rate limited, returns immediately without building Identity/Worldview/Context/Volatile blocks.

---

## Token Cost Comparison

### Per ProcessStimulus Call (Routine)

| Component | Before | After | Saved |
|---|---|---|---|
| Instructions (user msg) | ~250 | 0 (in system msg) | ~250 |
| Identity (full backstory) | ~250 | ~80 (simplified) | ~170 |
| Worldview (all factions) | ~150 | ~40 (filtered) | ~110 |
| World State | ~150 | 0 (skipped) | ~150 |
| Context + Volatile | ~200 | ~200 | 0 |
| **Total User Message** | **~1000** | **~320** | **~680** |

### Per ProcessStimulus Call (High-Priority / Player Speech)

| Component | Before | After | Saved |
|---|---|---|---|
| Instructions (user msg) | ~250 | 0 (in system msg) | ~250 |
| Identity (full) | ~250 | ~250 | 0 |
| Worldview (filtered) | ~150 | ~60 | ~90 |
| World State | ~150 | ~150 | 0 |
| **Total User Message** | **~1000** | **~660** | **~340** |

### Per Dreaming Call

| Before | After | Saved |
|---|---|---|
| 30 memories (~600 tok) | 15 memories (~300 tok) | ~300 input |
| Uncapped output | MaxTokens=256 | Bounded output |

### Per Target Selection Call

| Before | After | Saved |
|---|---|---|
| Uncapped output | MaxTokens=32 | Bounded output |
| Temp 0.7 | Temp 0.1 | More consistent results |

---

## Architecture Diagram (After)

```
CognitionComponent
  ├─ ProcessStimulus() ──→ LLMService->SendRoleplayRequest()
  │                         ├─ System: [Instructions + MentalState Schema] (STATIC, CACHED)
  │                         ├─ User: [Identity + Worldview + Context + Volatile] (DYNAMIC)
  │                         ├─ Temp: 0.7, JSON Mode: ON
  │                         └─ Response: FMentalState
  │
  ├─ StartDreaming()  ──→ LLMService->SendFunctionalRequest()
  │                         ├─ System: "Memory consolidation AI"
  │                         ├─ User: Top 15 memories
  │                         ├─ Temp: 0.3, MaxTokens: 256
  │                         └─ Response: JSON array of insights
  │
  └─ SuggestTarget()  ──→ LLMService->SendFunctionalRequest()
                            ├─ System: "Target selection AI"
                            ├─ User: Role + Plot + Memories + Candidates
                            ├─ Temp: 0.1, MaxTokens: 32
                            └─ Response: Single actor name
```
