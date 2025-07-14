// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PCGSettings.h"
#include "Data/PCGSurfaceData.h"
#include "Elements/PCGDataFromActor.h"
#include "PCGTraceProjectionSettings.generated.h"

/**
* Add your tooltip here
*/

UCLASS(MinimalAPI, BlueprintType)
class UPCGFilterNumberSplinePointSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override;
	virtual FText GetDefaultNodeTitle() const override;
	virtual FText GetNodeTooltipText() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif //WITH_EDITOR

	UPROPERTY(EditAnywhere, Category = Settings)
	int NumberIndex = 2;

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

/**
* Define various user facing properties here.
* Tooltip can be added above UPROPERTY line.
* PCG_Overridable meta tag allows the properties to be overriden with other attributes in the graph editor.
*
* public:
*	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
*	bool CheckBox = true;
*/
};

class FPCGFilterNumberSplinePointSettingsElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};



UCLASS(MinimalAPI, BlueprintType)
class UPCGTraceProjectionSettingsSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override;
	virtual FText GetDefaultNodeTitle() const override;
	virtual FText GetNodeTooltipText() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif //WITH_EDITOR

	UPROPERTY(EditAnywhere, Category = Settings)
	TSubclassOf<AActor> FilterActorClass;

	UPROPERTY(EditAnywhere, Category = Settings)
	bool UseCoordAsWorldCoordinate;


protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

/**
* Define various user facing properties here. 
* Tooltip can be added above UPROPERTY line.
* PCG_Overridable meta tag allows the properties to be overriden with other attributes in the graph editor.
*
* public:
*	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
*	bool CheckBox = true;
*/
};

class FPCGTraceProjectionSettingsElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};