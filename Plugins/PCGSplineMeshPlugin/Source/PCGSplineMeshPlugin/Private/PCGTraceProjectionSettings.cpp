// Fill out your copyright notice in the Description page of Project Settings.

#include "PCGTraceProjectionSettings.h"
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

#define LOCTEXT_NAMESPACE "PCGTraceProjectionSettingsElement"

#if WITH_EDITOR
// The label the node is known as internally.
FName UPCGTraceProjectionSettingsSettings::GetDefaultNodeName() const
{
	return FName(TEXT("PCGTraceProjectionSettings"));
}

// Default node name shown in the graph editor. Include spaces.
FText UPCGTraceProjectionSettingsSettings::GetDefaultNodeTitle() const
{
	return LOCTEXT("NodeTitle", "PCGTraceProjectionSettings");
}

// Default tooltip for the node
FText UPCGTraceProjectionSettingsSettings::GetNodeTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Add tooltip here.");
}
#endif //WITH_EDITOR

// Input/Output pin setup with specific properties, including:
// Pin data type, allowing singular or multiple inputs per pin, and creating multiple in/out pins.
TArray<FPCGPinProperties> UPCGTraceProjectionSettingsSettings::InputPinProperties() const
{
	return Super::InputPinProperties();
}

TArray<FPCGPinProperties> UPCGTraceProjectionSettingsSettings::OutputPinProperties() const
{
	return Super::OutputPinProperties();
}

// Creates the Element to be used for ExecuteInternal.
FPCGElementPtr UPCGTraceProjectionSettingsSettings::CreateElement() const
{
	return MakeShared<FPCGTraceProjectionSettingsElement>();
}

/*
* Processing function for this node. 
* Context holds the InputData, containing the input data collection for this node 
* and the OutputData, the output data collection to write to as output.
* Returns true if the processing is done. 
* Returning false will call back this function at next tick, and will call it until it returns true.
* Settings contains all the setup options for this node, and if a property was marked PCG_Overridable, 
* "Context->GetInputSettings" will contain the overridden value for this property if it is overridden
*/ 
bool FPCGTraceProjectionSettingsElement::ExecuteInternal(FPCGContext* Context) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FPCGTraceProjectionSettingsElement::Execute);

    check(Context);

    const UPCGTraceProjectionSettingsSettings* Settings = Context->GetInputSettings<UPCGTraceProjectionSettingsSettings>();
    check(Settings);

    TArray<AActor*> ActorsInclude;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, Settings->FilterActorClass, ActorsInclude);
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    for (AActor* Actor : ActorsInclude)
    {
        AllActors.Remove(Actor);
    }

    for (FPCGTaggedData& Input : Context->InputData.GetInputs())
    {
        const UPCGSpatialData* SpatialData = Cast<UPCGSpatialData>(Input.Data);
        TArray<FPCGPoint> Points = SpatialData->ToPointData()->GetPoints();

        FHitResult HitResultDown;
        FCollisionQueryParams CollisionParams;
        CollisionParams.AddIgnoredActor(Context->SourceComponent->GetOwner());
        CollisionParams.AddIgnoredActors(AllActors);

        FPCGTaggedData& Output = Context->OutputData.TaggedData.Emplace_GetRef();
        UPCGPointData* OutSampledPointData = NewObject<UPCGPointData>();
        OutSampledPointData->InitializeFromData(SpatialData);
        Output.Data = OutSampledPointData;

        for (int x = 0; x < Points.Num(); x++)
        {
            float NearestDistance = FLT_MAX;
            FVector NearestPoint;

            TArray<FVector> OutputPoints;
            if (GWorld->LineTraceSingleByChannel(HitResultDown, Points[x].Transform.GetLocation(), Points[x].Transform.GetLocation() - FVector(0, 0, 100000), ECC_Visibility, CollisionParams))
            {
                FVector HitPoint = HitResultDown.ImpactPoint;
                float Distance = FVector::Dist(Points[x].Transform.GetLocation(), HitPoint);

                if (Distance < NearestDistance)
                {
                    NearestDistance = Distance;
                    NearestPoint = HitPoint;
                }
            }


            FHitResult HitResultUp;
            if (GWorld->LineTraceSingleByChannel(HitResultUp, Points[x].Transform.GetLocation(), Points[x].Transform.GetLocation() + FVector(0, 0, 10000), ECC_Visibility, CollisionParams))
            {
                FVector HitPoint = HitResultUp.ImpactPoint;
                float Distance = FVector::Dist(Points[x].Transform.GetLocation(), HitPoint);

                if (Distance < NearestDistance)
                {
                    NearestDistance = Distance;
                    NearestPoint = HitPoint;
                }
            }

            if (NearestDistance < FLT_MAX)
            {
                Points[x].Transform.SetLocation(NearestPoint);
            }
        }
        OutSampledPointData->SetPoints(Points);
    }

    return true;
}
#if WITH_EDITOR
FName UPCGFilterNumberSplinePointSettings::GetDefaultNodeName() const
{
    return FName(TEXT("PCGFilterNumberSplinePointSettings"));
}

FText UPCGFilterNumberSplinePointSettings::GetDefaultNodeTitle() const
{
    return LOCTEXT("NodeTitle", "PCGFilterNumberSplinePointSettings");
}

FText UPCGFilterNumberSplinePointSettings::GetNodeTooltipText() const
{
    return LOCTEXT("NodeTooltip", "Add tooltip here.");
}
#endif

TArray<FPCGPinProperties> UPCGFilterNumberSplinePointSettings::InputPinProperties() const
{
    return Super::InputPinProperties();
}

TArray<FPCGPinProperties> UPCGFilterNumberSplinePointSettings::OutputPinProperties() const
{
    return Super::OutputPinProperties();
}

FPCGElementPtr UPCGFilterNumberSplinePointSettings::CreateElement() const
{
    return MakeShared<FPCGFilterNumberSplinePointSettingsElement>();
}

bool FPCGFilterNumberSplinePointSettingsElement::ExecuteInternal(FPCGContext* Context) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FPCGTraceProjectionSettingsElement::Execute);

    check(Context);

    const UPCGFilterNumberSplinePointSettings* Settings = Context->GetInputSettings<UPCGFilterNumberSplinePointSettings>();
    check(Settings);

    TArray<FPCGTaggedData> TaggedDataArray;

    for (FPCGTaggedData& Input : Context->InputData.GetInputs())
    {
        const UPCGSplineData* SplineData = Cast<UPCGSplineData>(Input.Data);

        if (SplineData->SplineStruct.SplineCurves.Position.Points.Num() > Settings->NumberIndex)
        {
            FPCGTaggedData PCGTaggedData;
            PCGTaggedData.Data = SplineData;
            TaggedDataArray.Add(PCGTaggedData);
        }
    }
    Context->OutputData.TaggedData = TaggedDataArray;
    return true;
}
