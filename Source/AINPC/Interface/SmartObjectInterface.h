// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SmartObjectInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USmartObjectInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class AINPC_API ISmartObjectInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
	public:
	/** * 获取这个物品能提供的"情绪价值" 
	 * Key: 对应 MentalState 的字段名 (Hunger, Energy, Anger)
	 * Value: 提供的数值 (负数代表减少需求，正数代表增加)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SmartObject")
	void GetAffordances(TMap<FString, float>& OutAffordances);

	/**
	 * 执行交互逻辑
	 * @param User - 谁在使用我？
	 * @return - 交互是否成功
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SmartObject")
	bool Interact(AActor* User);
};
