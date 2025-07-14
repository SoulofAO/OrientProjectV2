#pragma once

#include "PCGSettings.h"
#include "Data/PCGSurfaceData.h"
#include "Elements/PCGDataFromActor.h"
#include "SimpleLandscape.h"
#include "PCGSimpleLandscapeSettings.generated.h"

/**
* Add your tooltip here
*/

UCLASS(BlueprintType, ClassGroup = (Procedural))
class UPCGSimpleLandscapeData : public UPCGSurfaceData
{
	GENERATED_BODY()

public:
	void Initialize(const TArray<TWeakObjectPtr<ASimpleLandscape>>& InLandscapes, const FBox& InBounds);

	// ~Begin UPCGData interface
	virtual EPCGDataType GetDataType() const override { return EPCGDataType::Landscape; }
	// ~End UPCGData interface

	// ~Begin UPGCSpatialData interface
	virtual FBox GetBounds() const override;
	virtual FBox GetStrictBounds() const override;
	virtual bool SamplePoint(const FTransform& Transform, const FBox& Bounds, FPCGPoint& OutPoint, UPCGMetadata* OutMetadata) const override;
	virtual bool ProjectPoint(const FTransform& InTransform, const FBox& InBounds, const FPCGProjectionParams& InParams, FPCGPoint& OutPoint, UPCGMetadata* OutMetadata) const override;
	virtual bool HasNonTrivialTransform() const override { return true; }

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = SourceData)
	TArray<TSoftObjectPtr<ASimpleLandscape>> Landscapes;

protected:
	virtual UPCGSpatialData* CopyInternal() const override;
	//~End UPCGSpatialData interface

public:
	// ~Begin UPCGSpatialDataWithPointCache interface
	virtual bool SupportsBoundedPointData() const { return true; }
	virtual const UPCGPointData* CreatePointData(FPCGContext* Context) const override { return CreatePointData(Context, FBox(EForceInit::ForceInit)); }
	virtual const UPCGPointData* CreatePointData(FPCGContext* Context, const FBox& InBounds) const override;
	// ~End UPCGSpatialDataWithPointCache interface

protected:

	UPROPERTY()
	FBox Bounds = FBox(EForceInit::ForceInit);
};

UCLASS(BlueprintType, ClassGroup = (Procedural))
class UPCGGetSimpleLandscapeSettings : public UPCGDataFromActorSettings
{
	GENERATED_BODY()

public:
	UPCGGetSimpleLandscapeSettings();

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("GetSimpleLandscapeData")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGGetSimpleLandscapeSettings", "NodeTitle", "Get Simple Landscape Data"); }
	virtual FText GetNodeTooltipText() const override;
#endif

	virtual FName AdditionalTaskName() const override;

protected:
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings

public:
	//~Begin UPCGDataFromActorSettings interface
	virtual EPCGDataType GetDataFilter() const override { return EPCGDataType::Landscape; }
	virtual TSubclassOf<AActor> GetDefaultActorSelectorClass() const override;
	//~End UPCGDataFromActorSettings
};


class FPCGGetSimpleLandscapeDataElement : public FPCGDataFromActorElement
{
protected:
	virtual void ProcessActors(FPCGContext* Context, const UPCGDataFromActorSettings* Settings, const TArray<AActor*>& FoundActors) const override;
	virtual void ProcessActor(FPCGContext* Context, const UPCGDataFromActorSettings* Settings, AActor* FoundActor) const override;
};