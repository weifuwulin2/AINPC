#include "MemoryComponent.h"

UMemoryComponent::UMemoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMemoryComponent::BeginPlay()
{
    Super::BeginPlay();
    // 可以在这里加载存档的记忆
}

void UMemoryComponent::AddMemory(FString Content, float Importance)
{
    // 1. 构建新记忆
    FMemoryFragment NewMem;
    NewMem.Description = Content;
    NewMem.Timestamp = FDateTime::Now();
    NewMem.Importance = Importance;

    // 2. 加入流
    MemoryStream.Add(NewMem);

    // 3. 遗忘机制 (FIFO - 先进先出，但保留极其重要的记忆)
    if (MemoryStream.Num() > MaxMemoryCount)
    {
        // 简单策略：直接删除最旧的
        // 进阶策略：遍历前10个，删除 Importance 低于 0.8 的
        MemoryStream.RemoveAt(0);
    }

    UE_LOG(LogTemp, Log, TEXT("[Memory] Stored: %s"), *Content);
}

FString UMemoryComponent::RetrieveRelevantMemories(FString CurrentContext)
{
    if (MemoryStream.Num() == 0) return TEXT("No past memories.");

    FString ResultString = "";
    int32 FoundCount = 0;
    const int32 MaxRetrieve = 5; // 最多给 LLM 看 5 条，省 Token

    // --- 策略 A: 关键字匹配 (Relevance) ---
    // 简单的做法：把 CurrentContext 拆成单词，去历史里搜
    // 比如 Context="I see a Zombie"，那么我们去搜包含 "Zombie" 的旧记忆
    
    TArray<FString> Keywords;
    CurrentContext.ParseIntoArray(Keywords, TEXT(" "), true);

    // 倒序遍历 (从最新的开始找)
    for (int32 i = MemoryStream.Num() - 1; i >= 0; i--)
    {
        bool bIsRelevant = false;
        
        // 1. 检查是否包含关键字
        for (const FString& Word : Keywords)
        {
            // 忽略像 "I", "a", "the" 这种停用词 (这里简化处理，只匹配长度大于3的词)
            if (Word.Len() > 3 && MemoryStream[i].Description.Contains(Word))
            {
                bIsRelevant = true;
                break;
            }
        }

        // 2. 总是包含最近的 2 条记忆 (Recency Bias)
        // 即使没有关键字，最近发生的事也很重要
        if (i >= MemoryStream.Num() - 2)
        {
            bIsRelevant = true;
        }

        // 如果相关，就加入结果
        if (bIsRelevant)
        {
            FString TimeStr = MemoryStream[i].Timestamp.ToString();
            ResultString += FString::Printf(TEXT("- [%s] %s\n"), *TimeStr, *MemoryStream[i].Description);
            
            FoundCount++;
            if (FoundCount >= MaxRetrieve) break; // 找够了就停
        }
    }

    return ResultString;
}