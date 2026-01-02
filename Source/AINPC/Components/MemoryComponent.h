// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MemoryComponent.generated.h"

// 定义单条记忆的结构
USTRUCT(BlueprintType)
struct FMemoryFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Description; // 记忆内容："I saw a zombie"

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDateTime Timestamp; // 时间戳：2023-10-01 12:00:00

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Importance;    // 重要性 (0.0 - 1.0) - 用于决定是否长期保留

	// 新增：标记这是不是一条经过总结的长期记忆
	UPROPERTY(VisibleAnywhere) bool bIsLongTermInsight = false;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AINPC_API UMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:    
	UMemoryComponent();
	virtual void BeginPlay() override;

	// --- 核心接口 ---

	/** * 存入记忆 
	 * @param Content 记忆的内容
	 * @param Importance 重要程度(0-1)，越高越不容易被遗忘
	 */
	UFUNCTION(BlueprintCallable, Category = "AI | Memory")
	void AddMemory(FString Content, float Importance = 0.5f);

	/** * 检索记忆 (简易 RAG)
	 * 根据当前的输入(CurrentContext)，在记忆库里找相关的旧事
	 * @return 格式化好的字符串，可以直接丢进 LLM 的 Prompt
	 */
	UFUNCTION(BlueprintCallable, Category = "AI | Memory")
	FString RetrieveRelevantMemories(FString CurrentContext);

	UFUNCTION(BlueprintCallable)
	FString GetAllRecentMemoriesAsString();

	UFUNCTION(BlueprintCallable)
	void ConsolidateMemories(const TArray<FString>& NewInsights);
	
protected:
	// 记忆流：按时间顺序存储
	UPROPERTY(VisibleAnywhere, Category = "AI | Memory")
	TArray<FMemoryFragment> MemoryStream;

	// 记忆库最大容量 (防止内存无限膨胀)
	UPROPERTY(EditDefaultsOnly, Category = "AI | Memory")
	int32 MaxMemoryCount = 50;
		
};
