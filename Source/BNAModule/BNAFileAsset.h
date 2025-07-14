// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "AssetTypeActions_Base.h"
#include "BNAFileAsset.generated.h"
/**
 * 
 */

class  FXmlNode;

UCLASS(config = EditorPerProjectUserSettings)
class UOrientiringImportAssetDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "PropertyMenuAssetPickerDeveloperSettings")
	
	/* { "Contour", "Index contour", "Slope line", "Contour value", "Earth bank",
		"Earth bank, minimum size", "Earth bank, very high", "Earth bank, very high, minimum size", "Earth bank, tag line", "Erosion gully", "Magnetic north line"};*/
	TArray<FString> IgnoringNames = { "Earth bank",  "Earth bank, minimum size", "Earth bank, very high", "Earth bank, very high, minimum size", "Earth bank, tag line", "Erosion gully", "Magnetic north line"};

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
};


USTRUCT(Blueprintable)
struct FPointArrayStruct
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TArray<FVector2D> Points;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float Rotation = 0.0;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	FString Text;
};

USTRUCT(Blueprintable)
struct FLineStruct
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TMap<int, FPointArrayStruct> Line;
};

UCLASS(Blueprintable)
class UBNADataObject : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TMap<FName, FLineStruct> DataMap;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float Scale;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FString ImportSource;

	UFUNCTION(BlueprintCallable)
	FVector2D GetMinPosition();

	UFUNCTION(BlueprintCallable)
	FVector2D GetMaxPosition();
};

enum ELineStatus
{
	Base,
	Pop
};

class FBNADataAssetTypeActions : public FAssetTypeActions_Base
{
public:
	UClass* GetSupportedClass() const override;
	FText GetName() const override;
	FColor GetTypeColor() const override;
	uint32 GetCategories() override;
};

UCLASS()
class UBNADataAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	UBNADataAssetFactory();
	UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn);
	virtual bool FactoryCanImport(const FString& Filename) override;
	bool CanConvertToFloat(const FString& Str);
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
};


UCLASS()
class UOMAPDataAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	UOMAPDataAssetFactory();
	UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn);
	virtual bool FactoryCanImport(const FString& Filename) override;
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	FXmlNode* FindXmlNodeByName(FXmlNode* CurrentNode, const FString& TargetName);
};

