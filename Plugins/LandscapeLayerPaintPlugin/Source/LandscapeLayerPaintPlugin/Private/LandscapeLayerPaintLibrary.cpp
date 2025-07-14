
#include "LandscapeLayerPaintLibrary.h"
#include "LandscapeLayerInfoObject.h"
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "Engine.h"
#include "LandscapeSplineSegment.h"
#include "Raster.h"
#include "LandscapeSubsystem.h"

#if WITH_EDITOR
ULandscapeLayerInfoObject* ULandscapeLayerPaintLibrary::CreateLayerInfo(UObject* Outer, const FString& LayerName)
{
    ULandscapeLayerInfoObject* LayerInfo = NewObject<ULandscapeLayerInfoObject>(Outer);
    LayerInfo->LayerName = FName(*LayerName);

    LayerInfo->PhysMaterial = nullptr;

    return LayerInfo;
}

void ULandscapeLayerPaintLibrary::PaintLandscapeLayer(ULandscapeInfo* LandscapeInfo, ULandscapeLayerInfoObject* LandscapeLayerInfoObject, FVector2D Position, FVector2D Scale, int Power)
{
    if (!LandscapeInfo || !LandscapeLayerInfoObject)
    {
        return; 
    }
    FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo, true);

    int32 Width = 1 + Scale.X;
    int32 Height = 1 + Scale.Y;

    TArray<uint8> AlphaData;
    AlphaData.AddZeroed(Width * Height); 
    int ValidMinX = int(Position.X - Scale.X / 2);
    int ValidMinY = int(Position.Y - Scale.Y / 2);
    int ValidMaxX = int(Position.X + Scale.X / 2);
    int ValidMaxY = int(Position.Y + Scale.Y / 2);

    LandscapeEdit.GetWeightData(LandscapeLayerInfoObject, ValidMinX, ValidMinY, ValidMaxX, ValidMaxY, AlphaData.GetData(), 0);

    for (int32 i = 0; i < Width * Height; i++)
    {
        AlphaData[i] = FMath::Clamp(Power, 0, 255); 
    }

    TSet<ULandscapeComponent*> Components;
    LandscapeInfo->GetComponentsInRegion(Position.X - Scale.X / 2,
        Position.Y - Scale.Y / 2,
        Position.X + Scale.X / 2,
        Position.Y + Scale.Y / 2, Components);

    LandscapeInfo->LandscapeActor->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Weightmap_Editing);

    LandscapeEdit.SetAlphaData(LandscapeLayerInfoObject,
        Position.X - Scale.X / 2,
        Position.Y - Scale.Y / 2,
        Position.X + Scale.X / 2,
        Position.Y + Scale.Y / 2,
        AlphaData.GetData(),
        0,
        ELandscapeLayerPaintingRestriction::None,
        !LandscapeLayerInfoObject->bNoWeightBlend,
        false);


    if (LandscapeInfo)
    {
        LandscapeInfo->LandscapeActor->MarkPackageDirty();
    }

    LandscapeInfo->GetLandscapeProxy()->GetWorld()->GetSubsystem<ULandscapeSubsystem>()->SaveModifiedLandscapes();
}
#endif

bool ULandscapeLayerPaintLibrary::WorldPositionToLandscapePosition(FVector Position, ALandscape* Landscape, FVector2D& AnswerPosition)
{
#if WITH_EDITOR
	if (!Landscape)
	{
		return false;
	}
	FIntRect LandscapeExtent;
	Landscape->GetLandscapeInfo()->GetLandscapeExtent(LandscapeExtent.Min.X, LandscapeExtent.Min.Y, LandscapeExtent.Max.X, LandscapeExtent.Max.Y);
	float DistanceToProxy = FLT_MAX;
	ALandscapeProxy* AnswerProxy = nullptr;
	for (TActorIterator<ALandscapeProxy> It(Landscape->GetWorld(), ALandscapeProxy::StaticClass()); It; ++It)
	{
		ALandscapeProxy* Proxy = *It;
		if (DistanceToProxy > FVector::Distance(Proxy->GetActorLocation(), Position))
		{
			DistanceToProxy = FVector::Distance(Proxy->GetActorLocation(), Position);
			AnswerProxy = Proxy;
		}
	}
	if (!AnswerProxy)
	{
		return false;
	}
	AnswerPosition = FVector2D(AnswerProxy->LandscapeActorToWorld().InverseTransformPosition(Position).X, AnswerProxy->LandscapeActorToWorld().InverseTransformPosition(Position).Y);
	if (AnswerPosition.X > LandscapeExtent.Min.X && AnswerPosition.X < LandscapeExtent.Max.X && AnswerPosition.Y > LandscapeExtent.Min.Y && AnswerPosition.Y < LandscapeExtent.Max.Y)
	{
		return true;
	}
#endif
	return false;
}

bool ULandscapeLayerPaintLibrary::LandscapePositionToWorldPosition(FVector2D LocalPosition, ALandscape* Landscape, FVector& WorldPosition)
{
	if (!Landscape)
	{
		return false;
	}

	ALandscapeProxy* AnswerProxy = nullptr;
	float DistanceToProxy = FLT_MAX;

	for (TActorIterator<ALandscapeProxy> It(Landscape->GetWorld(), ALandscapeProxy::StaticClass()); It; ++It)
	{
		ALandscapeProxy* Proxy = *It;
		float Distance = FVector::Distance(Proxy->GetActorLocation(), Landscape->GetActorLocation());
		if (Distance < DistanceToProxy)
		{
			DistanceToProxy = Distance;
			AnswerProxy = Proxy;
		}
	}

	if (!AnswerProxy)
	{
		return false;
	}

	// ѕреобразуем локальную позицию обратно в мировую
	WorldPosition = AnswerProxy->LandscapeActorToWorld().TransformPosition(FVector(LocalPosition.X, LocalPosition.Y, 0.0f));

	return true;
}
