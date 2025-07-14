// Fill out your copyright notice in the Description page of Project Settings.

#include "PCGSimpleLandscapeSettings.h"
#include "PCGContext.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h" 
#include "Kismet/GameplayStatics.h" 
#include "Engine/World.h" 
#include "Engine/EngineTypes.h" 
#include "PCGContext.h" 
#include "PCGComponent.h"
#include "Data/PCGSpatialData.h"
#include "Data/PCGPointData.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Data/PCGSplineData.h"
#include "Helpers/PCGHelpers.h"
#include "SimpleLandscape.h"

#define LOCTEXT_NAMESPACE "PCGTraceProjectionSettingsElement"


void UPCGSimpleLandscapeData::Initialize(const TArray<TWeakObjectPtr<ASimpleLandscape>>& InLandscapes, const FBox& InBounds)
{
    Landscapes.Empty();
    for (TWeakObjectPtr<ASimpleLandscape> Landscape : InLandscapes)
    {
        Landscapes.Add(Landscape.Get());
    }

    ASimpleLandscape* FirstLandscape = Landscapes[0].Get();

    Bounds = InBounds;
    Transform = FirstLandscape->GetActorTransform();
}


FBox UPCGSimpleLandscapeData::GetBounds() const
{
    return Bounds;
}

FBox UPCGSimpleLandscapeData::GetStrictBounds() const
{
    return Bounds;
}

bool UPCGSimpleLandscapeData::SamplePoint(const FTransform& InTransform, const FBox& InBounds, FPCGPoint& OutPoint, UPCGMetadata* OutMetadata) const
{
    if (ProjectPoint(InTransform, InBounds, {}, OutPoint, OutMetadata))
    {
        if (InBounds.IsValid)
        {
            return FMath::PointBoxIntersection(OutPoint.Transform.GetLocation(), InBounds.TransformBy(InTransform));
        }
        else
        {
            return (InTransform.GetLocation() - OutPoint.Transform.GetLocation()).SquaredLength() < UE_SMALL_NUMBER;
        }
    }

    return false;
}

bool UPCGSimpleLandscapeData::ProjectPoint(const FTransform& InTransform, const FBox& InBounds, const FPCGProjectionParams& InParams, FPCGPoint& OutPoint, UPCGMetadata* OutMetadata) const
{
    FVector ProjectWorldVector;
    bool Sucsess = false;

    for (TSoftObjectPtr<ASimpleLandscape> SimpleLandscape : Landscapes)
    {
        if (SimpleLandscape.Get()->ProjectWorldPoint(InTransform.GetLocation(), ProjectWorldVector))
        {
            Sucsess = true;
            break;
        }
        continue;
    }

    // Respect projection settings
    if (InParams.bProjectPositions)
    {
        OutPoint.Transform.SetLocation(ProjectWorldVector);
    }
    if (InParams.bProjectRotations)
    {
        OutPoint.Transform.SetRotation(InTransform.GetRotation());
    }
    if (InParams.bProjectScales)
    {
        OutPoint.Transform.SetScale3D(InTransform.GetScale3D());
    }

    return true;
}

UPCGSpatialData* UPCGSimpleLandscapeData::CopyInternal() const
{
    UPCGSimpleLandscapeData* NewLandsacapeData = NewObject<UPCGSimpleLandscapeData>();

    CopyBaseSurfaceData(NewLandsacapeData);

    NewLandsacapeData->Landscapes = Landscapes;
    NewLandsacapeData->Bounds = Bounds;

    return NewLandsacapeData;
}

const UPCGPointData* UPCGSimpleLandscapeData::CreatePointData(FPCGContext* Context, const FBox& InBounds) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UPCGLandscapeData::CreatePointData);

    UPCGPointData* Data = NewObject<UPCGPointData>();
    Data->InitializeFromData(this);
    TArray<FPCGPoint>& Points = Data->GetMutablePoints();

    FBox EffectiveBounds = Bounds;
    if (InBounds.IsValid)
    {
        EffectiveBounds = Bounds.Overlap(InBounds);
    }

    // Early out
    if (!EffectiveBounds.IsValid)
    {
        return Data;
    }

    bool Sucsess = false;

    for (TSoftObjectPtr<ASimpleLandscape> SimpleLandscape : Landscapes)
    {
        if (SimpleLandscape.Get()->CheckLandscapeInRange(EffectiveBounds))
        {
            FVector2D MinGlobalVector;
            FVector2D MaxGlobalVector;
            SimpleLandscape.Get()->TransformWorldVectorToGlobalVector(EffectiveBounds.Min, MinGlobalVector);
            SimpleLandscape.Get()->TransformWorldVectorToGlobalVector(EffectiveBounds.Max, MaxGlobalVector);

            TArray<TSoftObjectPtr<ASimpleLandscapePart>> SimpleLandscapeParts = SimpleLandscape.Get()->FindLandscapePartsInGlobalRange(MinGlobalVector, MaxGlobalVector);
            for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : SimpleLandscapeParts)
            {
                for (int IndexX = 0; IndexX <= SimpleLandscapePart->PartSize/SimpleLandscapePart->GridStep; IndexX++)
                {
                    for (int IndexY = 0; IndexY<= SimpleLandscapePart->PartSize / SimpleLandscapePart->GridStep; IndexY++)
                    {
                        FVector WorldLocation = FVector(SimpleLandscapePart->GetActorLocation().X + SimpleLandscapePart->GridStep * IndexX, SimpleLandscapePart->GetActorLocation().Y + SimpleLandscapePart->GridStep * IndexY, SimpleLandscapePart->GetActorLocation().Z);
                        FVector ProjectPoint;
                        SimpleLandscape->ProjectWorldPoint(WorldLocation, ProjectPoint);
                        if (EffectiveBounds.IsInside(ProjectPoint))
                        {
                            FPCGPoint& Point = Points.Emplace_GetRef();
                            Point.Transform.SetLocation(WorldLocation);
                            Point.Transform.SetScale3D(FVector(1, 1, 1));
                        }
                    }
                }
            }
            break;
        }
        continue;
    }
    return Data;
}

UPCGGetSimpleLandscapeSettings::UPCGGetSimpleLandscapeSettings()
{
    bDisplayModeSettings = false;
    Mode = EPCGGetDataFromActorMode::ParseActorComponents;
    ActorSelector.bShowActorFilter = false;
    ActorSelector.bIncludeChildren = false;
    ActorSelector.bShowActorSelectionClass = false;
    ActorSelector.bSelectMultiple = true;
    ActorSelector.bShowSelectMultiple = false;

    if (PCGHelpers::IsNewObjectAndNotDefault(this))
    {
        ActorSelector.ActorFilter = EPCGActorFilter::AllWorldActors;
        ActorSelector.bMustOverlapSelf = true;
        ActorSelector.ActorSelection = EPCGActorSelection::ByClass;
        ActorSelector.ActorSelectionClass = ASimpleLandscape::StaticClass();
    }
}

#if WITH_EDITOR
FText UPCGGetSimpleLandscapeSettings::GetNodeTooltipText() const
{
    return LOCTEXT("GetSimpleLandscapeTooltip", "Builds a collection of SIMPLE landscapes from the selected actors.");
}
#endif
FName UPCGGetSimpleLandscapeSettings::AdditionalTaskName() const
{
    // Do not use the version from data from actor otherwise we'll show the selected actor class, which serves no purpose
    return UPCGSettings::AdditionalTaskName();
}

TArray<FPCGPinProperties> UPCGGetSimpleLandscapeSettings::OutputPinProperties() const
{
    TArray<FPCGPinProperties> PinProperties;
    PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Landscape, /*bAllowMultipleConnections=*/true, /*bAllowMultipleData=*/false);

    return PinProperties;
}

FPCGElementPtr UPCGGetSimpleLandscapeSettings::CreateElement() const
{
    return MakeShared<FPCGGetSimpleLandscapeDataElement>();
}

TSubclassOf<AActor> UPCGGetSimpleLandscapeSettings::GetDefaultActorSelectorClass() const
{
    return ASimpleLandscape::StaticClass();
}

void FPCGGetSimpleLandscapeDataElement::ProcessActors(FPCGContext* Context, const UPCGDataFromActorSettings* InSettings, const TArray<AActor*>& FoundActors) const
{
    check(Context);
    check(InSettings);

    const UPCGGetSimpleLandscapeSettings* Settings = CastChecked<UPCGGetSimpleLandscapeSettings>(InSettings);

    // In the base class (FPCGDataFromActorElement) we'd go through all actors, one by one and call
    // UPCGComponent::CreateActorPCGDataCollection, and push the results to the output.
    // However, in this case, we want to do what's done in UPCGComponent::GetLandscapeData,
    // which is to create a single UPCGLandscapeData keeping tabs on all selected landscapes.
    TArray<TWeakObjectPtr<ASimpleLandscape>> Landscapes;
    FBox LandscapeBounds(EForceInit::ForceInit);
    TSet<FString> LandscapeTags;

    for (AActor* FoundActor : FoundActors)
    {
        if (!FoundActor || !IsValid(FoundActor))
        {
            continue;
        }

        ASimpleLandscape* Landscape = Cast<ASimpleLandscape>(FoundActor);
        if (ensure(Landscape))
        {
            Landscapes.Add(Landscape);
            for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : Landscape->LandscapeParts)
            {
                LandscapeBounds += PCGHelpers::GetGridBounds(SimpleLandscapePart.Get(), nullptr);
            }

            for (FName Tag : Landscape->Tags)
            {
                LandscapeTags.Add(Tag.ToString());
            }
        }
    }

    if (!Landscapes.IsEmpty())
    {
        LandscapeBounds.Min = LandscapeBounds.Min - FVector(10, 10, 10);
        LandscapeBounds.Max = LandscapeBounds.Max + FVector(10, 10, 10);

        UPCGSimpleLandscapeData* LandscapeData = NewObject<UPCGSimpleLandscapeData>();
        LandscapeData->Initialize(Landscapes, LandscapeBounds);

        FPCGTaggedData& TaggedData = Context->OutputData.TaggedData.Emplace_GetRef();
        TaggedData.Data = LandscapeData;
        TaggedData.Tags = LandscapeTags;
    }
}

void FPCGGetSimpleLandscapeDataElement::ProcessActor(FPCGContext* Context, const UPCGDataFromActorSettings* Settings, AActor* FoundActor) const
{
    checkf(false, TEXT("This should never be called directly"));
}
