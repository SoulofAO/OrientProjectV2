// Fill out your copyright notice in the Description page of Project Settings.

#include "TestBlueprintFunctionLibrary.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/IPlatformFileModule.h"
#include "Misc/Paths.h"
#include "Runtime/PakFile/Public/IPlatformFilePak.h"
#include "IPlatformFilePak.h"

void UTestBlueprintFunctionLibrary::LoadPak(FString PakPath)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString PakFilePath = FPaths::ProjectContentDir() + PakPath;

	FPakPlatformFile* PakPlatformFile = new FPakPlatformFile();
	PakPlatformFile->Initialize(&PlatformFile, TEXT(""));
	FPlatformFileManager::Get().SetPlatformFile(*PakPlatformFile);

	if (PlatformFile.FileExists(*PakFilePath))
	{
		PakPlatformFile->Mount(*PakFilePath, 0, TEXT(""));
		UE_LOG(LogTemp, Log, TEXT("DLC loaded successfully"));
	}
}
