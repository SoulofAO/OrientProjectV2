// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SCompoundWidget.h"
#include "BNAFileAsset.h"

class FBNADataAssetTypeActions;
class IModuleInterface;

class FBNAModule : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;

	TSharedPtr<FBNADataAssetTypeActions> BNADataAssetTypeActions;

private:
	void RegisterTab();
	void UnregisterTab();

	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);
};

class ASimpleLandscape;

class SMyImportWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMyImportWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    UBNADataObject* BNADataObject;
    TSharedPtr<SEditableTextBox> MapNameTextBox;
    float PartSize = 6300;
    float GridStep = 100;

    float DistanceBounds = 3000;

    FReply OnImportClicked();
    FReply OnGenerateClicked();

    TArray<FString> OpenFileDialog();
};



