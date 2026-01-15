// PersonalityComponent.cpp
// 性格组件实现 / Personality Component Implementation

#include "Components/PersonalityComponent.h"
#include "Controller/UtilityAIController.h"
#include "Components/CognitionComponent.h"

UPersonalityComponent::UPersonalityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPersonalityComponent::BeginPlay()
{
	Super::BeginPlay();

	// 如果没有设置心理学模型，使用默认的
	// If no psychology model is set, use default
	if (!PsychologyModel)
	{
		InitializeDefaultPsychologyModel();
	}
	
	// Note: PersonalityID will be set by CombatEnemy::BeginPlay() via delayed timer
	// CombatEnemy will call SetPersonalityByID() which triggers full initialization
	// 注意：PersonalityID 会由 CombatEnemy::BeginPlay() 通过延迟定时器设置
	// CombatEnemy 会调用 SetPersonalityByID() 触发完整初始化
	UE_LOG(LogTemp, Verbose, TEXT("[PersonalityComponent] BeginPlay complete. Waiting for CombatEnemy to set PersonalityID..."));
}

void UPersonalityComponent::SetPersonalityByID(FName NewPersonalityID)
{
	UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
	UE_LOG(LogTemp, Warning, TEXT("[PersonalityComponent] SetPersonalityByID called"));
	UE_LOG(LogTemp, Warning, TEXT("  Old PersonalityID: %s"), *PersonalityID.ToString());
	UE_LOG(LogTemp, Warning, TEXT("  New PersonalityID: %s"), *NewPersonalityID.ToString());
	
	// 设置新的 PersonalityID
	// Set new PersonalityID
	PersonalityID = NewPersonalityID;
	
	// 重新加载配置
	// Reload configuration
	RecalculateWeights();
	
	UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
}

void UPersonalityComponent::RecalculateWeights()
{
	// ✅ Reduce log noise - only show detailed logs when PersonalityID is set
	// 减少日志噪音 - 仅在 PersonalityID 已设置时显示详细日志
	bool bShowDetailedLogs = !PersonalityID.IsNone();
	
	if (bShowDetailedLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
		UE_LOG(LogTemp, Warning, TEXT("[PersonalityComponent] RecalculateWeights called"));
		UE_LOG(LogTemp, Warning, TEXT("  PersonalityID: %s"), *PersonalityID.ToString());
		UE_LOG(LogTemp, Warning, TEXT("  PersonalityTable: %s"), PersonalityTable ? TEXT("Valid") : TEXT("NULL"));
	}
	
	if (!PsychologyModel)
	{
		UE_LOG(LogTemp, Error, TEXT("[PersonalityComponent] PsychologyModel is null! Cannot calculate weights."));
		if (bShowDetailedLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
		}
		return;
	}

	// 如果有 PersonalityTable，尝试重新加载配置
	// If PersonalityTable exists, try to reload configuration
	if (PersonalityTable && !PersonalityID.IsNone())
	{
		FPersonalityConfig* PersonalityRow = PersonalityTable->FindRow<FPersonalityConfig>(PersonalityID, TEXT("RecalculateWeights"));
		
		if (PersonalityRow)
		{
			Personality = *PersonalityRow;
			UE_LOG(LogTemp, Warning, TEXT("  ✅ Reloaded personality from table: %s"), *PersonalityID.ToString());
			UE_LOG(LogTemp, Log, TEXT("     OCEAN: O=%.2f, C=%.2f, E=%.2f, A=%.2f, N=%.2f"), 
			       Personality.Openness, Personality.Conscientiousness, 
			       Personality.Extraversion, Personality.Agreeableness, Personality.Neuroticism);
			UE_LOG(LogTemp, Log, TEXT("     Faction: %s"), *UEnum::GetValueAsString(Personality.Faction));
			UE_LOG(LogTemp, Log, TEXT("     RoleDescription: %s"), *Personality.RoleDescription);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("  ❌ PersonalityID '%s' not found in table!"), *PersonalityID.ToString());
		}
	}
	else
	{
		// ✅ This is expected during initialization - don't spam warnings
		// 这在初始化期间是预期的 - 不要发出警告
		if (bShowDetailedLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("  ⚠️ No PersonalityTable or PersonalityID is None, using current Personality struct"));
		}
	}

	// 调用心理学模型的核心函数
	// Call the core function of psychology model
	MaslowWeights = PsychologyModel->RecalculateWeights(Personality);

	// 同步 RoleDescription 到 CognitionComponent
	// Sync RoleDescription to CognitionComponent
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (AIController)
	{
		AUtilityAIController* UtilityController = Cast<AUtilityAIController>(AIController);
		if (UtilityController && UtilityController->CognitionComp)
		{
			UtilityController->CognitionComp->RoleDescription = Personality.RoleDescription;
			
			// 如果有行为准则，也一起传递
			// If there are behavioral guidelines, pass them too
			if (!Personality.BehavioralGuidelines.IsEmpty())
			{
				UtilityController->CognitionComp->BehavioralGuidelines = Personality.BehavioralGuidelines;
			}
			
			if (bShowDetailedLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("  ✅ Updated RoleDescription: %s"), *Personality.RoleDescription);
			}
		}
	}

	if (bShowDetailedLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("  ✅ Maslow weights recalculated successfully"));
		UE_LOG(LogTemp, Warning, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
	}
}

float UPersonalityComponent::GetWeightForVariable(const FString& VariableName) const
{
	// 使用 if-else 链来匹配变量名并映射到马斯洛层级权重
	// Use if-else chain to match variable name and map to Maslow layer weights
	
	// 生理层 (Physiological)
	if (VariableName == TEXT("Hunger") || VariableName == TEXT("Fatigue"))
	{
		return MaslowWeights.Physiological;
	}
	// 安全层 (Safety)
	else if (VariableName == TEXT("Perceived_Threat"))
	{
		return MaslowWeights.Safety;
	}
	// 社交层 (Belonging)
	else if (VariableName == TEXT("Loneliness") || VariableName == TEXT("Trust")) // Trust 保留兼容,映射到Belonging
	{
		return MaslowWeights.Belonging;
	}
	// 尊严层 (Esteem)
	else if (VariableName == TEXT("Indignity") || VariableName == TEXT("Anger")) // Anger 保留兼容
	{
		return MaslowWeights.Esteem;
	}
	// 自我实现层 (Self-Actualization)
	else if (VariableName == TEXT("Boredom") || VariableName == TEXT("Curiosity")) // Curiosity 保留兼容
	{
		return MaslowWeights.SelfActualization;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalityComponent] Unknown variable name: %s, returning default weight 1.0"), *VariableName);
		return 1.0f;  // 默认权重
	}
}

void UPersonalityComponent::UsePersonalityTemplate(const FString& TemplateName)
{
	if (TemplateName == TEXT("BraveWarrior"))
	{
		Personality = FPersonalityConfig::BraveWarrior();
		UE_LOG(LogTemp, Log, TEXT("[PersonalityComponent] Applied template: Brave Warrior"));
	}
	else if (TemplateName == TEXT("CautiousGuard"))
	{
		Personality = FPersonalityConfig::CautiousGuard();
		UE_LOG(LogTemp, Log, TEXT("[PersonalityComponent] Applied template: Cautious Guard"));
	}
	else if (TemplateName == TEXT("FriendlyMerchant"))
	{
		Personality = FPersonalityConfig::FriendlyMerchant();
		UE_LOG(LogTemp, Log, TEXT("[PersonalityComponent] Applied template: Friendly Merchant"));
	}
	else if (TemplateName == TEXT("ReclusiveScholar"))
	{
		Personality = FPersonalityConfig::ReclusiveScholar();
		UE_LOG(LogTemp, Log, TEXT("[PersonalityComponent] Applied template: Reclusive Scholar"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalityComponent] Unknown template name: %s"), *TemplateName);
		return;
	}

	// 重新计算权重
	// Recalculate weights
	RecalculateWeights();
}

void UPersonalityComponent::DebugPrintPersonality() const
{
	UE_LOG(LogTemp, Log, TEXT("========================================"));
	UE_LOG(LogTemp, Log, TEXT("=== Personality Component Debug Info ==="));
	UE_LOG(LogTemp, Log, TEXT("========================================"));
	
	UE_LOG(LogTemp, Log, TEXT("OCEAN Personality:"));
	UE_LOG(LogTemp, Log, TEXT("  Openness (开放性):          %.2f"), Personality.Openness);
	UE_LOG(LogTemp, Log, TEXT("  Conscientiousness (尽责性): %.2f"), Personality.Conscientiousness);
	UE_LOG(LogTemp, Log, TEXT("  Extraversion (外向性):      %.2f"), Personality.Extraversion);
	UE_LOG(LogTemp, Log, TEXT("  Agreeableness (宜人性):     %.2f"), Personality.Agreeableness);
	UE_LOG(LogTemp, Log, TEXT("  Neuroticism (神经质):       %.2f"), Personality.Neuroticism);
	
	UE_LOG(LogTemp, Log, TEXT(""));
	UE_LOG(LogTemp, Log, TEXT("Maslow Weights (5-Layer Model):"));
	UE_LOG(LogTemp, Log, TEXT("  Physiological (Body):    %.2f"), MaslowWeights.Physiological);
	UE_LOG(LogTemp, Log, TEXT("  Safety (Threat):         %.2f"), MaslowWeights.Safety);
	UE_LOG(LogTemp, Log, TEXT("  Belonging (Social):      %.2f"), MaslowWeights.Belonging);
	UE_LOG(LogTemp, Log, TEXT("  Esteem (Dignity):        %.2f"), MaslowWeights.Esteem);
	UE_LOG(LogTemp, Log, TEXT("  Self-Actualization:      %.2f"), MaslowWeights.SelfActualization);
	
	UE_LOG(LogTemp, Log, TEXT("========================================"));
}

void UPersonalityComponent::InitializeDefaultPsychologyModel()
{
	// 创建默认的心理学模型
	// Create default psychology model
	PsychologyModel = NewObject<UPsychologyModel>(this, UPsychologyModel::StaticClass());
	
	if (PsychologyModel)
	{
		UE_LOG(LogTemp, Log, TEXT("[PersonalityComponent] Created default PsychologyModel."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PersonalityComponent] Failed to create default PsychologyModel!"));
	}
}
