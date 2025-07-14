// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGCorrectSplineSamplerSettings.generated.h"

/**
 * 
 */


UCLASS(BlueprintType, ClassGroup = (Procedural))
class UPCGCorrectSplineSamplerSettings : public UPCGSettings
{
	GENERATED_BODY()
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("CorrectSplineSampler")); }
	virtual FText GetDefaultNodeTitle() const override { return FText::FromString("CorrectSplineSampler"); }
	virtual FText GetNodeTooltipText() const override { return FText::FromString("Generates points along the given Spline, and within the Bounding Shape if provided."); };
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Sampler; }
#endif
	virtual FPCGElementPtr CreateElement() const override;
public:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override { return Super::DefaultPointOutputPinProperties(); }

	UPROPERTY(EditAnywhere, Category = Settings)
	float MeshSize = 1100;
	
};


class FPCGCorrectSplineSamplerElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
