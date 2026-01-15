// SentimentMapping.cpp
// 情感强度映射实现 / Sentiment Intensity Mapping Implementation

#include "UtilityAI/SentimentMapping.h"

void USentimentMapper::Initialize(UDataTable* MappingTable)
{
    if (MappingTable)
    {
        // 从 DataTable 加载映射
        // Load mapping from DataTable
        TArray<FName> RowNames = MappingTable->GetRowNames();
        
        for (const FName& RowName : RowNames)
        {
            FSentimentMappingRow* Row = MappingTable->FindRow<FSentimentMappingRow>(RowName, TEXT("SentimentMapper"));
            if (Row)
            {
                TagToValueMap.Add(Row->Tag, Row->Value);
                UE_LOG(LogTemp, Log, TEXT("[SentimentMapper] Loaded mapping: %s -> %.2f"), *Row->Tag, Row->Value);
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("[SentimentMapper] Initialized from DataTable with %d mappings"), TagToValueMap.Num());
    }
    else
    {
        // 使用默认映射
        // Use default mapping
        InitializeDefaultMapping();
        UE_LOG(LogTemp, Log, TEXT("[SentimentMapper] Initialized with default mapping"));
    }
}

void USentimentMapper::InitializeDefaultMapping()
{
    // 5点量表法 (Likert Scale)
    // 5-Point Likert Scale
    
    TagToValueMap.Empty();
    
    // 标准强度标签 / Standard Intensity Tags
    TagToValueMap.Add(TEXT("None"),     0.0f);   // 无感 / Neutral
    TagToValueMap.Add(TEXT("Neutral"),  0.0f);   // 同义词
    TagToValueMap.Add(TEXT("Slight"),   0.25f);  // 轻微 / Low
    TagToValueMap.Add(TEXT("Low"),      0.25f);  // 同义词
    TagToValueMap.Add(TEXT("Moderate"), 0.5f);   // 中等 / Medium
    TagToValueMap.Add(TEXT("Medium"),   0.5f);   // 同义词
    TagToValueMap.Add(TEXT("Strong"),   0.75f);  // 强烈 / High
    TagToValueMap.Add(TEXT("High"),     0.75f);  // 同义词
    TagToValueMap.Add(TEXT("Extreme"),  1.0f);   // 极端 / Max
    TagToValueMap.Add(TEXT("Max"),      1.0f);   // 同义词
    
    // 特定情绪的自然语言标签 / Natural Language Tags for Specific Emotions
    
    // Indignity (屈辱/尊严受损)
    TagToValueMap.Add(TEXT("Annoyed"),      0.2f);
    TagToValueMap.Add(TEXT("Disrespected"), 0.4f);
    TagToValueMap.Add(TEXT("Insulted"),     0.6f);
    TagToValueMap.Add(TEXT("Humiliated"),   0.9f);
    TagToValueMap.Add(TEXT("Resentful"),    1.0f);

    // Boredom (无聊)
    TagToValueMap.Add(TEXT("Interested"),   0.0f); // 反向: 感兴趣=0无聊
    TagToValueMap.Add(TEXT("Routine"),      0.3f);
    TagToValueMap.Add(TEXT("Bored"),        0.6f);
    TagToValueMap.Add(TEXT("Tedious"),      0.8f);
    TagToValueMap.Add(TEXT("Monotonous"),   1.0f);
    
    // Trust (信任)
    TagToValueMap.Add(TEXT("Suspicious"), 0.2f); // 怀疑
    TagToValueMap.Add(TEXT("Trusting"),   0.8f); // 信任
    TagToValueMap.Add(TEXT("Devoted"),    1.0f); // 死忠
    
    // Hunger (饥饿)
    TagToValueMap.Add(TEXT("Satisfied"),  0.0f); // 饱足
    TagToValueMap.Add(TEXT("Peckish"),    0.3f); // 有点饿
    TagToValueMap.Add(TEXT("Hungry"),     0.6f); // 饥饿
    TagToValueMap.Add(TEXT("Starving"),   1.0f); // 饿死了
    
    // Threat (威胁)
    TagToValueMap.Add(TEXT("Safe"),       0.0f); // 安全
    TagToValueMap.Add(TEXT("Cautious"),   0.3f); // 谨慎
    TagToValueMap.Add(TEXT("Threatened"), 0.7f); // 受威胁
    TagToValueMap.Add(TEXT("Terrified"),  1.0f); // 恐惧
    
    UE_LOG(LogTemp, Log, TEXT("[SentimentMapper] Default mapping initialized with %d entries"), TagToValueMap.Num());
}

float USentimentMapper::TagToValue(const FString& Tag) const
{
    // 查找映射
    // Lookup mapping
    const float* ValuePtr = TagToValueMap.Find(Tag);
    
    if (ValuePtr)
    {
        return *ValuePtr;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[SentimentMapper] Tag '%s' not found in mapping! Returning -1.0"), *Tag);
        return -1.0f;  // 表示未找到
    }
}

float USentimentMapper::IntensityToValue(ESentimentIntensity Intensity) const
{
    // 枚举到数值的直接映射
    // Direct mapping from enum to value
    switch (Intensity)
    {
        case ESentimentIntensity::None:     return 0.0f;
        case ESentimentIntensity::Slight:   return 0.25f;
        case ESentimentIntensity::Moderate: return 0.5f;
        case ESentimentIntensity::Strong:   return 0.75f;
        case ESentimentIntensity::Extreme:  return 1.0f;
        default:                            return 0.0f;
    }
}

TArray<FString> USentimentMapper::GetAvailableTags() const
{
    TArray<FString> Tags;
    TagToValueMap.GetKeys(Tags);
    return Tags;
}

FString USentimentMapper::GeneratePromptInstructions() const
{
    // 生成 LLM Prompt 中的标签选择说明
    // Generate tag selection instructions for LLM Prompt
    
    FString Instructions = TEXT("# Sentiment Intensity Tags\n\n");
    Instructions += TEXT("When analyzing emotional changes, choose from these STANDARD INTENSITY TAGS:\n");
    Instructions += TEXT("[None, Slight, Moderate, Strong, Extreme]\n\n");
    
    Instructions += TEXT("Mapping:\n");
    Instructions += TEXT("- None/Neutral: 0.0 (无感)\n");
    Instructions += TEXT("- Slight/Low: 0.25 (轻微)\n");
    Instructions += TEXT("- Moderate/Medium: 0.5 (中等)\n");
    Instructions += TEXT("- Strong/High: 0.75 (强烈)\n");
    Instructions += TEXT("- Extreme/Max: 1.0 (极端)\n\n");
    
    Instructions += TEXT("You can also use natural language tags for specific emotions:\n");
    Instructions += TEXT("- Indignity: Disrespected, Insulted, Humiliated, Resentful\n");
    Instructions += TEXT("- Boredom: Routine, Bored, Tedious, Monotonous\n");
    Instructions += TEXT("- Trust: Suspicious, Trusting, Devoted\n");
    Instructions += TEXT("- Hunger: Satisfied, Peckish, Hungry, Starving\n");
    Instructions += TEXT("- Threat: Safe, Cautious, Threatened, Terrified\n\n");

    
    Instructions += TEXT("IMPORTANT: Do NOT output numerical values (0.0-1.0). Only use the tags above.\n");
    
    return Instructions;
}
