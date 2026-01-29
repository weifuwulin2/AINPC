# Universal Target Selection System Design

## 问题分析

用户提出的核心改进：
1. **通用化**：目标选择不应该只在 `Action_Attack` 中，应该是一个通用服务
2. **LLM 增强**：Cognition 可以调用 LLM 来根据记忆、性格、叙事上下文选择最合理的目标

## 架构设计

### 方案：双层目标选择系统

```
┌─────────────────────────────────────────────────┐
│          Target Selection Subsystem              │
├─────────────────────────────────────────────────┤
│  Layer 1: Rule-Based Scoring (快速路径)         │
│  - Memory-driven priority                        │
│  - Faction reputation                            │
│  - Threat assessment                             │
│  - Distance/Health scoring                       │
├─────────────────────────────────────────────────┤
│  Layer 2: LLM-Enhanced Selection (智能路径)     │
│  - CognitionComponent::SuggestTarget()           │
│  - Context-aware reasoning                       │
│  - Narrative consistency                         │
│  - Personality-driven choice                     │
└─────────────────────────────────────────────────┘
```

---

## 实现方案

### 1. 创建通用目标选择子系统

**文件**: `TargetSelectionSubsystem.h/.cpp`

```cpp
// TargetSelectionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TargetSelectionSubsystem.generated.h"

/** Context for target selection */
UENUM(BlueprintType)
enum class ETargetSelectionContext : uint8
{
    Combat      UMETA(DisplayName = "Combat"),           // Attack action
    Social      UMETA(DisplayName = "Social"),           // Speak/Gossip action
    Follow      UMETA(DisplayName = "Follow"),           // Follow action
    Trade       UMETA(DisplayName = "Trade"),            // Trade/Barter
    Help        UMETA(DisplayName = "Help"),             // Assist ally
    Investigate UMETA(DisplayName = "Investigate")       // Investigate event
};

/** Configuration for target selection */
USTRUCT(BlueprintType)
struct FTargetSelectionConfig
{
    GENERATED_BODY()

    /** Use LLM for intelligent selection (slow but narrative-aware) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseLLMEnhancement = false;

    /** Maximum distance to consider targets */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxDistance = 3000.0f;

    /** Minimum reputation to be considered friendly (-100 to 100) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FriendlyReputationThreshold = 50.0f;

    /** Minimum reputation to be considered hostile */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HostileReputationThreshold = -50.0f;
};

/**
 * Universal target selection service.
 * Provides intelligent target selection for all actions that need a target.
 */
UCLASS()
class AINPC_API UTargetSelectionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /**
     * Select best target for the given context.
     * @param Controller - The AI controller requesting target
     * @param Context - What is the target needed for? (Combat, Social, etc.)
     * @param Config - Configuration for selection criteria
     * @return Best target actor, or nullptr if none found
     */
    UFUNCTION(BlueprintCallable, Category = "AI|Target Selection")
    AActor* SelectTarget(
        AAIController* Controller,
        ETargetSelectionContext Context,
        const FTargetSelectionConfig& Config
    );

    /**
     * Get all valid target candidates for the given context.
     * Filters out dead, invalid, or inappropriate targets.
     */
    UFUNCTION(BlueprintCallable, Category = "AI|Target Selection")
    TArray<AActor*> GetTargetCandidates(
        AAIController* Controller,
        ETargetSelectionContext Context,
        const FTargetSelectionConfig& Config
    );

    /** Clear cached results for a specific controller (e.g., when situation changes dramatically) */
    UFUNCTION(BlueprintCallable, Category = "AI|Target Selection")
    void InvalidateCache(AAIController* Controller);

private:
    /** Rule-based scoring for quick target selection */
    AActor* SelectTargetByScoring(
        AAIController* Controller,
        const TArray<AActor*>& Candidates,
        ETargetSelectionContext Context,
        const FTargetSelectionConfig& Config
    );

    /** LLM-enhanced selection for narrative consistency */
    AActor* SelectTargetByLLM(
        AAIController* Controller,
        const TArray<AActor*>& Candidates,
        ETargetSelectionContext Context
    );

    /** Calculate score for a target in given context */
    float CalculateTargetScore(
        AActor* Target,
        APawn* MyPawn,
        ETargetSelectionContext Context,
        UCognitionComponent* Cognition,
        UFactionReputationComponent* FactionComp
    );

    /** Check if target is valid for the given context */
    bool IsValidTarget(
        AActor* Target,
        APawn* MyPawn,
        ETargetSelectionContext Context,
        UFactionReputationComponent* FactionComp
    );

    // ✅ Performance Optimization: Caching
    
    /** Cache key for target selection results */
    struct FTargetCacheKey
    {
        AAIController* Controller;
        ETargetSelectionContext Context;
        float Timestamp;

        bool operator==(const FTargetCacheKey& Other) const
        {
            return Controller == Other.Controller && Context == Other.Context;
        }

        friend uint32 GetTypeHash(const FTargetCacheKey& Key)
        {
            return HashCombine(GetTypeHash(Key.Controller), (uint32)Key.Context);
        }
    };

    /** Cached target selection results */
    TMap<FTargetCacheKey, TWeakObjectPtr<AActor>> TargetCache;

    /** Cache validity duration (seconds) */
    UPROPERTY(Config)
    float CacheDuration = 5.0f;

    /** Check if cached result is still valid */
    bool IsCacheValid(const FTargetCacheKey& Key) const;

    /** Get cached target if valid */
    AActor* GetCachedTarget(AAIController* Controller, ETargetSelectionContext Context);

    /** Cache a target selection result */
    void CacheTarget(AAIController* Controller, ETargetSelectionContext Context, AActor* Target);
};
```

---

### 2. LLM 增强的目标选择

**在 CognitionComponent 中添加**：

```cpp
// CognitionComponent.h
public:
    /**
     * Ask LLM to suggest best target from candidates.
     * @param Candidates - List of potential targets (with smart names)
     * @param Context - What is the target needed for?
     * @return Smart name of suggested target
     */
    UFUNCTION(BlueprintCallable, Category = "Cognition|LLM")
    FString SuggestTarget(
        const TArray<FString>& CandidateNames,
        const FString& SelectionContext
    );
```

**实现逻辑**：

```cpp
// CognitionComponent.cpp
FString UCognitionComponent::SuggestTarget(
    const TArray<FString>& CandidateNames,
    const FString& SelectionContext
)
{
    if (CandidateNames.Num() == 0) return "";

    // 1. Build LLM Prompt
    FString Prompt = FString::Printf(
        TEXT("You are %s (%s). You need to choose a target for: %s.\n\n"),
        *PersonalityID.ToString(),
        *PersonalityDescription,  // "A brave orc warrior"
        *SelectionContext         // "combat", "gossip", "follow"
    );

    // 2. Add Narrative Context
    if (!CurrentPlotContext.IsEmpty())
    {
        Prompt += FString::Printf(
            TEXT("Current narrative context: %s\n\n"),
            *CurrentPlotContext
        );
    }

    // 3. Add Memory Context
    Prompt += TEXT("Your recent memories:\n");
    for (const FMemoryEntry& Memory : MemoryBuffer)
    {
        if (Memory.Salience > 5.0f)  // Only important memories
        {
            Prompt += FString::Printf(
                TEXT("- %s (Salience: %.1f)\n"),
                *Memory.Description,
                *Memory.Salience
            );
        }
    }

    // 4. List Candidates
    Prompt += TEXT("\nAvailable targets:\n");
    for (const FString& Name : CandidateNames)
    {
        Prompt += FString::Printf(TEXT("- %s\n"), *Name);
    }

    // 5. Ask LLM
    Prompt += FString::Printf(
        TEXT("\nWho should you choose for %s? Reply with ONLY the name from the list above. Consider your personality, memories, and current situation."),
        *SelectionContext
    );

    // 6. Call LLM
    FString LLMResponse = CallLLM(Prompt, 50);  // Max 50 tokens

    // 7. Parse Response (find which candidate was mentioned)
    for (const FString& Name : CandidateNames)
    {
        if (LLMResponse.Contains(Name))
        {
            UE_LOG(LogAINPCBrain, Warning, 
                TEXT("[%s] LLM Selected Target: %s for %s (Reason: %s)"),
                *PersonalityID.ToString(),
                *Name,
                *SelectionContext,
                *LLMResponse
            );
            return Name;
        }
    }

    // 8. Fallback: Return first candidate
    UE_LOG(LogAINPCBrain, Warning, 
        TEXT("[%s] LLM failed to select, using first candidate: %s"),
        *PersonalityID.ToString(),
        *CandidateNames[0]
    );
    return CandidateNames[0];
}
```

---

### 3. 在 TargetSelectionSubsystem 中集成

```cpp
// TargetSelectionSubsystem.cpp
AActor* UTargetSelectionSubsystem::SelectTarget(
    AAIController* Controller,
    ETargetSelectionContext Context,
    const FTargetSelectionConfig& Config
)
{
    if (!Controller) return nullptr;

    // ✅ Step 1: Check Cache First
    AActor* CachedTarget = GetCachedTarget(Controller, Context);
    if (CachedTarget)
    {
        // Verify cached target is still valid (not dead, still in range)
        if (!CachedTarget->ActorHasTag("Dead"))
        {
            APawn* MyPawn = Controller->GetPawn();
            if (MyPawn)
            {
                float Distance = FVector::Dist(MyPawn->GetActorLocation(), CachedTarget->GetActorLocation());
                if (Distance <= Config.MaxDistance)
                {
                    NARRATIVE_LOG(Verbose, "Using cached target: %s for context: %d", 
                        *CachedTarget->GetName(), (int32)Context);
                    return CachedTarget;
                }
            }
        }
        
        // Cache invalid, clear it
        InvalidateCache(Controller);
    }

    // ✅ Step 2: Get Candidates
    TArray<AActor*> Candidates = GetTargetCandidates(Controller, Context, Config);
    
    if (Candidates.Num() == 0)
    {
        NARRATIVE_LOG(Warning, "No valid target candidates found for context: %d", (int32)Context);
        return nullptr;
    }

    AActor* SelectedTarget = nullptr;

    // ✅ Step 3: Performance-Aware Selection Strategy
    bool bShouldUseLLM = Config.bUseLLMEnhancement;

    // 🔥 Combat Context: Force Rule-Based (Real-time requirement)
    if (Context == ETargetSelectionContext::Combat)
    {
        bShouldUseLLM = false;
        NARRATIVE_LOG(Verbose, "Combat context detected, forcing rule-based selection for performance");
    }

    // 📊 Too Many Candidates: Use Rule-Based (LLM token limit)
    if (Candidates.Num() > 10 && bShouldUseLLM)
    {
        NARRATIVE_LOG(Warning, "Too many candidates (%d), falling back to rule-based", Candidates.Num());
        bShouldUseLLM = false;
    }

    // ✅ Step 4: Select Target
    if (bShouldUseLLM)
    {
        // 🧠 LLM-Enhanced Selection (Slow but narrative-aware)
        SelectedTarget = SelectTargetByLLM(Controller, Candidates, Context);
        
        // Fallback if LLM fails
        if (!SelectedTarget)
        {
            NARRATIVE_LOG(Warning, "LLM selection failed, falling back to rule-based");
            SelectedTarget = SelectTargetByScoring(Controller, Candidates, Context, Config);
        }
    }
    else
    {
        // ⚡ Rule-Based Selection (Fast)
        SelectedTarget = SelectTargetByScoring(Controller, Candidates, Context, Config);
    }

    // ✅ Step 5: Cache Result
    if (SelectedTarget)
    {
        CacheTarget(Controller, Context, SelectedTarget);
    }

    return SelectedTarget;
}

// ✅ Cache Implementation

AActor* UTargetSelectionSubsystem::GetCachedTarget(AAIController* Controller, ETargetSelectionContext Context)
{
    FTargetCacheKey Key;
    Key.Controller = Controller;
    Key.Context = Context;
    Key.Timestamp = GetWorld()->GetTimeSeconds();

    if (IsCacheValid(Key))
    {
        TWeakObjectPtr<AActor>* CachedPtr = TargetCache.Find(Key);
        if (CachedPtr && CachedPtr->IsValid())
        {
            return CachedPtr->Get();
        }
    }

    return nullptr;
}

void UTargetSelectionSubsystem::CacheTarget(AAIController* Controller, ETargetSelectionContext Context, AActor* Target)
{
    if (!Target) return;

    FTargetCacheKey Key;
    Key.Controller = Controller;
    Key.Context = Context;
    Key.Timestamp = GetWorld()->GetTimeSeconds();

    TargetCache.Add(Key, Target);
    
    NARRATIVE_LOG(Verbose, "Cached target: %s for context: %d (valid for %.1fs)", 
        *Target->GetName(), (int32)Context, CacheDuration);
}

bool UTargetSelectionSubsystem::IsCacheValid(const FTargetCacheKey& Key) const
{
    const TWeakObjectPtr<AActor>* CachedPtr = TargetCache.Find(Key);
    if (!CachedPtr || !CachedPtr->IsValid())
    {
        return false;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    float Age = CurrentTime - Key.Timestamp;

    return Age < CacheDuration;
}

void UTargetSelectionSubsystem::InvalidateCache(AAIController* Controller)
{
    // Remove all cached entries for this controller
    TArray<FTargetCacheKey> KeysToRemove;
    
    for (const auto& Pair : TargetCache)
    {
        if (Pair.Key.Controller == Controller)
        {
            KeysToRemove.Add(Pair.Key);
        }
    }

    for (const FTargetCacheKey& Key : KeysToRemove)
    {
        TargetCache.Remove(Key);
    }

    NARRATIVE_LOG(Verbose, "Invalidated cache for controller: %s", 
        *Controller->GetName());
}
```

AActor* UTargetSelectionSubsystem::SelectTargetByLLM(
    AAIController* Controller,
    const TArray<AActor*>& Candidates,
    ETargetSelectionContext Context
)
{
    APawn* MyPawn = Controller->GetPawn();
    if (!MyPawn) return nullptr;

    UCognitionComponent* Cognition = MyPawn->FindComponentByClass<UCognitionComponent>();
    if (!Cognition)
    {
        NARRATIVE_LOG(Warning, "No CognitionComponent, falling back to scoring");
        FTargetSelectionConfig FallbackConfig;
        return SelectTargetByScoring(Controller, Candidates, Context, FallbackConfig);
    }

    // 1. Convert Actors to Smart Names
    TArray<FString> CandidateNames;
    TMap<FString, AActor*> NameToActorMap;
    
    for (AActor* Actor : Candidates)
    {
        FString SmartName = AINPCHelpers::GetSmartActorName(Actor);
        CandidateNames.Add(SmartName);
        NameToActorMap.Add(SmartName, Actor);
    }

    // 2. Get Context String
    FString ContextString;
    switch (Context)
    {
        case ETargetSelectionContext::Combat:
            ContextString = "combat";
            break;
        case ETargetSelectionContext::Social:
            ContextString = "social interaction";
            break;
        case ETargetSelectionContext::Follow:
            ContextString = "following someone";
            break;
        case ETargetSelectionContext::Trade:
            ContextString = "trading";
            break;
        default:
            ContextString = "unknown action";
    }

    // 3. Ask LLM
    FString SelectedName = Cognition->SuggestTarget(CandidateNames, ContextString);

    // 4. Find Actor
    AActor** FoundActor = NameToActorMap.Find(SelectedName);
    if (FoundActor && *FoundActor)
    {
        NARRATIVE_LOG(Log, "LLM selected target: %s for context: %s", 
            *SelectedName, *ContextString);
        return *FoundActor;
    }

    // 5. Fallback to scoring
    NARRATIVE_LOG(Warning, "LLM selection failed, falling back to scoring");
    FTargetSelectionConfig FallbackConfig;
    return SelectTargetByScoring(Controller, Candidates, Context, FallbackConfig);
}
```

---

## 使用示例

### 在 Action_Attack 中使用

```cpp
// Action_Attack.cpp
void UAction_Attack::Enter_Implementation(AAIController* Controller)
{
    Super::Enter_Implementation(Controller);
    
    if (!Controller) return;

    OwningController = Controller;
    
    // ✅ 使用通用目标选择系统
    UTargetSelectionSubsystem* TargetSystem = 
        GetWorld()->GetSubsystem<UTargetSelectionSubsystem>();
    
    if (TargetSystem)
    {
        FTargetSelectionConfig Config;
        Config.bUseLLMEnhancement = true;  // 启用 LLM（可从 DataTable 配置）
        Config.MaxDistance = 3000.0f;
        Config.HostileReputationThreshold = -30.0f;
        
        TargetActor = TargetSystem->SelectTarget(
            Controller,
            ETargetSelectionContext::Combat,
            Config
        );
    }
    
    if (!TargetActor)
    {
        NARRATIVE_LOG(Warning, "No valid combat target found!");
        return;
    }
    
    Controller->SetFocus(TargetActor);
    NARRATIVE_LOG(Log, "⚔️ ATTACK ACTION - Target: %s (Intelligent Selection)", 
        *TargetActor->GetName());
}
```

### 在 Action_Speak 中使用

```cpp
// Action_Speak.cpp
void UAction_Speak::Enter_Implementation(AAIController* Controller)
{
    Super::Enter_Implementation(Controller);
    
    UTargetSelectionSubsystem* TargetSystem = 
        GetWorld()->GetSubsystem<UTargetSelectionSubsystem>();
    
    if (TargetSystem)
    {
        FTargetSelectionConfig Config;
        Config.bUseLLMEnhancement = true;  // LLM 选择最有话题的人
        Config.MaxDistance = 800.0f;       // 社交距离较近
        Config.FriendlyReputationThreshold = 0.0f;  // 中立即可
        
        AActor* SocialTarget = TargetSystem->SelectTarget(
            Controller,
            ETargetSelectionContext::Social,
            Config
        );
        
        if (SocialTarget)
        {
            // LLM 会基于记忆、性格选择最适合交谈的人
            InitiateDialogue(SocialTarget);
        }
    }
}
```

---

## LLM Prompt 示例

### Combat 场景

```
You are Orc_Guard_12 (A loyal guard protecting the mining camp). 
You need to choose a target for: combat.

Current narrative context: Guards are on high alert. A stranger has been spotted near the camp.

Your recent memories:
- Player killed Orc_Worker_5 (Salience: 9.5)
- Player is sneaking near the mine entrance (Salience: 8.2)
- Elf_Scout_3 was seen in the forest (Salience: 6.1)

Available targets:
- Player
- Elf_Scout_3
- Wolf_2

Who should you choose for combat? Reply with ONLY the name from the list above.
```

**LLM Response**: `Player`  
**理由**: 记忆中 Player 杀了队友，复仇优先级最高

### Social 场景

```
You are Merchant_Anna (A friendly trader who loves gossip). 
You need to choose a target for: social interaction.

Your recent memories:
- Blacksmith_Tom told you about the dragon sighting (Salience: 7.0)
- Guard_John arrested a thief yesterday (Salience: 6.5)
- Player bought expensive armor (Salience: 5.0)

Available targets:
- Blacksmith_Tom
- Guard_John
- Player

Who should you choose for social interaction? Reply with ONLY the name from the list above.
```

**LLM Response**: `Blacksmith_Tom`  
**理由**: 性格喜欢八卦，Tom 之前分享了龙的消息，有话题延续性

---

## 优势

### 1. 通用性
- 所有需要 target 的 Action 都可以使用
- 统一的评分逻辑，易于维护

### 2. LLM 增强
- 考虑叙事上下文和性格
- 选择更符合剧情的目标
- 提升沉浸感

### 3. 灵活配置
- 可通过 `bUseLLMEnhancement` 权衡性能和智能度
- 不同 Context 有不同的评分权重

### 4. 可扩展
- 添加新的 Context 类型很容易
- 可以为特定场景定制评分算法

---

## 实现优先级

1. **Phase 1**: 实现 `TargetSelectionSubsystem` 基础框架
2. **Phase 2**: 添加 Rule-Based Scoring
3. **Phase 3**: 在 `CognitionComponent` 中实现 `SuggestTarget()`
4. **Phase 4**: 集成 LLM-Enhanced Selection
5. **Phase 5**: 重构现有 Actions 使用新系统

---

## 性能考虑

### LLM 调用策略

**自动降级规则**：
1. **战斗时**：强制使用 Rule-Based（实时性要求高）
2. **候选过多**：超过 10 个候选时降级（LLM token 限制）
3. **社交时**：可使用 LLM（有时间思考）
4. **关键剧情时刻**：强制使用 LLM（叙事一致性优先）

### 缓存机制

**缓存策略**：
- **缓存时长**: 默认 5 秒（可配置 `CacheDuration`）
- **缓存验证**: 每次使用前检查目标是否仍有效（未死亡、在范围内）
- **同帧复用**: 同一控制器在同一帧多次请求相同 Context，直接返回缓存结果

**自动失效触发**：
当以下事件发生时，应调用 `InvalidateCache(Controller)` 清除缓存：

```cpp
// 示例：受到伤害时清除缓存（目标优先级可能改变）
void UHealthComponent::OnDamageReceived(float Damage, AController* InstigatedBy)
{
    // ... 扣血逻辑 ...
    
    // ✅ 清除目标缓存（攻击者成为新的高优先级目标）
    if (AAIController* AI = Cast<AAIController>(GetOwner()->GetInstigatorController()))
    {
        if (UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
        {
            TargetSystem->InvalidateCache(AI);
            NARRATIVE_LOG(Verbose, "Cache invalidated due to damage from %s", 
                *InstigatedBy->GetPawn()->GetName());
        }
    }
}

// 示例：队友死亡时清除缓存（复仇优先级提升）
void USensoryComponent::HandleDeath(AActor* Victim, AActor* Killer)
{
    // ... 记录死亡 ...
    
    // ✅ 清除缓存（凶手成为复仇目标）
    if (AAIController* AI = Cast<AAIController>(GetOwnerController()))
    {
        if (UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
        {
            TargetSystem->InvalidateCache(AI);
            NARRATIVE_LOG(Warning, "Cache invalidated due to ally death: %s killed by %s", 
                *Victim->GetName(), *Killer->GetName());
        }
    }
}

// 示例：Timeline 指令变化时清除缓存
void UNarrativeSquadSubsystem::TriggerTimelineNode(int32 SquadID, int32 NodeIndex)
{
    // ... 应用 DirectiveOverride ...
    
    // ✅ 清除所有 Squad 成员的缓存（行为目标可能改变）
    if (Node.DirectiveOverride.IsValid())
    {
        if (UTargetSelectionSubsystem* TargetSystem = GetWorld()->GetSubsystem<UTargetSelectionSubsystem>())
        {
            for (const auto& MemberPair : Squad->MemberRoles)
            {
                AAIController* AI = Cast<AAIController>(MemberPair.Key->GetInstigatorController());
                if (AI)
                {
                    TargetSystem->InvalidateCache(AI);
                }
            }
            NARRATIVE_LOG(Log, "Cleared target cache for Squad %d due to directive change", SquadID);
        }
    }
}
```

### 性能优势

**缓存带来的性能提升**：
- **规避重复计算**: 同一目标搜索不会在短时间内重复执行评分
- **减少 LLM 调用**: 5 秒内不会重复调用 LLM（节省 API 成本）
- **稳定性**: 目标不会频繁切换（避免"目标震荡"）

**实测数据**（预期）：
- 无缓存: 100 NPC × 每帧目标选择 = **10,000 次评分/秒**
- 有缓存: 100 NPC × 每 5 秒重选 = **20 次评分/秒** (500x 性能提升)
