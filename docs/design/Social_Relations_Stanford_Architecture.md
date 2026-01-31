# Feature Design: Dynamic Social Relations (Stanford Architecture)

## 1. Overview
This feature implements deep, evolving social relationships between NPCs based on the *Generative Agents* (Stanford Smallville) paper. It shifts the AI from static "Attitude Values" (0-100) to dynamic "Social Bonds" driven by memory, retrieval, and reflection.

**Core Philosophy**: Relationships are not just numbers; they are the sum of shared memories and synthesized insights.

## 2. Architecture Changes

### 2.1. Social Bond Structure (`FactionReputationComponent`)
We will upgrade the existing `PersonalReputations` map to store detailed bond structures instead of simple floats.

```cpp
UENUM(BlueprintType)
enum class ESocialBondStatus : uint8
{
    Stranger,
    Acquaintance,
    Friend,
    CloseFriend,
    Rival,
    Enemy,
    Nemesis
};

USTRUCT(BlueprintType)
struct FSocialBond
{
    GENERATED_BODY()

    // Base numeric value (Matches existing "Attitude" terminology)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Attitude = 50.0f; // 0-100

    // The semantic definition of the relationship
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESocialBondStatus Status = ESocialBondStatus::Stranger;

    // "Reflection" Result: A high-level summary of what I think of this person
    // e.g., "He is generous but talks too much about politics."
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString CoreImpression;

    // History tracking
    UPROPERTY()
    float LastInteractionTime = 0.0f;

    UPROPERTY()
    int32 InteractionCount = 0;
};
```

### 2.2. Enhanced Memory Retrieval (`MemoryComponent`)
The heart of the Stanford architecture is retrieving the *right* memories at the *right* time. We will implement the weighted scoring function:

$$Score = (\alpha \cdot Recency) + (\beta \cdot Importance) + (\gamma \cdot Relevance)$$

*   **Recency**: Exponential decay based on `GetTimeSeconds()`.
*   **Importance**: Integer (1-10), assigned upon memory creation (e.g., "Buying coffee" = 2, "Being attacked" = 9).
*   **Relevance**: Keyword matching or semantic similarity (if embedding available) between the Query Topic and Memory Content.

### 2.3. The Reflection Loop (`Action_Reflect`)
Relationships evolve through "Reflection". We will not process this every tick. Instead, we introduce a new **Utility Action: `Reflect`**.

*   **Trigger**:
    *   State is `Idle`.
    *   `UnprocessedMemories` count > Threshold (e.g., 5-10 events).
*   **Input**: The 10 most recent raw memories involving specific people.
*   **Process**: Prompt LLM to synthesize these details into a high-level insight.
*   **Output**:
    1.  Create a new `MemoryItem` (Type = Reflection).
    2.  Update `FSocialBond.CoreImpression`.
    3.  Reset `UnprocessedMemories` counter.

### 2.4. Information Diffusion (Gossip)
*   **Mechanism**: When executing `Action_Talk`, if the topic is *another person*, the listener generates a memory about the subject, citing the speaker as the source.
*   **Effect**: Allows reputation to spread without direct interaction.

## 3. Workflow Examples

### Scenario A: First Meeting
1.  **Event**: NPC A sees Player.
2.  **Lookup**: `FactionReputation` checks `SocialBonds`. Result: `nullptr`.
3.  **Action**: Create default Bond (`Stranger`, Affinity 50).
4.  **Memory**: Store "Saw [Player] at [Location]".

### Scenario B: Developing Friendship
1.  **Event**: NPC A and B have a conversation (Context: "Fishing").
2.  **Memory**: Store "B told me he loves bass fishing" (Importance: 3).
3.  **Reflect (Later)**:
    *   NPC A enters `Action_Reflect`.
    *   Retrieves recent memories: "Saw B", "B likes fishing", "B smiled".
    *   LLM Synthesis: "B is an outdoor enthusiast and friendly."
    *   Update Bond: Status -> `Acquaintance`, Impression -> "Outdoor enthusiast".
4.  **Next Meeting**:
    *   A sees B.
    *   Prompt generation includes Bond Impression ("Outdoor enthusiast").
    *   A says: "Catch anything good lately?" (Contextually relevant).

## 4. Risks & Mitigation
*   **Performance**: Retrieval calculation on every frame is too expensive.
    *   *Mitigation*: Cache `RelevantMemories` in `CognitionComponent` and only update when Context changes.
*   **LLM Cost**: Reflection consumes tokens.
    *   *Mitigation*: Only reflect when `UnprocessedMemories` exceeds threshold, and use "Batch Reflection" (reflect on the most important person first).
