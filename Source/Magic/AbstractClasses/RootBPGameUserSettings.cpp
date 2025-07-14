


#include "RootBPGameUserSettings.h"
#include "Magic/HelperBlueprintFunctionLibrary.h"
#include "Magic/MainWizardSaveGame.h"
#include "Kismet/GameplayStatics.h"

void URootBPGameUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);

	UMainWizardSaveGame* MainWizardSaveGame = Cast<UMainWizardSaveGame>(UGameplayStatics::LoadGameFromSlot("MainSettings", 0));
	if (IsValid(MainWizardSaveGame))
	{
		UHelperBlueprintFunctionLibrary::DeserializeObject(this, MainWizardSaveGame->Bytes);
	}
}

void URootBPGameUserSettings::SaveSettings()
{
	Super::SaveSettings();

	UMainWizardSaveGame* MainWizardSaveGame = NewObject<UMainWizardSaveGame>((UObject*)GetTransientPackage(), UMainWizardSaveGame::StaticClass());
	MainWizardSaveGame->Bytes = UHelperBlueprintFunctionLibrary::SerializeObject(this);
	if (UGameplayStatics::DoesSaveGameExist("MainSettings", 0))
	{
		UGameplayStatics::DeleteGameInSlot("MainSettings", 0);
	}

	UGameplayStatics::SaveGameToSlot(MainWizardSaveGame, "MainSettings", 0);
}

