// SentimentMapping.h
// 情感强度映射 / Sentiment Intensity Mapping
// 
// LLM 输出定性标签，代码转换为数值
// LLM outputs qualitative tags, code converts to numerical values

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SentimentMapping.generated.h"

/**
 * 情感强度标签 / Sentiment Intensity Tag
 * 
 * 5点量表法 (Likert Scale)
 * LLM 从这些标签中选择，而不是直接输出数值
 */
UENUM(BlueprintType)
enum class ESentimentIntensity : uint8
{
    None     UMETA(DisplayName = "None / Neutral"),      // 0.0 - 无感
    Slight   UMETA(DisplayName = "Slight / Low"),        // 0.25 - 轻微
    Moderate UMETA(DisplayName = "Moderate / Medium"),   // 0.5 - 中等
    Strong   UMETA(DisplayName = "Strong / High"),       // 0.75 - 强烈
    Extreme  UMETA(DisplayName = "Extreme / Max")        // 1.0 - 极端
};

/**
 * 情感映射配置 / Sentiment Mapping Configuration
 * 
 * 用于 DataTable，定义每个标签对应的数值
 * Used in DataTable to define numerical value for each tag
 */
USTRUCT(BlueprintType)
struct FSentimentMappingRow : public FTableRowBase
{
    GENERATED_BODY()

    // 标签名称 / Tag Name
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mapping")
    FString Tag;

    // 对应数值 / Corresponding Value
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mapping",
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Value = 0.0f;

    // 描述 / Description
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mapping")
    FString Description;
};

/**
 * 情感映射器 / Sentiment Mapper
 * 
 * 负责将 LLM 输出的标签转换为数值
 * Responsible for converting LLM output tags to numerical values
 */
UCLASS()
class AINPC_API USentimentMapper : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 初始化映射表 / Initialize Mapping Table
     * 
     * 可以从 DataTable 加载，或使用默认值
     * Can load from DataTable or use default values
     */
    void Initialize(UDataTable* MappingTable = nullptr);

    /**
     * 将标签转换为数值 / Convert Tag to Value
     * 
     * @param Tag - LLM 输出的标签 (如 "Strong", "Moderate")
     * @return 对应的数值 (0.0 - 1.0)，如果标签未找到返回 -1.0
     */
    UFUNCTION(BlueprintCallable, Category = "Sentiment Mapping")
    float TagToValue(const FString& Tag) const;

    /**
     * 将枚举转换为数值 / Convert Enum to Value
     * 
     * @param Intensity - 情感强度枚举
     * @return 对应的数值 (0.0 - 1.0)
     */
    UFUNCTION(BlueprintCallable, Category = "Sentiment Mapping")
    float IntensityToValue(ESentimentIntensity Intensity) const;

    /**
     * 获取所有可用标签 / Get All Available Tags
     * 
     * 用于生成 LLM Prompt 的选项列表
     * Used to generate option list for LLM Prompt
     */
    UFUNCTION(BlueprintCallable, Category = "Sentiment Mapping")
    TArray<FString> GetAvailableTags() const;

    /**
     * 生成 Prompt 指令 / Generate Prompt Instructions
     * 
     * 生成 LLM Prompt 中的标签选择说明
     * Generates tag selection instructions for LLM Prompt
     */
    UFUNCTION(BlueprintCallable, Category = "Sentiment Mapping")
    FString GeneratePromptInstructions() const;

private:
    // 映射字典 / Mapping Dictionary
    // Key: 标签名称 (如 "Strong")
    // Value: 对应数值 (如 0.75)
    TMap<FString, float> TagToValueMap;

    // 初始化默认映射 / Initialize Default Mapping
    void InitializeDefaultMapping();
};
