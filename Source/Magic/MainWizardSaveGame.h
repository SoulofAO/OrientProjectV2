

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MainWizardSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class UMainWizardSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	TArray<uint8> Bytes;
	
};
