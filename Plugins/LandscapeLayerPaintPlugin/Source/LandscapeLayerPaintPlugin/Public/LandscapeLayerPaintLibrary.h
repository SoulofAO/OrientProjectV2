// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LandscapeLayerPaintLibrary.generated.h"

/**
 * 
 */

class ULandscapeHeightfieldCollisionComponent;
class ALandscapeProxy;

struct FProcessLandscapeTraceHitsResult
{
    FVector HitLocation;
    ULandscapeHeightfieldCollisionComponent* HeightfieldComponent;
    ALandscapeProxy* LandscapeProxy;
};

struct FHitResult;


UCLASS()
class LANDSCAPELAYERPAINTPLUGIN_API ULandscapeLayerPaintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable)
    static ULandscapeLayerInfoObject* CreateLayerInfo(UObject* Outer, const FString& LayerName);

    UFUNCTION(BlueprintCallable)
    static void PaintLandscapeLayer(ULandscapeInfo* LandscapeInfo, ULandscapeLayerInfoObject* LandscapeLayerInfoObject, FVector2D Position, FVector2D Scale, int Power = 20);
#endif
    UFUNCTION(BlueprintCallable)
    static bool WorldPositionToLandscapePosition(FVector Position, ALandscape* Landscape, FVector2D& AnswerPosition);

    UFUNCTION(BlueprintCallable)
    static bool LandscapePositionToWorldPosition(FVector2D LocalPosition, ALandscape* Landscape, FVector& WorldPosition);
};
