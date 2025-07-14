

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UpgradeDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, Config = Game, defaultconfig, meta = (DisplayName = "Blueprint Game Settings"))
class MAGIC_API UUpgradeDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
};
