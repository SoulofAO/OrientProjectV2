// Fill out your copyright notice in the Description page of Project Settings.
#include "BNAModule.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "BNAFileAsset.h"
#include "Modules/ModuleInterface.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "PropertyCustomizationHelpers.h"
#include "OrientProject/SimpleLandscape.h"

#define LOCTEXT_NAMESPACE "FBNAModule"

void FBNAModule::StartupModule()
{
	BNADataAssetTypeActions = MakeShared<FBNADataAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(BNADataAssetTypeActions.ToSharedRef());
    RegisterTab();
}

void FBNAModule::ShutdownModule()
{
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools")) return;
	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(BNADataAssetTypeActions.ToSharedRef());
    UnregisterTab();
}

void FBNAModule::RegisterTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		"ImportMapByBNA",
		FOnSpawnTab::CreateRaw(this, &FBNAModule::OnSpawnPluginTab)
	)
		.SetDisplayName(LOCTEXT("TabTitle", "ImportMap"))
		.SetMenuType(ETabSpawnerMenuType::Enabled)
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory()); // »ли свое меню
}

void FBNAModule::UnregisterTab()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("ImportMapByBNA");
}
TSharedRef<SDockTab> FBNAModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .Label(LOCTEXT("TabTitle", "MapImport"))
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SMyImportWindow)
        ];
}
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBNAModule, BNAModule);

void SMyImportWindow::Construct(const FArguments& InArgs)
{
    ChildSlot
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot().Padding(5)
                [
                    SNew(SObjectPropertyEntryBox)
                        .AllowedClass(UBNADataObject::StaticClass())
                        .ObjectPath_Lambda([this]() -> FString {
                        return BNADataObject ? BNADataObject->GetPathName() : FString();
                            })
                        .OnObjectChanged_Lambda([this](const FAssetData& AssetData)
                            {
                                BNADataObject = Cast<UBNADataObject>(AssetData.GetAsset());
                            })
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(5)
                [
                    SNew(SButton)
                        .Text(FText::FromString("Import"))
                        .OnClicked(this, &SMyImportWindow::OnImportClicked)
                ]

                + SVerticalBox::Slot().AutoHeight().Padding(5)
                [
                    SNew(SButton)
                        .Text(FText::FromString("Generate"))
                        .OnClicked(this, &SMyImportWindow::OnImportClicked)
                ]


        ];
}

FReply SMyImportWindow::OnImportClicked()
{
    TArray<FString> OpenFilePaths = OpenFileDialog();
    if (OpenFilePaths.Num() > 0)
    {
        FString ImportPath = TEXT("/Game/BNAObjects");
        TArray<UObject*> ImportedAssets =
            FAssetToolsModule::GetModule().Get().ImportAssets(OpenFilePaths, ImportPath);

        if (ImportedAssets.Num() > 0)
        {
            BNADataObject = Cast<UBNADataObject>(ImportedAssets[0]);
        }
    }

    return FReply::Handled();
}

FReply SMyImportWindow::OnGenerateClicked()
{
    if (IsValid(BNADataObject))
    {
        FVector2D MinPoint = BNADataObject->GetMinPosition();
        FVector2D MaxPoint = BNADataObject->GetMaxPosition();
        
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (World)
        {
            int SizeX = MaxPoint.X - MinPoint.X + DistanceBounds * 2;
            int SizeY = MaxPoint.Y - MinPoint.Y + DistanceBounds * 2;

            int CountX = FMath::CeilToInt(SizeX / PartSize);
            int CountY = FMath::CeilToInt(SizeY / PartSize);
            
            /*
            ASimpleLandscape* SimpleLandscape = World->SpawnActor<ASimpleLandscape>();
            SimpleLandscape->PartSize = PartSize;
            SimpleLandscape->GridStep = GridStep;
            SimpleLandscape->PartCountX = CountX;
            SimpleLandscape->PartCountY = CountY;


            SimpleLandscape->UpdateSettings();

            UClass* BPClass = LoadObject<UClass>(nullptr, TEXT("/Game/BP_OnlyBlueprint.BP_OnlyBlueprint_C"));
            
            if (BPClass)
            {
                FActorSpawnParameters Params;
                AActor* SpawnedActor = World->SpawnActor<AActor>(BPClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);

                FName FunctionName("DoSomething");

                UFunction* Function = SpawnedActor->FindFunction(FunctionName);
                if (Function)
                {
                    SpawnedActor->ProcessEvent(Function, nullptr);
                }
            }*/
        }
    }
    return FReply::Handled();
}

TArray<FString> SMyImportWindow::OpenFileDialog()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    FString FileTypes = TEXT("OMAP Files (*.omap)|*.omap|BNA Files (*.bna)|*.bna");

    if (DesktopPlatform)
    {
        FString OutFileName;
        TArray<FString> OutFiles;
        const FString Title = TEXT("ChooseFile");
        const FString DefaultPath = FPaths::ProjectDir();

        bool bOpened = DesktopPlatform->OpenFileDialog(
            FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
            Title,
            DefaultPath,
            TEXT(""),
            FileTypes,
            EFileDialogFlags::None,
            OutFiles
        );

        if (bOpened && OutFiles.Num() > 0)
        {
            OutFileName = OutFiles[0];
            UE_LOG(LogTemp, Log, TEXT("ChoosenFile: %s"), *OutFileName);
            return OutFiles;
        }
    }
    return TArray<FString>();
}
