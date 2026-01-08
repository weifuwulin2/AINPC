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

	// 1. 从 DataTable 加载性格配置
	// Load personality configuration from DataTable
	if (PersonalityTable && !PersonalityID.IsNone())
	{
		FPersonalityConfig* PersonalityRow = PersonalityTable->FindRow<FPersonalityConfig>(PersonalityID, TEXT("PersonalityComponent"));
		
		if (PersonalityRow)
		{
			Personality = *PersonalityRow;
			UE_LOG(LogTemp, Log, TEXT("[PersonalityComponent] Loaded personality: %s"), *PersonalityID.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[PersonalityComponent] Personality ID '%s' not found in table! Using default."), *PersonalityID.ToString());
			Personality = FPersonalityConfig();  // 使用默认值
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalityComponent] No PersonalityTable or PersonalityID set! Using default personality."));
		Personality = FPersonalityConfig();  // 使用默认值
	}

	// 2. 如果没有设置心理学模型，使用默认的
	// If no psychology model is set, use default
	if (!PsychologyModel)
	{
		InitializeDefaultPsychologyModel();
	}

	// 3. 计算马斯洛权重
	// Calculate Maslow weights
	RecalculateWeights();

	// 4. 将角色描述传递给 CognitionComponent
	// Pass role description to CognitionComponent
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
			
			UE_LOG(LogTemp, Log, TEXT("[PersonalityComponent] Role description set: %s"), *Personality.RoleDescription);
		}
	}

	// 5. 调试输出
	// Debug output
	DebugPrintPersonality();
}

void UPersonalityComponent::RecalculateWeights()
{
	if (!PsychologyModel)
	{
		UE_LOG(LogTemp, Error, TEXT("[PersonalityComponent] PsychologyModel is null! Cannot calculate weights."));
		return;
	}

	// 调用心理学模型的核心函数
	// Call the core function of psychology model
	MaslowWeights = PsychologyModel->RecalculateWeights(Personality);

	UE_LOG(LogTemp, Log, TEXT("[PersonalityComponent] Maslow weights recalculated successfully."));
}

float UPersonalityComponent::GetWeightForVariable(const FString& VariableName) const
{
	// 使用 if-else 链来匹配变量名
	// Use if-else chain to match variable name
	
	// 生理层 (Physiological)
	if (VariableName == TEXT("Hunger"))
	{
		return MaslowWeights.HungerWeight;
	}
	else if (VariableName == TEXT("Energy"))
	{
		return MaslowWeights.EnergyWeight;
	}
	// 安全层 (Safety)
	else if (VariableName == TEXT("Perceived_Threat"))
	{
		return MaslowWeights.ThreatWeight;
	}
	else if (VariableName == TEXT("Resource_Anxiety"))
	{
		return MaslowWeights.ResourceAnxietyWeight;
	}
	// 社交层 (Social)
	else if (VariableName == TEXT("Loneliness"))
	{
		return MaslowWeights.LonelinessWeight;
	}
	else if (VariableName == TEXT("Trust"))
	{
		return MaslowWeights.TrustWeight;
	}
	// 尊严层 (Esteem)
	else if (VariableName == TEXT("Anger"))
	{
		return MaslowWeights.AngerWeight;
	}
	else if (VariableName == TEXT("Social_Status"))
	{
		return MaslowWeights.SocialStatusWeight;
	}
	// 自我实现层 (Self-Actualization)
	else if (VariableName == TEXT("Duty_Urgency"))
	{
		return MaslowWeights.DutyWeight;
	}
	else if (VariableName == TEXT("Curiosity"))
	{
		return MaslowWeights.CuriosityWeight;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalityComponent] Unknown variable name: %s"), *VariableName);
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
	UE_LOG(LogTemp, Log, TEXT("Maslow Weights:"));
	UE_LOG(LogTemp, Log, TEXT("  [Physiological]"));
	UE_LOG(LogTemp, Log, TEXT("    Hunger:          %.2f"), MaslowWeights.HungerWeight);
	UE_LOG(LogTemp, Log, TEXT("    Energy:          %.2f"), MaslowWeights.EnergyWeight);
	UE_LOG(LogTemp, Log, TEXT("  [Safety]"));
	UE_LOG(LogTemp, Log, TEXT("    Threat:          %.2f"), MaslowWeights.ThreatWeight);
	UE_LOG(LogTemp, Log, TEXT("    ResourceAnxiety: %.2f"), MaslowWeights.ResourceAnxietyWeight);
	UE_LOG(LogTemp, Log, TEXT("  [Social]"));
	UE_LOG(LogTemp, Log, TEXT("    Loneliness:      %.2f"), MaslowWeights.LonelinessWeight);
	UE_LOG(LogTemp, Log, TEXT("    Trust:           %.2f"), MaslowWeights.TrustWeight);
	UE_LOG(LogTemp, Log, TEXT("  [Esteem]"));
	UE_LOG(LogTemp, Log, TEXT("    Anger:           %.2f"), MaslowWeights.AngerWeight);
	UE_LOG(LogTemp, Log, TEXT("    SocialStatus:    %.2f"), MaslowWeights.SocialStatusWeight);
	UE_LOG(LogTemp, Log, TEXT("  [Self-Actualization]"));
	UE_LOG(LogTemp, Log, TEXT("    Duty:            %.2f"), MaslowWeights.DutyWeight);
	UE_LOG(LogTemp, Log, TEXT("    Curiosity:       %.2f"), MaslowWeights.CuriosityWeight);
	
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
