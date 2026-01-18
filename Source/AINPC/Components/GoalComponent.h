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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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

	UFUNCTION(BlueprintPure, Category = "Goal")
	EContextLOD GetCurrentLOD() const { return CurrentLOD; }

protected:
	void UpdateArbitration();

	// Checks schedule and updates CachedScheduleDirective
	void CheckSchedule();

	void SetDirective(FGameplayTag NewDirective);
	void SetLOD(EContextLOD NewLOD);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Goal")
	FGameplayTag CurrentDirective;

	UPROPERTY(VisibleAnywhere, Category = "Goal")
	EContextLOD CurrentLOD = EContextLOD::Standard;

	// Cached result from Schedule (updated via Time Slicing)
	UPROPERTY()
	FGameplayTag CachedScheduleDirective;

	// Timer for Schedule checks
	float TimeSinceLastScheduleCheck = 0.0f;
	float ScheduleCheckRandomOffset = 0.0f;

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
