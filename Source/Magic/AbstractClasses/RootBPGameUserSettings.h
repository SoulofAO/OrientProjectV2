

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "RootBPGameUserSettings.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class URootBPGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	virtual void LoadSettings(bool bForceReload = false) override;

	virtual void SaveSettings() override;

};
