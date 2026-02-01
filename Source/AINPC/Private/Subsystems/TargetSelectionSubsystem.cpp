// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystems/TargetSelectionSubsystem.h"

#include <Subsystems/NarrativeSquadSubsystem.h>

#include "AINPC.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Components/CognitionComponent.h"
#include "Components/FactionReputationComponent.h"
#include "Components/SensoryComponent.h"
#include "Components/MemoryComponent.h" // ✅ Added
#include "Components/GoalComponent.h"   // ✅ Added
#include "Social/SocialGameplayTags.h"  // ✅ Added
#include "GameFramework/Character.h"
#include "Utilities/AINPCHelpers.h"
#include "Utilities/FactionHelpers.h" // ✅ Faction utilities

// ========================================
// Main Entry Point
// ========================================

AActor* UTargetSelectionSubsystem::SelectTarget(
	AAIController* Controller,
	ETargetSelectionContext Context,
	const FTargetSelectionConfig& Config
)
{
	if (!Controller) return nullptr;

	APawn* MyPawn = Controller->GetPawn();
	if (!MyPawn) return nullptr;

	// ========================================
	// ✅ FIX: Lookup all components ONCE
	// 修复：只查找一次所有组件
	// ========================================
	UFactionReputationComponent* FactionComp = MyPawn->FindComponentByClass<UFactionReputationComponent>();
	UCognitionComponent* Cognition = MyPawn->FindComponentByClass<UCognitionComponent>();

	// ✅ Step 1: Check Cache First (with Combat Policy validation)
	AActor* CachedTarget = GetCachedTarget(Controller, Context, Config, MyPawn, FactionComp);
	if (CachedTarget)
	{
		// Verify cached target is still valid (not dead/ragdoll, still in range, Combat Policy)
		bool bIsDead = CachedTarget->ActorHasTag(FName("Dead")) || CachedTarget->ActorHasTag(FName("Status.Dead"));
		if (!bIsDead)
		{
			if (ACharacter* CharTarget = Cast<ACharacter>(CachedTarget))
			{
				if (CharTarget->GetMesh() && CharTarget->GetMesh()->IsSimulatingPhysics())
				{
					bIsDead = true;
				}
			}
		}

		if (!bIsDead)
		{
			float Distance = FVector::Dist(MyPawn->GetActorLocation(), CachedTarget->GetActorLocation());
			if (Distance <= Config.MaxDistance)
			{
				// ✅ FIX: Validate Combat Policy for cached targets
				// 修复：为缓存目标验证战斗策略
				if (Context == ETargetSelectionContext::Combat)
				{
					if (FactionComp && !FactionComp->EvaluateCombatPolicy(MyPawn, CachedTarget))
					{
						// InScene started, cached combat target invalid
						TARGET_LOG(Log, "   ⚠️ Cached target invalid: Combat Policy changed (InScene?)");
						InvalidateCache(Controller);
						CachedTarget = nullptr; // Force re-selection
					}
				}
				
				if (CachedTarget) // Still valid after all checks
				{
					TARGET_LOG(Verbose, "Using cached target: %s for context: %d", 
						*CachedTarget->GetName(), (int32)Context);
					return CachedTarget;
				}
			}
		}
		
		// Cache invalid, clear it
		InvalidateCache(Controller);
	}

	// ✅ Step 2: Get Candidates (pass FactionComp)
	TArray<AActor*> Candidates = GetTargetCandidates(Controller, Context, Config, MyPawn, FactionComp);
	
	if (Candidates.Num() == 0)
	{
		TARGET_LOG(Warning, "No valid target candidates found for context: %d", (int32)Context);
		return nullptr;
	}

	AActor* SelectedTarget = nullptr;

	// ✅ Step 3: Performance-Aware Selection Strategy
	bool bShouldUseLLM = Config.bUseLLMEnhancement;

	// 🔥 Combat Context: Force Rule-Based (Real-time requirement)
	if (Context == ETargetSelectionContext::Combat)
	{
		bShouldUseLLM = false;
		TARGET_LOG(Verbose, "Combat context detected, forcing rule-based selection for performance");
	}

	// 📊 Too Many Candidates: Use Rule-Based (LLM token limit)
	if (Candidates.Num() > 10 && bShouldUseLLM)
	{
		TARGET_LOG(Warning, "Too many candidates (%d), falling back to rule-based", Candidates.Num());
		bShouldUseLLM = false;
	}

	// ✅ Step 4: Select Target (pass components)
	bool bWasFallback = false;
	if (bShouldUseLLM)
	{
		// 🧠 LLM-Enhanced Selection (Slow but narrative-aware)
		SelectedTarget = SelectTargetByLLM(Controller, Candidates, Context, Cognition);
		
		// Fallback if LLM fails (returns null)
		if (!SelectedTarget)
		{
			TARGET_LOG(Warning, "LLM selection failed/pending, falling back to rule-based");
			SelectedTarget = SelectTargetByScoring(Controller, Candidates, Context, Config, MyPawn, Cognition, FactionComp);
			bWasFallback = true; // Mark as fallback so we don't cache this "temporary" decision long-term
		}
	}
	else
	{
		// ⚡ Rule-Based Selection (Fast)
		SelectedTarget = SelectTargetByScoring(Controller, Candidates, Context, Config, MyPawn, Cognition, FactionComp);
	}

	// ✅ Step 5: Cache Result
	// Don't cache if we fell back from LLM (waiting for async response), or if Config disables it
	if (SelectedTarget && !bWasFallback)
	{
		CacheTarget(Controller, Context, SelectedTarget);
	}
	else if (bWasFallback)
	{
		TARGET_LOG(Verbose, "Skipping cache for fallback selection (waiting for LLM)");
	}

	return SelectedTarget;
}

// ========================================
// Get Target Candidates
// ========================================

TArray<AActor*> UTargetSelectionSubsystem::GetTargetCandidates(
	AAIController* Controller,
	ETargetSelectionContext Context,
	const FTargetSelectionConfig& Config,
	APawn* MyPawn,
	UFactionReputationComponent* FactionComp
)
{
	TArray<AActor*> Candidates;

	if (!MyPawn) return Candidates;

	// Get AIPerceptionComponent to find perceived actors
	UAIPerceptionComponent* PerceptionComp = Controller->FindComponentByClass<UAIPerceptionComponent>();
	if (!PerceptionComp)
	{
		TARGET_LOG(Warning, "No AIPerceptionComponent found on %s", *Controller->GetName());
		return Candidates;
	}

	// Get all perceived actors
	TArray<AActor*> PerceivedActors;
	PerceptionComp->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

	// ✅ Narrative Shared Awareness (Omniscience for Plot)
	// If in a squad, we automatically "know" about other members (even if not seen)
	if (UNarrativeSquadSubsystem* NarrativeSys = Controller->GetWorld()->GetSubsystem<UNarrativeSquadSubsystem>())
	{
		TArray<AActor*> SquadMembers;
		if (NarrativeSys->GetSquadMembers(MyPawn, SquadMembers))
		{
			int32 InjectedCount = 0;
			for (AActor* Member : SquadMembers)
			{
				if (Member && !PerceivedActors.Contains(Member))
				{
					PerceivedActors.Add(Member);
					InjectedCount++;
				}
			}
			if (InjectedCount > 0)
			{
				TARGET_LOG(Verbose, "   🌐 Injected %d Squad Members as candidates (Narrative Awareness)", InjectedCount);
			}
		}
	}
	
	TARGET_LOG(Verbose, "🔍 [GetTargetCandidates] Total Candidates (Perception + Narrative): %d", PerceivedActors.Num());

	// Filter valid targets
	for (AActor* Actor : PerceivedActors)
	{
		bool bIsValid = IsValidTarget(Actor, MyPawn, Context, Config, FactionComp);
		TARGET_LOG(Verbose, "   - %s: %s", *Actor->GetName(), bIsValid ? TEXT("✅ VALID") : TEXT("❌ FILTERED"));
		
		if (bIsValid)
		{
			Candidates.Add(Actor);
		}
	}

	TARGET_LOG(Verbose, "🎯 [GetTargetCandidates] Final Candidates: %d (from %d perceived)", 
		Candidates.Num(), PerceivedActors.Num());

	return Candidates;
}

// ========================================
// Rule-Based Scoring
// ========================================

AActor* UTargetSelectionSubsystem::SelectTargetByScoring(
	AAIController* Controller,
	const TArray<AActor*>& Candidates,
	ETargetSelectionContext Context,
	const FTargetSelectionConfig& Config,
	APawn* MyPawn,
	UCognitionComponent* Cognition,
	UFactionReputationComponent* FactionComp
)
{
	if (!MyPawn || Candidates.Num() == 0) return nullptr;

	TARGET_LOG(Warning, "🎯 [SelectTargetByScoring] Scoring %d candidates for %s (Context: %d)", 
		Candidates.Num(), *MyPawn->GetName(), (int32)Context);
	
	TARGET_LOG(Warning, "   Components: Cognition=%s, FactionComp=%s", 
		Cognition ? TEXT("YES") : TEXT("NO"), 
		FactionComp ? TEXT("YES") : TEXT("NO"));

	AActor* BestTarget = nullptr;
	float BestScore = -100000.0f;  // Very low initial score

	// Score each candidate
	for (AActor* Candidate : Candidates)
	{
		float Score = CalculateTargetScore(Candidate, MyPawn, Context, Cognition, FactionComp);
		
		TARGET_LOG(Warning, "   → %s: Score = %.1f", *Candidate->GetName(), Score);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	if (BestTarget)
	{
		TARGET_LOG(Warning, "   ✅ Selected: %s (Score: %.1f)", *BestTarget->GetName(), BestScore);
	}
	else
	{
		TARGET_LOG(Warning, "   ❌ No target selected (BestScore: %.1f)", BestScore);
	}

	return BestTarget;
}

float UTargetSelectionSubsystem::CalculateTargetScore(
	AActor* Target,
	APawn* MyPawn,
	ETargetSelectionContext Context,
	UCognitionComponent* Cognition,
	UFactionReputationComponent* FactionComp
)
{
	float Score = 0.0f;

	FString TargetName = AINPCHelpers::GetSmartActorName(Target);

	// ✅ Memory-Driven Priority (Highest weight)
	if (Cognition)
	{
		// Check for "killed ally" memory
		// Access MemoryComponent directly as Cognition doesn't expose retrieval
		UMemoryComponent* MemoryComp = MyPawn->FindComponentByClass<UMemoryComponent>();
		if (MemoryComp)
		{
			TArray<FMemoryItem> RelevantMemories = MemoryComp->RetrieveRelevantMemories(TargetName, 5);
			for (const FMemoryItem& Memory : RelevantMemories)
			{
				// ✅ Refactored: Use Tags instead of fragile string matching
				if ((Memory.Tags.HasTag(AINPCTags::Event_Death_Witnessed) || Memory.Description.Contains("killed")) && 
					Memory.ImportanceScore > 7.0f)
				{
					Score += 1000.0f;  // Extreme priority - REVENGE
					TARGET_LOG(Verbose, "Revenge bonus for %s (killed ally): +1000", *TargetName);
					break;
				}
				else if ((Memory.Tags.HasTag(AINPCTags::Event_Combat_Damage) || Memory.Description.Contains("attacked")) && 
						 Memory.ImportanceScore > 5.0f)
				{
					Score += 500.0f;  // High priority - retaliation
					TARGET_LOG(Verbose, "Retaliation bonus for %s: +500", *TargetName);
				}
			}
		}
	}

	// ✅ Faction Reputation
	if (FactionComp)
	{
		// GetAttitudeTowards returns 0-100, where lower = hostile, higher = friendly
		float Attitude = FactionComp->GetAttitudeTowards(Target);

		if (Context == ETargetSelectionContext::Combat)
		{
			if (FactionHelpers::IsAttitudeHostile(Attitude))  // Hostile (below hostility threshold)
			{
				Score += (FactionHelpers::DEFAULT_HOSTILITY_THRESHOLD - Attitude) * 4.0f;  // 0-25 → +0 to +100 score
			}
			else if (Attitude > 75.0f)  // Friendly
			{
				Score -= 10000.0f;  // Heavily penalize attacking allies
			}
		}
		else if (Context == ETargetSelectionContext::Social)
		{
			// For social, prefer friendly/neutral
			if (Attitude > 50.0f)
			{
				Score += (Attitude - 50.0f) * 4.0f;  // 50-100 → +0 to +200
			}
		}
	}

	// ✅ Distance Factor (closer targets preferred)
	float Distance = FVector::Dist(MyPawn->GetActorLocation(), Target->GetActorLocation());
	float DistanceScore = FMath::Max(0.0f, (2000.0f - Distance) / 10.0f);  // +0 to +200
	Score += DistanceScore;

	// ✅ Aggression (is target attacking me?)
	AActor* TargetsFocus = GetActorFocus(Target);
	if (TargetsFocus == MyPawn)
	{
		Score += 300.0f;  // Prioritize counter-attack
		TARGET_LOG(Verbose, "Target %s is attacking me: +300", *TargetName);
	}

	// ✅ Match Goal/Directive State
	// If I am in Combat/Work/Social, I prefer targets who are appropriate for that state.
	UGoalComponent* MyGoal = MyPawn->FindComponentByClass<UGoalComponent>();
	UGoalComponent* TargetGoal = Target->FindComponentByClass<UGoalComponent>();
	
	if (MyGoal && TargetGoal)
	{
		FGameplayTag MyDirective = MyGoal->GetCurrentDirective();
		FGameplayTag TargetDirective = TargetGoal->GetCurrentDirective();

		if (Context == ETargetSelectionContext::Combat)
		{
			// In combat, prioritize targets who are ALSO in combat (threats)
			if (TargetDirective.MatchesTag(AINPCTags::Directive_Combat))
			{
				Score += 200.0f;
				TARGET_LOG(Verbose, "Target %s is also in Combat: +200", *TargetName);
			}
		}
		else if (Context == ETargetSelectionContext::Social)
		{
			// In social, avoid people who are in Combat or Panic
			if (TargetDirective.MatchesTag(AINPCTags::Directive_Combat) || 
				TargetDirective.MatchesTag(AINPCTags::Directive_Survival))
			{
				Score -= 500.0f; // Don't talk to fighting/panicking people
				TARGET_LOG(Verbose, "Target %s is Busy/Fighting: -500", *TargetName);
			}
			// Prefer people who are Idle or Social
			else if (TargetDirective.MatchesTag(AINPCTags::Directive_Idle) || 
					 TargetDirective.MatchesTag(AINPCTags::Directive_Social))
			{
				Score += 100.0f;
			}
		}
	}

	return Score;
}

// ========================================
// LLM-Enhanced Selection
// ========================================

AActor* UTargetSelectionSubsystem::SelectTargetByLLM(
	AAIController* Controller,
	const TArray<AActor*>& Candidates,
	ETargetSelectionContext Context,
	UCognitionComponent* Cognition
)
{
	APawn* MyPawn = Controller->GetPawn();
	if (!MyPawn) return nullptr;

	if (!Cognition)
	{
		TARGET_LOG(Warning, "No CognitionComponent for LLM selection, falling back to scoring");
		return nullptr;
	}

	// Convert actors to smart names
	TArray<FString> CandidateNames;
	TMap<FString, AActor*> NameToActorMap;

	for (AActor* Actor : Candidates)
	{
		FString SmartName = AINPCHelpers::GetSmartActorName(Actor);
		CandidateNames.Add(SmartName);
		NameToActorMap.Add(SmartName, Actor);
	}

	// Get context string
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

	// Ask LLM (will be implemented in CognitionComponent)
	FString SelectedName = Cognition->SuggestTarget(CandidateNames, ContextString);

	// Find actor by name
	AActor** FoundActor = NameToActorMap.Find(SelectedName);
	if (FoundActor && *FoundActor)
	{
		TARGET_LOG(Log, "LLM selected target (Exact Match): %s for context: %s", 
			*SelectedName, *ContextString);
		return *FoundActor;
	}

	// ✅ Fuzzy Match Fallback
	// LLM might reply "Grim Redbeard" instead of "Grim Redbeard (Guard, Stoic)"
	AActor* BestMatchActor = nullptr;
	int32 BestMatchLength = 0;

	for (const auto& Pair : NameToActorMap)
	{
		FString CandidateName = Pair.Key;
		
		// 1. Check if Candidate contains Selection (e.g. Candidate="Grim (Guard)", Selection="Grim")
		if (CandidateName.Contains(SelectedName))
		{
			// Prefer the shortest match that contains selection (most precise?) or longest?
			// Actually, just taking the first strong match is usually fine for names.
			BestMatchActor = Pair.Value;
			break;
		}
		
		// 2. Check if Selection contains Candidate (e.g. Selection="I choose Grim (Guard)", Candidate="Grim (Guard)")
		if (SelectedName.Contains(CandidateName))
		{
			BestMatchActor = Pair.Value;
			break;
		}

		// 3. Check partial name match (First name basis)
		// Split strings by space and check first token? 
		// Simpler: Check if Candidate starts with Selection
		if (CandidateName.StartsWith(SelectedName))
		{
			BestMatchActor = Pair.Value;
			break;
		}
	}

	if (BestMatchActor)
	{
		TARGET_LOG(Warning, "LLM selected target (Fuzzy Match): '%s' matched with candidate '%s'", 
			*SelectedName, *AINPCHelpers::GetSmartActorName(BestMatchActor));
		return BestMatchActor;
	}

	TARGET_LOG(Warning, "LLM selection failed to find actor for: %s (No exact or fuzzy match found)", *SelectedName);
	return nullptr;
}

// ========================================
// Validation & Helpers
// ========================================

bool UTargetSelectionSubsystem::IsValidTarget(
	AActor* Target,
	APawn* MyPawn,
	ETargetSelectionContext Context,
	const FTargetSelectionConfig& Config,
	UFactionReputationComponent* FactionComp
)
{
	if (!Target || Target == MyPawn) return false;

	// ✅ FIX: Reduced log verbosity to prevent flooding
	TARGET_LOG(Verbose, "🔍 [IsValidTarget] Checking %s for %s (Context: %d)", 
		*Target->GetName(), *MyPawn->GetName(), (int32)Context);

	// Check if dead or pending kill
	if (!IsValid(Target) || Target->IsPendingKillPending()) return false;

	// Check Tags (explicit FName)
	if (Target->ActorHasTag(FName("Dead")) || Target->ActorHasTag(FName("Status.Dead")))
	{
		TARGET_LOG(Verbose, "   ❌ REJECTED: Dead (Tag)");
		return false;
	}

	// Check Physics (Ragdoll = Dead)
	if (ACharacter* CharTarget = Cast<ACharacter>(Target))
	{
		if (CharTarget->GetMesh() && CharTarget->GetMesh()->IsSimulatingPhysics())
		{
			TARGET_LOG(Verbose, "   ❌ REJECTED: Dead (Ragdoll)");
			return false;
		}
	}

	// Check distance
	float Distance = FVector::Dist(MyPawn->GetActorLocation(), Target->GetActorLocation());
	
	// ✅ Combat Context Range Boost
	// Allow selecting targets further away in combat to prevent "freezing" when enemy is visible but out of standard range.
	// We can select them, then MoveTo will handle closing the distance.
	float EffectiveMaxDist = Config.MaxDistance;
	if (Context == ETargetSelectionContext::Combat)
	{
		EffectiveMaxDist = FMath::Max(Config.MaxDistance * 2.0f, 6000.0f); 
	}

	if (Distance > EffectiveMaxDist)
	{
		TARGET_LOG(Verbose, "   ❌ REJECTED: Distance %.0f > MaxDistance %.0f (Effective: %.0f)", 
			Distance, Config.MaxDistance, EffectiveMaxDist);
		return false;
	}

	// Context-specific validation
	if (Context == ETargetSelectionContext::Combat)
	{
		// ✅ CRITICAL FIX: Check Combat Policy BEFORE selecting combat targets
		// This prevents NPCs from entering Attack Action during InScene
		// 关键修复：在选择战斗目标前检查战斗策略
		// 这防止 NPC 在 InScene 时进入攻击动作
		if (FactionComp)
		{
			if (!FactionComp->EvaluateCombatPolicy(MyPawn, Target))
			{
				TARGET_LOG(Warning, "   ❌ REJECTED: Combat Policy DENIED (InScene protection)");
				return false; // InScene protection - no combat targets allowed
			}
		}
		
		// For combat, avoid friendly targets
		if (FactionComp)
		{
			float Attitude = FactionComp->GetAttitudeTowards(Target);
			TARGET_LOG(Verbose, "   → Attitude check: %.1f (Threshold: %.1f)", Attitude, Config.FriendlyReputationThreshold);
			
			if (Attitude > Config.FriendlyReputationThreshold)
			{
				TARGET_LOG(Verbose, "   ❌ REJECTED: Too friendly (Attitude %.1f > %.1f)", 
					Attitude, Config.FriendlyReputationThreshold);
				return false;  // Too friendly to attack
			}
		}
		else
		{
			TARGET_LOG(Verbose, "   ⚠️ No FactionComp - skipping attitude check");
		}
	}

	TARGET_LOG(Verbose, "   ✅ ACCEPTED as valid target");
	return true;
}

FName UTargetSelectionSubsystem::GetActorFaction(AActor* Actor)
{
	if (!Actor) return NAME_None;

	// GetFactionID is static and requires an Actor parameter
	return UFactionReputationComponent::GetFactionID(Actor);
}

AActor* UTargetSelectionSubsystem::GetActorFocus(AActor* Actor)
{
	if (!Actor) return nullptr;

	APawn* Pawn = Cast<APawn>(Actor);
	if (!Pawn) return nullptr;

	AAIController* AI = Cast<AAIController>(Pawn->GetController());
	return AI ? AI->GetFocusActor() : nullptr;
}

// ========================================
// Caching System
// ========================================

AActor* UTargetSelectionSubsystem::GetCachedTarget(
	AAIController* Controller, 
	ETargetSelectionContext Context,
	const FTargetSelectionConfig& Config,
	APawn* MyPawn,
	UFactionReputationComponent* FactionComp
)
{
	FTargetCacheKey Key;
	Key.Controller = Controller;
	Key.Context = Context;

	FTargetCacheEntry* CachedEntry = TargetCache.Find(Key);
	if (CachedEntry && IsCacheValid(*CachedEntry) && CachedEntry->Target.IsValid())
	{
		return CachedEntry->Target.Get();
	}

	return nullptr;
}

void UTargetSelectionSubsystem::CacheTarget(AAIController* Controller, ETargetSelectionContext Context, AActor* Target)
{
	if (!Target) return;

	FTargetCacheKey Key;
	Key.Controller = Controller;
	Key.Context = Context;

	FTargetCacheEntry Entry;
	Entry.Target = Target;

	if (UWorld* World = GetWorld())
	{
		Entry.CachedTime = World->GetTimeSeconds();
	}
	else
	{
		Entry.CachedTime = 0.0f; // Fallback
	}

	TargetCache.Add(Key, Entry);
	
	TARGET_LOG(Verbose, "Cached target: %s for context: %d (valid for %.1fs)", 
		*Target->GetName(), (int32)Context, CacheDuration);
}

bool UTargetSelectionSubsystem::IsCacheValid(const FTargetCacheEntry& Entry) const
{
	if (!Entry.Target.IsValid())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false; // World invalid, cache invalid
	}

	float CurrentTime = World->GetTimeSeconds();
	float Age = CurrentTime - Entry.CachedTime;

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

	if (KeysToRemove.Num() > 0)
	{
		TARGET_LOG(Verbose, "Invalidated %d cache entries for controller: %s",
			KeysToRemove.Num(), *Controller->GetName());
	}
}

void UTargetSelectionSubsystem::NotifyTargetDied(AActor* DeadTarget)
{
	if (!DeadTarget)
	{
		return;
	}

	// Find all cache entries using this target
	TArray<FTargetCacheKey> InvalidKeys;

	for (auto It = TargetCache.CreateIterator(); It; ++It)
	{
		const FTargetCacheKey& Key = It.Key();
		const FTargetCacheEntry& Entry = It.Value();

		if (Entry.Target.Get() == DeadTarget)
		{
			// Broadcast invalidation BEFORE removing
			// This alerts listeners (Action_Attack) to stop immediately
			OnTargetInvalidated.Broadcast(Key.Controller, DeadTarget);

			InvalidKeys.Add(Key);

			TARGET_LOG(Verbose, "NotifyTargetDied: Target %s invalidated for controller %s",
				*DeadTarget->GetName(),
				Key.Controller ? *Key.Controller->GetName() : TEXT("Unknown"));
		}
	}

	// Remove invalid entries
	for (const FTargetCacheKey& Key : InvalidKeys)
	{
		TargetCache.Remove(Key);
	}

	if (InvalidKeys.Num() > 0)
	{
		TARGET_LOG(Log, "NotifyTargetDied: Invalidated %d cache entries for dead target: %s",
			InvalidKeys.Num(), *DeadTarget->GetName());
	}
}

// ⚠️ DEPRECATED: Tick-based polling replaced with event-driven NotifyTargetDied()
// 已弃用：Tick 轮询已被事件驱动的 NotifyTargetDied() 取代
//
// Performance Analysis:
// - Old approach: O(n) every frame (n = number of cached targets)
// - New approach: O(1) only when death occurs
//
// Event-driven approach is triggered by SensoryComponent::HandleDeath()
// which immediately calls NotifyTargetDied() when any actor dies.
//
// Kept here for reference only. Remove after testing confirms event-driven approach works.
/*
void UTargetSelectionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Check validity of all cached targets
	TArray<FTargetCacheKey> InvalidKeys;

	for (auto It = TargetCache.CreateIterator(); It; ++It)
	{
		const FTargetCacheKey& Key = It.Key();
		const FTargetCacheEntry& Entry = It.Value();
		AActor* Target = Entry.Target.Get();

		bool bIsInvalid = false;

		// 1. Check Pointer Validity
		if (!Entry.Target.IsValid())
		{
			bIsInvalid = true;
		}
		else
		{
			// 2. Check Tags (Dead/Status.Dead)
			if (Target->ActorHasTag(FName("Dead")) || Target->ActorHasTag(FName("Status.Dead")))
			{
				bIsInvalid = true;
			}
			// 3. Check Physics (Ragdoll)
			else if (ACharacter* CharTarget = Cast<ACharacter>(Target))
			{
				if (CharTarget->GetMesh() && CharTarget->GetMesh()->IsSimulatingPhysics())
				{
					bIsInvalid = true;
				}
			}
		}

		if (bIsInvalid)
		{
			// 🔥 Broadcast Event BEFORE removing
			// This alerts listeners (Action_Attack) to stop immediately
			OnTargetInvalidated.Broadcast(Key.Controller, Target);

			InvalidKeys.Add(Key);
			TARGET_LOG(Verbose, "Tick: Target invalidated for %s", Key.Controller ? *Key.Controller->GetName() : TEXT("Unknown"));
		}
	}

	// Remove invalid entries
	for (const FTargetCacheKey& Key : InvalidKeys)
	{
		TargetCache.Remove(Key);
	}
}
*/

TStatId UTargetSelectionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTargetSelectionSubsystem, STATGROUP_Tickables);
}
