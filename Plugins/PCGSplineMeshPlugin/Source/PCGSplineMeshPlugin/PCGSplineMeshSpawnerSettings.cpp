// Fill out your copyright notice in the Description page of Project Settings.


#include "PCGSplineMeshSpawnerSettings.h"
#include "PCGContext.h"
#include "Data/PCGSpatialData.h"
#include "Data/PCGPointData.h"
#include "Components/SplineMeshComponent.h"
#include "PCGComponent.h"

static TAutoConsoleVariable<bool> CVarAllowISMReuse(
	TEXT("pcg.ISM.AllowReuse"),
	true,
	TEXT("Controls whether ISMs can be reused and skipped when re-executing"));

UPCGSplineMeshSpawnerSettings::UPCGSplineMeshSpawnerSettings(const FObjectInitializer& ObjectInitializer)
{
	bUseSeed = true;
}

#if WITH_EDITOR
FText UPCGSplineMeshSpawnerSettings::GetDefaultNodeTitle() const
{
	return FText::FromString("Spline Mesh Spawner");
}
#endif

FPCGElementPtr UPCGSplineMeshSpawnerSettings::CreateElement() const
{
	return MakeShared<FPCGSplineMeshSpawnerElement>();
}

FPCGContext* FPCGSplineMeshSpawnerElement::CreateContext()
{
	return new FPCGSplineMeshSpawnerContext();
}

bool FPCGSplineMeshSpawnerElement::PrepareDataInternal(FPCGContext* Context) const
{
	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputs();
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;
	return true;
}


bool FPCGSplineMeshSpawnerElement::ExecuteInternal(FPCGContext* InContext) const
{
	FPCGSplineMeshSpawnerContext* Context = static_cast<FPCGSplineMeshSpawnerContext*>(InContext);
	const UPCGSplineMeshSpawnerSettings* Settings = Context->GetInputSettings<UPCGSplineMeshSpawnerSettings>();
	UPCGManagedSplineMeshComponentOldVersion * PCGManagedSplineMeshComponent = CreatePCGManagedSplineMeshComponent(Context);
	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputs();
	for(FPCGTaggedData Input: Inputs)
	{ 
		const UPCGSpatialData* SpatialData = Cast<UPCGSpatialData>(Input.Data);
		for (int x = 0; x < SpatialData->ToPointData()->GetPoints().Num() - 1; x++)
		{
			FVector StartLocation = SpatialData->ToPointData()->GetPoints()[x].Transform.GetLocation();
			FVector StartTangent = SpatialData->ToPointData()->GetPoints()[x].Transform.GetRotation().GetForwardVector();
			FVector EndLocation = SpatialData->ToPointData()->GetPoints()[x + 1].Transform.GetLocation();
			FVector EndTangent = SpatialData->ToPointData()->GetPoints()[x + 1].Transform.GetRotation().GetForwardVector();
			FVector UpDir = SpatialData->ToPointData()->GetPoints()[x].Transform.GetRotation().GetUpVector();
			AddNewSplineMesh(Context, PCGManagedSplineMeshComponent, Settings->StaticMesh, StartLocation, StartTangent, EndLocation, EndTangent, UpDir);
		}
	}
	return true;
}

bool FPCGSplineMeshSpawnerElement::CanExecuteOnlyOnMainThread(FPCGContext* Context) const
{
	return Context->CurrentPhase == EPCGExecutionPhase::Execute || Context->CurrentPhase == EPCGExecutionPhase::PrepareData;
}

void FPCGSplineMeshSpawnerElement::AbortInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGSplineMeshSpawnerElement::Execute);
	FPCGSplineMeshSpawnerContext* Context = static_cast<FPCGSplineMeshSpawnerContext*>(InContext);

	// Any resources we've touched during the execution of this node can potentially be in a "not-quite complete state" especially if we have multiple sources of data writing to the same ISMC.
	// In this case, we're aiming to mark the resources as "Unused" so they are picked up to be removed during the component's OnProcessGraphAborted, which is why we call Release here.
	TWeakObjectPtr<UPCGManagedSplineMeshComponentOldVersion  > ManagedResource = Context->ResourceObject;
	if (ManagedResource.IsValid())
	{
		TSet<TSoftObjectPtr<AActor>> Dummy;
		ManagedResource->Release(/*bHardRelease=*/false, Dummy);
	}

}

void FPCGSplineMeshSpawnerElement::AddNewSplineMesh(FPCGSplineMeshSpawnerContext* Context, UPCGManagedSplineMeshComponentOldVersion * ManagedSplineMeshComponent, UStaticMesh* StaticMesh, FVector StartPosition,FVector StartTangent, FVector EndPosition, FVector EndTangent, FVector UpDir) const
{

	AActor* TargetActor = Context->SourceComponent.Get()->GetOwner();
	const EObjectFlags ObjectFlags = (Context->SourceComponent.Get()->IsInPreviewMode() ? RF_Transient : RF_NoFlags);
	USplineMeshComponent* NewSplineMeshComponent = NewObject<USplineMeshComponent>(Context->SourceComponent.Get());
	NewSplineMeshComponent->RegisterComponent();
	TargetActor->AddInstanceComponent(NewSplineMeshComponent);
	NewSplineMeshComponent->AttachToComponent(TargetActor->GetRootComponent(), FAttachmentTransformRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false));
	NewSplineMeshComponent->SetStaticMesh(StaticMesh);
	NewSplineMeshComponent->SetStartAndEnd(StartPosition, StartTangent, EndPosition, EndTangent, false);
	NewSplineMeshComponent->SetSplineUpDir(UpDir);
	ManagedSplineMeshComponent->AddComponent(NewSplineMeshComponent);
}

UPCGManagedSplineMeshComponentOldVersion * FPCGSplineMeshSpawnerElement::CreatePCGManagedSplineMeshComponent(FPCGSplineMeshSpawnerContext* Context)
{
	Context->SourceComponent.Get()->GetOwner()->Modify(!Context->SourceComponent.Get()->IsInPreviewMode());

	// Done as in InstancedStaticMesh.cpp

	FString ComponentName;
	TSubclassOf<UInstancedStaticMeshComponent> ComponentClass = USplineMeshComponent::StaticClass();

	// Create managed resource on source component
	UPCGManagedSplineMeshComponentOldVersion * Resource = NewObject<UPCGManagedSplineMeshComponentOldVersion >(Context->SourceComponent.Get());
	Context->SourceComponent.Get()->AddToManagedResources(Resource);
	Resource->SetCrc(Context->DependenciesCrc);
	return Resource;
}

bool UPCGManagedSplineMeshComponentOldVersion ::Release(bool bHardRelease, TSet<TSoftObjectPtr<AActor>>& OutActorsToDelete)
{
	if (!GetComponents().IsEmpty())
	{
		while (GetComponents().Num() > 0)
		{
			if (GetComponents()[0])
			{
				GetComponents()[0]->DestroyComponent();
			}
			RemoveComponentByIndex(0);
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool UPCGManagedSplineMeshComponentOldVersion  ::ReleaseIfUnused(TSet<TSoftObjectPtr<AActor>>& OutActorsToDelete)
{
	if (Super::ReleaseIfUnused(OutActorsToDelete) || GetComponents().IsEmpty())
	{
		return true;
	}
	return false;
}

void UPCGManagedSplineMeshComponentOldVersion  ::ResetComponent()
{
	while (GetComponents().Num() > 0)
	{
		if (GetComponents()[0])
		{
			GetComponents()[0]->DestroyComponent();
		}
		RemoveComponentByIndex(0);
	}
}


TArray<TSoftObjectPtr<USplineMeshComponent>> UPCGManagedSplineMeshComponentOldVersion ::GetComponents() const
{
	return GeneratedComponents;
}

void UPCGManagedSplineMeshComponentOldVersion ::AddComponent(USplineMeshComponent* NewComponent)
{
	GeneratedComponents.Add(NewComponent);
}

void UPCGManagedSplineMeshComponentOldVersion ::RemoveComponent(USplineMeshComponent* RemoveComponent)
{
	GeneratedComponents.Remove(RemoveComponent);
}

void UPCGManagedSplineMeshComponentOldVersion ::RemoveComponentByIndex(int Index)
{
	GeneratedComponents.RemoveAt(Index);
}
