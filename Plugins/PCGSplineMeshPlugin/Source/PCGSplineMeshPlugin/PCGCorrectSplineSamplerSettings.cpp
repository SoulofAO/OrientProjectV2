// Fill out your copyright notice in the Description page of Project Settings.


#include "PCGCorrectSplineSamplerSettings.h"
#include "Data/PCGPolyLineData.h"
#include "Kismet/KismetMathLibrary.h"
#include "PCGContext.h"
#include "Data/PCGSplineData.h"
#include "Data/PCGPointData.h"
#include "Elements/PCGSplineSampler.h"
#include "PCGComponent.h"

class UPCGSplineProjectionData;

FPCGElementPtr UPCGCorrectSplineSamplerSettings::CreateElement() const
{
	return MakeShared<FPCGCorrectSplineSamplerElement>();
}

TArray<FPCGPinProperties> UPCGCorrectSplineSamplerSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	FPCGPinProperties& SplinePinProperty = PinProperties.Emplace_GetRef(PCGSplineSamplerConstants::SplineLabel, EPCGDataType::PolyLine, /*bAllowMultipleConnections=*/true, /*bAllowMultipleData=*/true);
	SplinePinProperty.SetRequiredPin();

	return PinProperties;
}



bool FPCGCorrectSplineSamplerElement::ExecuteInternal(FPCGContext* Context) const
{
	const UPCGCorrectSplineSamplerSettings* Settings = Context->GetInputSettings<UPCGCorrectSplineSamplerSettings>();
	check(Settings);

	TArray<FPCGTaggedData> SplineInputs = Context->InputData.GetInputsByPin(PCGSplineSamplerConstants::SplineLabel);

	const UPCGSpatialData* BoundingShape = nullptr;

	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;
	

	for (FPCGTaggedData& Input : SplineInputs)
	{
		TArray<FPCGPoint> Points;
		const UPCGSplineData* SplineData = Cast<UPCGSplineData>(Input.Data);
		if (!SplineData)
		{
			continue;
		}

		UPCGSpatialData* ProjectionTarget = nullptr;
		FPCGProjectionParams ProjectionParams;
		if (const UPCGSplineProjectionData* SplineProjection = Cast<const UPCGSplineProjectionData>(Input.Data))
		{
			ProjectionTarget = nullptr;
			ProjectionParams = SplineProjection->GetProjectionParams();
		}

		FPCGTaggedData& Output = Outputs.Emplace_GetRef();
		Output = Input;

		UPCGPointData* SampledPointData = NewObject<UPCGPointData>();
		SampledPointData->InitializeFromData(SplineData);
		Output.Data = SampledPointData;
		
		TArray<FVector> VectorPoints;
		if (SplineData->SplineStruct.GetSplinePointsPosition().Points.Num() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Number Of Spline Point At Actor With Name %s Is ZERO"), *Context->SourceComponent->GetOwner()->GetName());
			return true;
		}
		for (FInterpCurvePoint<FVector> Vector : SplineData->SplineStruct.GetSplinePointsPosition().Points)
		{
			VectorPoints.Add(Vector.OutVal + SplineData->SplineStruct.GetTransform().GetLocation());
		}
		if (SplineData->IsClosed())
		{

			VectorPoints.Add(SplineData->SplineStruct.GetSplinePointsPosition().Points[0].OutVal + SplineData->SplineStruct.GetTransform().GetLocation());
		}

		for (int Count = 0; Count< VectorPoints.Num()-1; Count++)
		{
			float SegmentLength = SplineData->GetSegmentLength(Count);
			int NumberPoint = FMath::RoundToInt(SegmentLength / Settings->MeshSize);
			for (int Number = 0; Number < ((Count == VectorPoints.Num()-2)? NumberPoint + 1 : NumberPoint ); Number++)
			{
				FPCGPoint PCGPoint;
				FVector Position = UKismetMathLibrary::VLerp(VectorPoints[Count], VectorPoints[Count + 1], float(Number) / float(NumberPoint));
				PCGPoint.Transform = FTransform(UKismetMathLibrary::FindLookAtRotation(VectorPoints[Count], VectorPoints[Count + 1]), Position, FVector(1, 1, 1));
				Points.Add(PCGPoint);
			}

			
		}
		if (SplineData->IsClosed())
		{
			FPCGPoint PCGPoint;
			PCGPoint.Transform = FTransform(UKismetMathLibrary::FindLookAtRotation(VectorPoints[0], VectorPoints[1]), VectorPoints[0], FVector(1, 1, 1));
			Points.Add(PCGPoint);
		}
		SampledPointData->SetPoints(Points);
	}
	return true;
}
