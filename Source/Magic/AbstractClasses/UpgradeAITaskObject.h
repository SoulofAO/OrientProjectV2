

#pragma once

#include "CoreMinimal.h"
#include "UpgradeObject.h"
#include "UpgradeAITaskObject.generated.h"

/**
 * 
 */

USTRUCT(Blueprintable)
struct MAGIC_API FUpgradeAITaskObjectStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	UUpgradeAITaskObject* EditInLineUpgradeAITaskObject;
};

UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew)
class MAGIC_API UUpgradeAITaskObject : public UEditInLineUpgradeObject
{
	GENERATED_BODY()
	
};
