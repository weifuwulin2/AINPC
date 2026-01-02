#pragma once

#include "CoreMinimal.h"
#include "Base/UtilityActionBase.h"
#include "Action_Attack.generated.h"

/**
 * 具体的攻击行为
 * 逻辑：愤怒值越高，得分越高
 */
UCLASS()
class AINPC_API UAction_Attack : public UUtilityActionBase
{
	GENERATED_BODY()

public:

	UAction_Attack();
	
};