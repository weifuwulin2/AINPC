#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Social/ProfessionTypes.h"
#include "AI/ContextLODTypes.h"
#include "GoalComponent.generated.h"

class UTimeManager;
class USensoryComponent;
class UCognitionComponent;
class UNPCMentalState;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API UGoalComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGoalComponent();

	virtual void BeginPlay() override;

	// --- Configuration ---

	// Profession ID (Key in the DataTable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal | Schedule")
	FName ProfessionID;

	// Load Profession from DataTable
	UFUNCTION(BlueprintCallable, Category = "Goal | Schedule")
	void InitializeProfession(FName NewProfessionID);

	// Profession DataTable (Maps ProfessionID -> FProfessionConfig)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal | Schedule")
	class UDataTable* ProfessionTable;

	// Loaded Config (Runtime)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Schedule")
	FProfessionConfig ProfessionConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal | Arbitration")
	float CriticalHungerThreshold = 0.6f;  // Trigger Survival when Hunger/Fatigue > 0.6

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal | Arbitration")
	float CriticalHealthThreshold = 0.3f;

	// Time Slicing Interval for Schedule Checks (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal | Optimization")
	float ScheduleCheckInterval = 5.0f;

	// --- State ---

	UFUNCTION(BlueprintPure, Category = "Goal")
	FGameplayTag GetCurrentDirective() const { return CurrentDirective; }

	// Returns the current scheduled activity (e.g., Interaction.Eat, Interaction.Work)
	UFUNCTION(BlueprintCallable, Category = "Goal")
	FGameplayTag GetScheduledActivity() const { return CachedScheduleActivity; }

	UFUNCTION(BlueprintPure, Category = "Goal")
	EContextLOD GetCurrentLOD() const { return CurrentLOD; }

	// --- Context Management (Gameplay Tags) ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Context")
	FGameplayTagContainer ActiveContextTags;

	UFUNCTION(BlueprintCallable, Category = "Goal | Context")
	void AddContextTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "Goal | Context")
	void RemoveContextTag(FGameplayTag Tag);

	UFUNCTION(BlueprintPure, Category = "Goal | Context")
	bool HasContextTag(FGameplayTag Tag) const;

protected:
	void UpdateArbitration();

	// Checks schedule and updates CachedScheduleDirective
	void CheckSchedule();

	void SetDirective(FGameplayTag NewDirective);
	void SetLOD(EContextLOD NewLOD);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	FGameplayTag CurrentDirective;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	EContextLOD CurrentLOD = EContextLOD::Standard;

	// Cached schedule state to avoid re-scanning every frame
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	FGameplayTag CachedScheduleDirective;

	// Cached scheduled activity
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	FGameplayTag CachedScheduleActivity;

	// ✅ Performance: Timer handles instead of tick
	FTimerHandle ArbitrationTimerHandle;
	FTimerHandle ScheduleCheckTimerHandle;

	// --- Dependencies ---
	UPROPERTY()
	UTimeManager* TimeManager;

	UPROPERTY()
	USensoryComponent* SensoryComp;

	UPROPERTY()
	UCognitionComponent* CognitionComp;
	
	UPROPERTY()
	UNPCMentalState* MentalState; // Accessed via Controller usually, but let's cache if possible or get from Owner
};
