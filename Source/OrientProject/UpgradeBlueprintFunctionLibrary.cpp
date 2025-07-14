#include "UpgradeBlueprintFunctionLibrary.h"
#include "Sound/SoundCue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SCS_Node.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Components/ActorComponent.h"
#include "Templates/SubclassOf.h"
#include "HAL/PlatformTime.h"
#include "Engine/StaticMesh.h"
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "RawMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "RawMesh.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "PCGComponent.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/LoaderAdapter/LoaderAdapterShape.h"
#include "LevelUtils.h"
#include "Engine/LevelStreaming.h"

#if WITH_EDITOR
#include "WorldPartition/WorldPartitionSubsystem.h"
#include "LevelEditor.h"
#include "Editor.h"
#include "FileHelpers.h"
#endif

UActorComponent* UUpgradeBlueprintFunctionLibrary::AddTransactionalInstanceComponent(TSubclassOf<UActorComponent> ActorComponentClass, AActor* OwnerActor)
{
    UActorComponent* NewComponent = NewObject<UActorComponent>(OwnerActor, ActorComponentClass);
    NewComponent->SetFlags(RF_Transactional);
    OwnerActor->AddInstanceComponent(NewComponent);
    NewComponent->RegisterComponent();
    USceneComponent* NewSceneComponent = Cast<USceneComponent>(NewComponent);
    if (NewSceneComponent)
    {
        NewSceneComponent->AttachToComponent(OwnerActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    }
    return NewComponent;
}

void UUpgradeBlueprintFunctionLibrary::SetSoundClass(USoundBase* SoundBase, USoundClass* SoundClass)
{
	SoundBase->SoundClassObject = SoundClass;
}

void UUpgradeBlueprintFunctionLibrary::MadeAssetDirty(UObject* Object)
{
	Object->MarkPackageDirty();
}

void UUpgradeBlueprintFunctionLibrary::SetSoundAttenuation(USoundCue* SoundCue, USoundAttenuation* SoundAttenuation)
{
	SoundCue->AttenuationSettings = SoundAttenuation;
}

void UUpgradeBlueprintFunctionLibrary::CreatePhysicalProxy(UGeometryCollectionComponent* GeometryCollectionComponent)
{
	GeometryCollectionComponent->SetDynamicState(Chaos::EObjectStateType::Dynamic);
}

UObject* UUpgradeBlueprintFunctionLibrary::GetCDOObject(TSubclassOf<UObject> Object)
{
	return Object->GetDefaultObject();
}

void UUpgradeBlueprintFunctionLibrary::DirectlyDestroyComponent(UActorComponent* ActorComponentToDestroy)
{
	if (IsValid(ActorComponentToDestroy))
	{
		ActorComponentToDestroy->DestroyComponent();
	}
}

void UUpgradeBlueprintFunctionLibrary::DestroyController(AController* ControllerToDestroy)
{
	if (IsValid(ControllerToDestroy))
	{
		ControllerToDestroy->Destroy(true, true);
	}
}


void UUpgradeBlueprintFunctionLibrary::SetGravityToCharacterMovement(UCharacterMovementComponent* CharacterMovementComponent, FVector NewGravity)
{
	CharacterMovementComponent->SetGravityDirection(NewGravity);
}

FVector UUpgradeBlueprintFunctionLibrary::GetGravityFromCharacterMovement(UCharacterMovementComponent* CharacterMovementComponent)
{
	return CharacterMovementComponent->GetGravityDirection();
}

UActorComponent* UUpgradeBlueprintFunctionLibrary::GetDefaultComponentByActorClass(TSubclassOf<UActorComponent> ClassActorComponent, TSubclassOf<AActor> ClassActor)
{
	if (!IsValid(ClassActor))
	{
		return nullptr;
	}


	AActor* ActorCDO = ClassActor->GetDefaultObject<AActor>();
	UActorComponent* FoundComponent = ActorCDO->FindComponentByClass(ClassActorComponent);

	if (FoundComponent != nullptr)
	{
		return FoundComponent;
	}

	UBlueprintGeneratedClass* RootBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(ClassActor);
	UClass* ActorClass = ClassActor;

	do
	{
		UBlueprintGeneratedClass* ActorBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(ActorClass);
		if (!ActorBlueprintGeneratedClass)
		{
			return nullptr;
		}

		const TArray<USCS_Node*>& ActorBlueprintNodes =
			ActorBlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes();

		for (USCS_Node* Node : ActorBlueprintNodes)
		{
			if (Node->ComponentClass->IsChildOf(ClassActorComponent))
			{
				return Node->GetActualComponentTemplate(RootBlueprintGeneratedClass);
			}
		}

		ActorClass = Cast<UClass>(ActorClass->GetSuperStruct());

	} while (ActorClass != AActor::StaticClass());

	return nullptr;
}

FVector2D UUpgradeBlueprintFunctionLibrary::GetMinVector(const TArray<FVector2D>& Vectors)
{
	if (Vectors.Num() == 0)
	{
		return FVector2D(0.0f, 0.0f);
	}

	FVector2D MinVector = Vectors[0];

	for (const FVector2D& Vec : Vectors)
	{
		MinVector.X = FMath::Min(MinVector.X, Vec.X);
		MinVector.Y = FMath::Min(MinVector.Y, Vec.Y);
	}

	return MinVector;
}

FVector2D UUpgradeBlueprintFunctionLibrary::GetMaxVector(const TArray<FVector2D>& Vectors)
{
	if (Vectors.Num() == 0)
	{
		return FVector2D(0.0f, 0.0f);
	}

	FVector2D MaxVector = Vectors[0];

	for (const FVector2D& Vec : Vectors)
	{
		MaxVector.X = FMath::Max(MaxVector.X, Vec.X);
		MaxVector.Y = FMath::Max(MaxVector.Y, Vec.Y);
	}

	return MaxVector;
}

FVector3f ConvertVectorToVector3F(FVector Vector)
{
	FVector3f ConvertVector = FVector3f(Vector.X, Vector.Y, Vector.Z);
	return ConvertVector;
}

FVector2f ConvertVectorToVector2F(FVector2D Vector)
{
	FVector2f ConvertVector = FVector2f(Vector.X, Vector.Y);
	return ConvertVector;
}
#if WITH_EDITOR
UStaticMesh* UUpgradeBlueprintFunctionLibrary::ConvertProceduralToStatic(UProceduralMeshComponent* ProceduralMesh, const FString& MeshName, const FString& PackagePath)
{
	if (!ProceduralMesh) return nullptr;

	FString FullPackageName = PackagePath + TEXT("/") + MeshName;
	UPackage* Package = CreatePackage(*FullPackageName);
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, *MeshName, RF_Public | RF_Standalone);
	StaticMesh->InitResources();

	FRawMesh RawMesh;
	TArray<UMaterialInterface*> MeshMaterials;
	int32 VertexBase = 0;

	for (int32 SectionIndex = 0; SectionIndex < ProceduralMesh->GetNumSections(); SectionIndex++)
	{
		FProcMeshSection* ProcSection = ProceduralMesh->GetProcMeshSection(SectionIndex);
		if (!ProcSection) continue;

		for (FProcMeshVertex& Vert : ProcSection->ProcVertexBuffer)
		{
			RawMesh.VertexPositions.Add(ConvertVectorToVector3F(Vert.Position));
		}

		int32 NumIndices = ProcSection->ProcIndexBuffer.Num();
		for (int32 Index = 0; Index < NumIndices; Index++)
		{
			RawMesh.WedgeIndices.Add(ProcSection->ProcIndexBuffer[Index] + VertexBase);
			FProcMeshVertex& ProcVertex = ProcSection->ProcVertexBuffer[ProcSection->ProcIndexBuffer[Index]];
			RawMesh.WedgeTangentX.Add(ConvertVectorToVector3F(ProcVertex.Tangent.TangentX));
			FVector TangentY = FVector::CrossProduct(ProcVertex.Tangent.TangentX, ProcVertex.Normal).GetSafeNormal();
			RawMesh.WedgeTangentY.Add(ConvertVectorToVector3F(TangentY));
			RawMesh.WedgeTangentZ.Add(ConvertVectorToVector3F(ProcVertex.Normal));
			RawMesh.WedgeTexCoords[0].Add(ConvertVectorToVector2F(ProcVertex.UV0));
			RawMesh.WedgeColors.Add(ProcVertex.Color);
		}

		int32 NumTris = NumIndices / 3;
		for (int32 TriIndex = 0; TriIndex < NumTris; TriIndex++)
		{
			RawMesh.FaceMaterialIndices.Add(SectionIndex);
			RawMesh.FaceSmoothingMasks.Add(0);
		}

		MeshMaterials.Add(ProceduralMesh->GetMaterial(SectionIndex));
		VertexBase += ProcSection->ProcVertexBuffer.Num();
	}

	if (RawMesh.VertexPositions.Num() > 0 && RawMesh.WedgeIndices.Num() > 0)
	{
		StaticMesh->AddSourceModel();
		FStaticMeshSourceModel& SrcModel = StaticMesh->GetSourceModel(0);
		SrcModel.BuildSettings.bRecomputeNormals = false;
		SrcModel.BuildSettings.bRecomputeTangents = false;
		SrcModel.BuildSettings.bRemoveDegenerates = false;
		SrcModel.BuildSettings.bGenerateLightmapUVs = true;
		SrcModel.RawMeshBulkData->SaveRawMesh(RawMesh);

		TArray< FStaticMaterial> Materials;
		for (UMaterialInterface* Material : MeshMaterials)
		{
			Materials.Add(FStaticMaterial(Material));
			
		}
		StaticMesh->SetStaticMaterials(Materials);
		StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;
		StaticMesh->Build(false);
		StaticMesh->PostEditChange();
		FAssetRegistryModule::AssetCreated(StaticMesh);
	}
	return StaticMesh;
}
#endif

void UUpgradeBlueprintFunctionLibrary::GetActorsWithSplinesOverlappingBox(const UObject* WorldContextObject, const FBox& BoundBox, TArray<AActor*>& OutActors, TSubclassOf<AActor> ActorClass, bool EqualExactly)
{
	if (!WorldContextObject) return;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return;

	for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
	{
		AActor* Actor = *It;
		if (EqualExactly && It->GetClass() != ActorClass)
		{
			continue;
		}

		TArray<USplineComponent*> SplineComponents;
		Actor->GetComponents(SplineComponents);

		for (USplineComponent* SplineComp : SplineComponents)
		{
			if (!SplineComp) continue;

			const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();
			for (int32 i = 0; i < NumPoints; ++i)
			{
				FVector PointLocation = SplineComp->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);

				if (BoundBox.IsInsideXY(PointLocation))
				{
					if (!OutActors.Contains(Actor))
					{
						OutActors.Add(Actor);
						break; 
					}
				}
			}
		}
	}
}


void UUpgradeBlueprintFunctionLibrary::GetActorsWithSplinesOverlappingBoxByIntersect(const UObject* WorldContextObject, const FBox& BoundBox, TArray<AActor*>& OutActors, TSubclassOf<AActor> ActorClass, bool EqualExactly)
{
	if (!WorldContextObject) return;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return;

	for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
	{
		if (EqualExactly && It->GetClass() != ActorClass)
		{
			continue;
		}
		AActor* Actor = *It;
		TArray<USplineComponent*> SplineComponents;
		Actor->GetComponents(SplineComponents);

		for (USplineComponent* SplineComp : SplineComponents)
		{
			if (!SplineComp) continue;

			const FBox SplineBounds = SplineComp->Bounds.GetBox();
			if (BoundBox.Intersect(SplineBounds) || SplineBounds.IsInside(BoundBox.Min) && SplineBounds.IsInside(BoundBox.Max))
			{
				if (!OutActors.Contains(Actor))
				{
					OutActors.Add(Actor);
					break;
				}
			}
		}
	}
}

void UUpgradeBlueprintFunctionLibrary::RefreshPCGComponent(UPCGComponent* PCGComponent, EPCGChangeType ChangeType, bool bCancelExistingRefresh)
{
#if WITH_EDITOR
	PCGComponent->bDirtyGenerated = true;
	PCGComponent->Refresh(ChangeType, bCancelExistingRefresh);
#endif
}

void UUpgradeBlueprintFunctionLibrary::SetDirtyGenerated(UPCGComponent* PCGComponent)
{
#if WITH_EDITOR
	PCGComponent->bDirtyGenerated = true;
#endif
}

void UUpgradeBlueprintFunctionLibrary::PostEditMoveUpdate(UPCGComponent * PCGComponent, EPCGChangeType ChangeType, bool bCancelExistingRefresh)
{
#if WITH_EDITOR
		PCGComponent->GetOwner()->PostEditMove(true);
#endif
}

void UUpgradeBlueprintFunctionLibrary::CleanUpPCGComponent(UPCGComponent* PCGComponent)
{
#if WITH_EDITOR
	PCGComponent->CleanupLocal(true);
#endif
}

void UUpgradeBlueprintFunctionLibrary::ComponentChangedPCGComponent(UPCGComponent* PCGComponent)
{
	if (PCGComponent)
	{
#if WITH_EDITOR
		PCGComponent->OnRefresh(true);
#endif
	}
}



TArray<AActor*> UUpgradeBlueprintFunctionLibrary::FilterActorsByTag(const TArray<AActor*>& Actors, const FName& Tag)
{
	TArray<AActor*> FilteredActors;
	for (AActor* Actor : Actors)
	{
		if (Actor && Actor->ActorHasTag(Tag))
		{
			FilteredActors.Add(Actor);
		}
	}
	return FilteredActors;
}

UObject* UUpgradeBlueprintFunctionLibrary::LoadObjectFromSoftPath(const FSoftObjectPath& SoftPath)
{
	if (!SoftPath.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid SoftObjectPath."));
		return nullptr;
	}

	UObject* LoadedObject = SoftPath.TryLoad();
	if (!LoadedObject)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load object from path: %s"), *SoftPath.ToString());
	}

	return LoadedObject;
}

bool UUpgradeBlueprintFunctionLibrary::IsPointInsideSpline(const TArray<FVector>& SplinePoints, const FVector& Point)
{
	int32 Num = SplinePoints.Num();
	if (Num < 3)
	{
		return false; // Треугольник — минимально замыкаемая фигура
	}

	FVector2D TestPoint(Point.X, Point.Y);
	bool bInside = false;

	for (int32 i = 0, j = Num - 1; i < Num; j = i++)
	{
		const FVector2D Pi(SplinePoints[i].X, SplinePoints[i].Y);
		const FVector2D Pj(SplinePoints[j].X, SplinePoints[j].Y);

		bool bIntersect = ((Pi.Y > TestPoint.Y) != (Pj.Y > TestPoint.Y)) &&
			(TestPoint.X < (Pj.X - Pi.X) * (TestPoint.Y - Pi.Y) / (Pj.Y - Pi.Y + SMALL_NUMBER) + Pi.X);

		if (bIntersect)
		{
			bInside = !bInside;
		}
	}

	return bInside;
}

void UUpgradeBlueprintFunctionLibrary::GetOriginalSplinePoint(UPCGSplineData* PCGSplineData, TArray<FVector>& Positions)
{
	if (!PCGSplineData)
	{
		return;
	}

	Positions.Empty();
	for (auto Point : PCGSplineData->SplineStruct.GetSplinePointsPosition().Points)
	{
		Positions.Add(Point.OutVal);
	}
}

void UUpgradeBlueprintFunctionLibrary::GetWorldOriginalSplinePoint(UPCGSplineData* PCGSplineData, TArray<FVector>& Positions)
{
	if (!PCGSplineData)
	{
		return;
	}

	Positions.Empty();
	for (auto Point : PCGSplineData->SplineStruct.GetSplinePointsPosition().Points)
	{
		Positions.Add(Point.OutVal + PCGSplineData->SplineStruct.GetTransform().GetLocation());
	}
}

bool UUpgradeBlueprintFunctionLibrary::IsPointInTriangle2D(const FVector2D& P, const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
	auto Sign = [](const FVector2D& P1, const FVector2D& P2, const FVector2D& P3)
		{
			return (P1.X - P3.X) * (P2.Y - P3.Y) - (P2.X - P3.X) * (P1.Y - P3.Y);
		};

	float d1 = Sign(P, A, B);
	float d2 = Sign(P, B, C);
	float d3 = Sign(P, C, A);

	bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

	return !(hasNeg && hasPos); // точка внутри, если все знаки одинаковы
}

UWorldPartitionLoaderWrapper* UUpgradeBlueprintFunctionLibrary::LoadCell(const UObject* WorldContextObject, FBox SelectionBox)
{
#if WITH_EDITOR
	if (WorldContextObject->GetWorld())
	{
		SelectionBox.Min = FVector(SelectionBox.Min.X, SelectionBox.Min.Y, -HALF_WORLD_MAX);
		SelectionBox.Max = FVector(SelectionBox.Max.X, SelectionBox.Max.Y, HALF_WORLD_MAX);

		TWeakObjectPtr<UWorldPartition> WorldPartition = WorldContextObject->GetWorld()->GetWorldPartition();
		UWorldPartitionEditorLoaderAdapter* EditorLoaderAdapter = WorldPartition->CreateEditorLoaderAdapter<FLoaderAdapterShape>(WorldContextObject->GetWorld(), SelectionBox, TEXT("Loaded Region"));
		UWorldPartitionLoaderWrapper* WorldPartitionLoaderWrapper = NewObject<UWorldPartitionLoaderWrapper>();
		WorldPartitionLoaderWrapper->LoaderAdapter = EditorLoaderAdapter;
		EditorLoaderAdapter->GetLoaderAdapter()->SetUserCreated(true);
		EditorLoaderAdapter->GetLoaderAdapter()->Load();

		GEditor->RedrawLevelEditingViewports();
		return WorldPartitionLoaderWrapper;
	}
#endif
	return nullptr;
}

void UUpgradeBlueprintFunctionLibrary::UnloadCell(const UObject* WorldContextObject, UWorldPartitionLoaderWrapper* WorldPartitionEditorLoaderAdapter)
{
#if WITH_EDITOR
	if (!WorldPartitionEditorLoaderAdapter)
	{
		return;
	}

	if (WorldContextObject->GetWorld())
	{
		WorldPartitionEditorLoaderAdapter->LoaderAdapter->GetLoaderAdapter()->Unload();

		TWeakObjectPtr<UWorldPartition> WorldPartition = WorldContextObject->GetWorld()->GetWorldPartition();
		WorldPartition->ReleaseEditorLoaderAdapter(WorldPartitionEditorLoaderAdapter->LoaderAdapter);
	}
#endif
}

void UUpgradeBlueprintFunctionLibrary::RegisterActorInWorldPartition(AActor* Actor)
{

}

void UUpgradeBlueprintFunctionLibrary::InitializeWorldPartition(const UObject* WorldContextObject)
{
#if WITH_EDITOR
	UWorld* OwningWorld = WorldContextObject->GetWorld();

	const ULevelStreaming* LevelStreaming = FLevelUtils::FindStreamingLevel(OwningWorld->GetLevel(0));

	if (WorldContextObject->GetWorld()->HasSubsystem<UWorldPartitionSubsystem>())
	{
		if (UWorldPartition* WorldPartition = WorldContextObject->GetWorld()->GetWorldPartition())
		{
			//
			// When do we need to initialize the associated world partition object?
			//
			//	- When the level is the main world persistent level
			//	- When the sublevel is streamed in the editor (mainly for data layers)
			//	- When the sublevel is streamed in game and the main world is not partitioned
			//
			const bool bIsOwningWorldGameWorld = WorldContextObject->GetWorld()->IsGameWorld();
			const bool bIsOwningWorldPartitioned = OwningWorld->IsPartitionedWorld();
			const bool bIsMainWorldLevel = OwningWorld->PersistentLevel == OwningWorld->GetLevel(0);
			const bool bInitializeForEditor = !bIsOwningWorldGameWorld;
			const bool bInitializeForGame = bIsOwningWorldGameWorld;

			if (bIsMainWorldLevel || bInitializeForEditor)
			{
				FTransform Transform = LevelStreaming ? LevelStreaming->LevelTransform : FTransform::Identity;
				WorldPartition->Initialize(OwningWorld, Transform);
			}
		}
	}
#endif
}

void UUpgradeBlueprintFunctionLibrary::DeinitializeWorldPartition(const UObject* WorldContextObject)
{
#if WITH_EDITOR
	if (WorldContextObject->GetWorld()->HasSubsystem<UWorldPartitionSubsystem>())
	{
		if (UWorldPartition* WorldPartition = WorldContextObject->GetWorld()->GetWorldPartition(); WorldPartition && WorldPartition->IsInitialized())
		{
			WorldPartition->Uninitialize();
		}
	}
#endif
}

void UUpgradeBlueprintFunctionLibrary::ReInitWorld(const UObject* WorldContextObject)
{
#if WITH_EDITOR
	if (WorldContextObject->GetWorld())
	{
		WorldContextObject->GetWorld()->ReInitWorld();
	}
#endif
}

void UUpgradeBlueprintFunctionLibrary::ReBuildWorld(const UObject* WorldContextObject)
{
}

void UUpgradeBlueprintFunctionLibrary::ForceGarbageCollection()
{
	CollectGarbage(RF_NoFlags);
}

void UUpgradeBlueprintFunctionLibrary::SaveCurrentLevelProxy()
{
#if WITH_EDITOR
	FEditorFileUtils::SaveCurrentLevel();
#endif
}

void UUpgradeBlueprintFunctionLibrary::ClearAllWorldPartitionUnloadActors()
{

}

bool UUpgradeBlueprintFunctionLibrary::CheckIsWorldPartitionEnable()
{
#if WITH_EDITOR
	if (GEditor)
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (World && World->PersistentLevel)
		{
			return World->IsPartitionedWorld();
		}
	}
#endif

	return false;
};

void AEditorTrackingActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	StartConstructionTimer();
}

void AEditorTrackingActor::Destroyed()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TimerHandle);
	EditorDestroyed();
	Super::Destroyed();
}

void AEditorTrackingActor::StartConstructionTimer()
{
	if (TimerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TimerHandle);
	}
	else
	{
		OnConstructionStart();
	}

	TWeakObjectPtr<AEditorTrackingActor> WeakThis = this; // Безопасная ссылка

	TimerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis](float DeltaTime)
			{
				if (WeakThis.IsValid()) // Проверяем перед разыменованием
				{
					AEditorTrackingActor* StrongThis = WeakThis.Get();
					if (StrongThis->GetOuter() && !StrongThis->GetOuter()->HasAnyFlags(RF_BeginDestroyed) && StrongThis->GetWorld())
					{
						if (UFunction* Result = StrongThis->FindFunction("OnConstructionFinished"))
						{
							StrongThis->OnConstructionFinished();
						}
						if (StrongThis->TimerHandle.IsValid())
						{
							StrongThis->TimerHandle.Reset();
						}
					}
				}
				return false;
			}),
		DelayTime
	);
}



void UEditorTimerObjectTrack::Initialization(float Time)
{
	if (TimerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TimerHandle);
	}

	TimerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaTime)
			{
				EndInitialization();
				return false;
			}),
		Time
	);
}

void UEditorTimerObjectTrack::EndInitialization()
{
	EditorTimerObjectTrackDelegate.Broadcast();
}
