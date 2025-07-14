

#pragma once

#include "CoreMinimal.h"
#include "UpgradeObject.h"
#include "EditInLineMatchSettingsObject.generated.h"

/**
 * 
 */

USTRUCT(Blueprintable)
struct FEditInLineMatchSettingsObjectStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	UEditInLineMatchSettingsObject* EditInLineUpgradeObject;
};

UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class MAGIC_API UEditInLineMatchSettingsObject : public UEditInLineUpgradeObject
{
	GENERATED_BODY()
	
};
