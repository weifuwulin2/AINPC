# Ambient Dialogue System - User Guide

## 📖 Overview

The **Ambient Dialogue System** automatically triggers NPCs in narrative scenes to speak periodically, creating a more immersive and living atmosphere. NPCs will comment on their surroundings, their roles, and their current activities based on the scene context.

### Key Features:
- ⏰ **Automatic Timing**: Randomly triggers every 30-60 seconds (configurable)
- 🎲 **Random Selection**: Picks 1-2 NPCs per trigger
- 📍 **Player Proximity**: Only triggers when player is nearby (performance optimization)
- 🎭 **Context-Aware**: NPCs speak based on their role, activity, and personality
- 🛠️ **Fully Configurable**: All parameters can be adjusted per scene

---

## 🎯 How It Works

### Automatic Activation

When you activate a narrative scene using `ActivateScene(SquadID)`, the ambient dialogue system **automatically starts** if enabled (which it is by default).

### Trigger Conditions

For ambient dialogue to trigger, ALL of these must be true:
1. ✅ Scene is active (`bIsActive = true`)
2. ✅ Ambient dialogue is enabled (`bEnableAmbientDialogue = true`)
3. ✅ Player is within activation radius (default 2000 units from anchor or any NPC)
4. ✅ At least one NPC has a `CognitionComponent`

### What Happens When Triggered

1. System checks if player is nearby
2. If yes: Randomly selects 1-2 NPCs from the scene
3. Each selected NPC generates a brief (1-2 sentence) dialogue via LLM
4. Timer resets for next trigger (30-60 seconds later)
5. If no: Skip trigger, check again next time

---

## ⚙️ Configuration

### Default Settings (Per Scene)

```cpp
// In FNarrativeSceneSquad:
bool bEnableAmbientDialogue = true;           // Enabled by default
float AmbientDialogueIntervalMin = 30.0f;     // Min interval (seconds)
float AmbientDialogueIntervalMax = 60.0f;     // Max interval (seconds)
int32 AmbientSpeakersPerTrigger = 2;          // Number of NPCs that speak
float PlayerActivationRadius = 2000.0f;       // Distance check (cm)
```

### Method 1: Blueprint Configuration

Use the `ConfigureAmbientDialogue` function:

```cpp
UFUNCTION(BlueprintCallable)
void ConfigureAmbientDialogue(
    int32 SquadID,
    bool bEnabled,
    float MinInterval,
    float MaxInterval,
    int32 SpeakersPerTrigger,
    float ActivationRadius
);
```

**Blueprint Example:**
```
Event BeginPlay
└─ Spawn Scene At Anchor → SquadID
   └─ Configure Ambient Dialogue
       ├─ SquadID: [from above]
       ├─ Enabled: true
       ├─ Min Interval: 20.0
       ├─ Max Interval: 40.0
       ├─ Speakers Per Trigger: 1
       └─ Activation Radius: 1500.0
```

### Method 2: C++ Configuration

```cpp
// After spawning a scene
int32 SquadID = NarrativeSquadSubsystem->SpawnSceneAtAnchor(...);

// Configure before activation
NarrativeSquadSubsystem->ConfigureAmbientDialogue(
    SquadID,
    true,      // Enable
    20.0f,     // Min interval
    40.0f,     // Max interval
    1,         // 1 speaker per trigger
    1500.0f    // Activation radius
);

// Activate scene (ambient dialogue starts automatically)
NarrativeSquadSubsystem->ActivateScene(SquadID);
```

### Method 3: Disable for Specific Scenes

```cpp
// Disable ambient dialogue for a specific scene
NarrativeSquadSubsystem->ConfigureAmbientDialogue(
    SquadID,
    false,  // Disable
    30.0f, 30.0f, 1, 2000.0f  // Other params don't matter if disabled
);
```

---

## 🧪 Testing

### Manual Trigger (Debug)

For testing purposes, you can manually trigger ambient dialogue:

```cpp
UFUNCTION(BlueprintCallable)
void TriggerAmbientDialogueNow(int32 SquadID);
```

This bypasses the timer and player proximity check.

**Blueprint:**
```
On Key Press [F9]
└─ Trigger Ambient Dialogue Now
   └─ SquadID: [your scene ID]
```

### Recommended Test Scenario

1. **Spawn a narrative scene** with multiple NPCs
2. **Activate the scene**
3. **Stand near the scene** (within 2000 units)
4. **Wait 30-60 seconds**
5. **Observe**: NPCs should speak based on their roles

### Debug Logs

Enable verbose logging to see ambient dialogue activity:

```
LogTemp: [AmbientDialogue] Configured Squad 0: Enabled=1, Interval=[30.0-60.0], Speakers=2, Radius=2000
LogTemp: [AmbientDialogue] Squad 0: Next trigger in 45.3 seconds
LogTemp: [AmbientDialogue] Squad 0: Triggering 2/5 speakers (Player nearby)
LogTemp: [AmbientDialogue] BP_OrcSlave_1 triggered ambient speech (Activity: idle)
```

---

## 💡 Example Scenarios

### Scenario 1: Orc Manor Liberation

```cpp
// Scene: Orcs have taken over the manor
int32 SquadID = SpawnSceneAtAnchor(Anchor, SceneTable, "Scene_Manor_Liberation", NPCTable);

// Make NPCs chatty (frequent, 1 speaker at a time)
ConfigureAmbientDialogue(SquadID, true, 15.0f, 30.0f, 1, 2500.0f);

ActivateScene(SquadID);
```

**Expected Ambient Dialogue:**
- Orc Leader: "This place is ours now. The humans will never take it back."
- Orc Warrior: "I'm keeping watch. No one gets past me."
- Orc Leader: "We should celebrate our freedom tonight."

### Scenario 2: Silent Tension Scene

```cpp
// Scene: Hostage situation, very quiet
int32 SquadID = SpawnSceneAtAnchor(Anchor, SceneTable, "Scene_Hostage", NPCTable);

// Rare, whispered comments
ConfigureAmbientDialogue(SquadID, true, 60.0f, 120.0f, 1, 1500.0f);

ActivateScene(SquadID);
```

**Expected Ambient Dialogue (rare):**
- Robber: *whispers* "Keep quiet and no one gets hurt."
- Hostage: *trembling* "Please... just let us go..."

### Scenario 3: Disable for Critical Dialogue

```cpp
// Scene: Important plot dialogue happening
int32 SquadID = SpawnSceneAtAnchor(Anchor, SceneTable, "Scene_KingCourt", NPCTable);

// Disable ambient dialogue to avoid interrupting main dialogue
ConfigureAmbientDialogue(SquadID, false, 30.0f, 60.0f, 2, 2000.0f);

ActivateScene(SquadID);
```

---

## 📊 Performance Considerations

### LLM Call Frequency

- **Default**: ~1-2 LLM calls every 30-60 seconds
- **Only when player is nearby** (no wasted API calls)
- **Per scene**: Each scene has its own timer

### Optimization Tips

1. **Increase Interval**: For less important scenes, use 60-120 second intervals
2. **Reduce Speaker Count**: Use 1 speaker per trigger instead of 2
3. **Smaller Radius**: Use 1000-1500 units for indoor scenes
4. **Disable When Not Needed**: For cutscenes or important dialogues

### Recommended Settings by Scene Type

| Scene Type | Interval | Speakers | Radius |
|------------|----------|----------|--------|
| Busy Market | 15-30s | 2 | 2500 |
| Manor/Interior | 30-60s | 1-2 | 2000 |
| Outdoor Camp | 45-90s | 1 | 3000 |
| Tense Standoff | 60-120s | 1 | 1500 |
| Cutscene | Disabled | - | - |

---

## 🎨 Customizing Prompts

The ambient dialogue prompt is constructed in `RequestAmbientDialogue()`:

```cpp
FString AmbientPrompt = FString::Printf(
    TEXT("You are currently in this scene: %s\n"
         "Your role in this scene: %s\n"
         "Current activity: %s\n\n"
         "Generate a brief ambient line (1-2 sentences) that reflects your character and current situation. "
         "This is NOT to advance the plot, just to add atmosphere and immersion. "
         "Feel free to comment on your surroundings, your feelings, or what you're doing. "
         "Keep it natural and in-character."),
    *Squad->PlotOutline,
    *Squad->MemberRoles.FindRef(Speaker),
    *CurrentActivity
);
```

### To Customize:

1. **Open**: `NarrativeSquadSubsystem.cpp`
2. **Find**: `RequestAmbientDialogue` function (line ~550)
3. **Edit**: The `AmbientPrompt` string
4. **Add More Context**: E.g., emotional state, nearby objects, time of day

**Example Enhancement:**
```cpp
// Add emotional state
FString EmotionalState = GetEmotionalState(Speaker); // You'd implement this

// Get current activity from GoalComponent
FString CurrentActivity = "idle";
if (UGoalComponent* GoalComp = Speaker->FindComponentByClass<UGoalComponent>())
{
    FGameplayTag ActivityTag = GoalComp->GetScheduledActivity();
    if (ActivityTag.IsValid())
    {
        CurrentActivity = ActivityTag.ToString(); // e.g. "Interaction.Eat", "Directive.Work"
    }
}

FString AmbientPrompt = FString::Printf(
    TEXT("Scene: %s\nRole: %s\nActivity: %s\nMood: %s\n\n"
         "Say something brief and in-character."),
    *Squad->PlotOutline,
    *Squad->MemberRoles.FindRef(Speaker),
    *CurrentActivity,
    *EmotionalState
);
```

---

## 🐛 Troubleshooting

### NPCs Not Speaking

**Check:**
1. Is the scene active? (`bIsActive = true`)
2. Is ambient dialogue enabled? (Default: `true`)
3. Is the player nearby? (Within `PlayerActivationRadius`)
4. Do NPCs have `CognitionComponent`?
5. Check logs for `[AmbientDialogue]` messages

**Debug Command:**
```cpp
TriggerAmbientDialogueNow(SquadID); // Force trigger
```

### Too Frequent / Too Rare

**Adjust intervals:**
```cpp
ConfigureAmbientDialogue(SquadID, true, 
    45.0f,  // Min interval (increase to reduce frequency)
    90.0f,  // Max interval
    1, 2000.0f);
```

### NPCs Speak When Player is Far Away

**Check:**
- `PlayerActivationRadius` might be too large
- Reduce to 1500-2000 for most scenes

### Ambient Dialogue Continues After Scene Ends

**This should never happen**. The timer is cleared in `EndScene()`. If it does:
- File a bug report
- Check that `EndScene()` is being called

---

## 📝 API Reference

### Functions

```cpp
// Configure ambient dialogue settings
void ConfigureAmbientDialogue(
    int32 SquadID, 
    bool bEnabled, 
    float MinInterval, 
    float MaxInterval, 
    int32 SpeakersPerTrigger, 
    float ActivationRadius
);

// Manually trigger for testing
void TriggerAmbientDialogueNow(int32 SquadID);
```

### Internal Functions (Advanced Users)

```cpp
// Start the timer (called automatically by ActivateScene)
void StartAmbientDialogue(int32 SquadID);

// Timer callback (don't call directly)
void TriggerAmbientDialogue(int32 SquadID);

// Generate dialogue for a specific NPC (don't call directly)
void RequestAmbientDialogue(AActor* Speaker, const FNarrativeSceneSquad* Squad);

// Check if player is near the scene
bool IsPlayerNearScene(const FNarrativeSceneSquad* Squad) const;
```

---

## 🎓 Best Practices

### DO ✅

- Use ambient dialogue for **background atmosphere**
- Keep intervals **30-60 seconds** for most scenes
- **Disable** during important plot dialogues
- Test with **player nearby** to hear the dialogue
- Use **shorter intervals** for busy/lively scenes (markets, taverns)
- Use **longer intervals** for tense/quiet scenes (sneaking, hostage)

### DON'T ❌

- Don't use ambient dialogue to **advance the plot**
- Don't set intervals **below 15 seconds** (too frequent)
- Don't make `ActivationRadius` too large (performance)
- Don't forget to **end scenes** when done (timer will keep running)
- Don't expect ambient dialogue without player nearby

---

## 🔮 Future Enhancements

Potential improvements (not yet implemented):

1. **Emotional State Integration**: NPCs comment based on current emotion
2. **NPC-to-NPC Dialogue**: Two NPCs can have short exchanges
3. **Proximity-Based Volume**: Dialogue fades with distance
4. **Activity-Specific Comments**: Custom dialogue for specific actions
5. **Time-of-Day Context**: Different comments at different times

---

## 📚 Related Documentation

- [Narrative System Design](../design/Narrative_System_Design.md)
- [Narrative Verification Guide](../design/Narrative_Verification.md)
- [Orc Liberation Story Example](../design/Orc_Liberation_Story.md)
- [CognitionComponent Guide](./Cognition_Component_Guide.md) *(if exists)*

---

## 💬 Questions?

If you have questions or suggestions for the Ambient Dialogue system:

1. Check the troubleshooting section above
2. Review the log output (`[AmbientDialogue]` tags)
3. Test with `TriggerAmbientDialogueNow()` manually
4. Check the source code: `NarrativeSquadSubsystem.cpp` (line ~440)

**Happy storytelling!** 🎭
