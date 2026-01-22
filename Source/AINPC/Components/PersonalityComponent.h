// PersonalityComponent.h
// 性格组件 / Personality Component
//
// 负责存储 NPC 的 OCEAN 性格和计算出的马斯洛权重
// Responsible for storing NPC's OCEAN personality and calculated Maslow weights

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UtilityAI/PersonalityConfig.h"
#include "UtilityAI/EmotionEvaluator.h" // 引入 FMaslowWeights 和 UPsychologyModel 定义
#include "PersonalityComponent.generated.h"

/**
 * 性格组件 / Personality Component
 * 
 * 这个组件存储 NPC 的"出厂设置"（OCEAN 性格）和计算出的马斯洛权重
 * This component stores the NPC's "factory settings" (OCEAN personality) and calculated Maslow weights
 * 
 * 使用流程 / Usage Flow:
 * 1. 在 BeginPlay 时，根据 OCEAN 性格计算马斯洛权重
 * 2. 游戏运行时，这些权重只读不改
 * 3. Utility AI 使用这些权重来调整动作得分
 * 
 * 1. At BeginPlay, calculate Maslow weights from OCEAN personality
 * 2. During runtime, these weights are read-only
 * 3. Utility AI uses these weights to adjust action scores
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AINPC_API UPersonalityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPersonalityComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ========== OCEAN 性格配置 (OCEAN Personality Configuration) ==========
	
	/**
	 * 性格 ID / Personality ID
	 * 
	 * 用于从 DataTable 中查找对应的 OCEAN 性格配置
	 * Used to lookup OCEAN personality configuration from DataTable
	 * 
	 * 例如: "BraveWarrior", "CautiousGuard", "FriendlyMerchant"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality",
	          meta = (DisplayName = "Personality ID", 
	                  ToolTip = "从 DataTable 中查找的性格 ID / Personality ID to lookup in DataTable"))
	FName PersonalityID = "Default";

	/**
	 * 性格配置表 / Personality Configuration Table
	 * 
	 * 存储所有 NPC 的 OCEAN 性格配置
	 * Stores OCEAN personality configurations for all NPCs
	 * 
	 * DataTable 结构: FPersonalityConfig
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality",
	          meta = (DisplayName = "Personality Table", 
	                  ToolTip = "性格配置数据表 / Personality configuration DataTable"))
	UDataTable* PersonalityTable;

	/**
	 * 心理学模型数据资产 / Psychology Model Data Asset
	 * 
	 * 存储 OCEAN → Maslow 的转化系数
	 * Stores OCEAN → Maslow transformation coefficients
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality",
	          meta = (DisplayName = "Psychology Model", 
	                  ToolTip = "心理学转化系数数据资产 / Psychology transformation coefficients data asset"))
	UPsychologyModel* PsychologyModel;

	// ========== 运行时数据 (Runtime Data) ==========
	
	/**
	 * 当前的 OCEAN 性格 / Current OCEAN Personality
	 * 
	 * 从 DataTable 加载后存储在这里
	 * Loaded from DataTable and stored here
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Personality",
	          meta = (DisplayName = "Current Personality"))
	FPersonalityConfig Personality;
	
	/**
	 * 马斯洛需求层次权重 / Maslow's Hierarchy Weights
	 * 
	 * 这些权重在 BeginPlay 时计算，游戏运行时只读
	 * These weights are calculated at BeginPlay and are read-only during runtime
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Personality",
	          meta = (DisplayName = "Maslow Weights", 
	                  ToolTip = "计算出的马斯洛需求权重 / Calculated Maslow hierarchy weights"))
	FMaslowWeights MaslowWeights;

	// ========== 公共接口 (Public Interface) ==========
	
	/**
	 * 获取指定马斯洛变量的权重 / Get weight for specific Maslow variable
	 * 
	 * @param VariableName - 变量名 (例如 "Hunger", "Perceived_Threat") / Variable name
	 * @return 对应的权重值 / Corresponding weight value
	 */
	UFUNCTION(BlueprintCallable, Category = "Personality")
	float GetWeightForVariable(const FString& VariableName) const;
	void UsePersonalityTemplate(const FString& TemplateName);

	/**
	 * 设置性格 ID 并重新加载配置 / Set Personality ID and reload configuration
	 * 
	 * 这是运行时设置性格的推荐方法，会自动重新加载配置
	 * This is the recommended way to set personality at runtime, will auto-reload configuration
	 * 
	 * @param NewPersonalityID - 新的性格 ID / New personality ID (e.g., "Zombie", "Merchant")
	 */
	UFUNCTION(BlueprintCallable, Category = "Personality")
	void SetPersonalityByID(FName NewPersonalityID);

	/**
	 * 重新计算马斯洛权重 / Recalculate Maslow weights
	 * 
	 * 通常不需要手动调用，除非你在运行时改变了 OCEAN 性格
	 * Usually no need to call manually unless you change OCEAN personality at runtime
	 */
	UFUNCTION(BlueprintCallable, Category = "Personality")
	void RecalculateWeights();

	/**
	 * 调试输出：打印当前性格和权重 / Debug output: Print current personality and weights
	 */
	UFUNCTION(BlueprintCallable, Category = "Personality")
	void DebugPrintPersonality() const;

	/**
	 * 获取指定 OCEAN 特质的当前值 (0.0 - 1.0)
	 * Get current value of specific OCEAN trait (0.0 - 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Personality")
	float GetTraitValue(EOCEANTrait Trait) const;

private:
	/**
	 * 初始化默认心理学模型 / Initialize default psychology model
	 */
	void InitializeDefaultPsychologyModel();
};
