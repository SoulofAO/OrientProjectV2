#include "SimpleLandscape.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "ImageUtils.h" 
#include "DynamicMeshBuilder.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"
#include "TextureResource.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "RenderingThread.h"
#include "Engine/Level.h"
#include "AI/NavigationSystemBase.h"

#if WITH_EDITOR
#include "Subsystems/EditorAssetSubsystem.h"
#include "LevelEditor.h"
#include "Editor.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "PackageTools.h"
#endif
#include "Engine/Texture2D.h"
#if WITH_EDITOR
#include "ObjectTools.h"
#endif
#include "Engine/Canvas.h"
#include "UpgradeBlueprintFunctionLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Async/Async.h"
#include "Engine/DataAsset.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Canvas.h"
#include "WorldPartition/WorldPartition.h"
#include "UObject/SavePackage.h"

ASimpleLandscapePart::ASimpleLandscapePart()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
    ProceduralMeshComponent->SetupAttachment(RootComponent);
    ProceduralMeshComponent->bUseComplexAsSimpleCollision = true;
    FCollisionResponseContainer NewReponses;
    NewReponses.SetAllChannels(ECR_Block);
    ProceduralMeshComponent->SetCollisionResponseToChannels(NewReponses);

    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
    StaticMeshComponent->SetupAttachment(RootComponent);
}


UTexture2D* ASimpleLandscapePart::GetLastCachTextureByLayer(TSharedPtr<FSimpleLayer> Layer)
{
    for (int Index = SimpleLandscape->LandscapeBlueprintBrushes.Num()-1; Index>=0; Index --)
    {
        if (!CacheLayerTextures.Contains(SimpleLandscape->LandscapeBlueprintBrushes[Index]))
        {
            continue;
        }
        if (CacheLayerTextures.Find(SimpleLandscape->LandscapeBlueprintBrushes[Index])->TextureByNameMap.Contains(Layer->Name))
        {
            return *CacheLayerTextures.Find(SimpleLandscape->LandscapeBlueprintBrushes[Index])->TextureByNameMap.Find(Layer->Name);
        }
    }
    return nullptr;
}

UTexture2D* ASimpleLandscapePart::GetLastCachTextureByLayerAndBrush(TSharedPtr<FSimpleLayer> Layer, const FBlueprintBrushStruct& BlueprintBrushStruct)
{
    for (int Index = SimpleLandscape->LandscapeBlueprintBrushes.Find(BlueprintBrushStruct) - 1; Index >= 0; Index--)
    {
        if (CacheLayerTextures.Contains(SimpleLandscape->LandscapeBlueprintBrushes[Index]))
        {
            if (CacheLayerTextures.Find(SimpleLandscape->LandscapeBlueprintBrushes[Index])->TextureByNameMap.Contains(Layer->Name))
            {
                return *CacheLayerTextures.Find(SimpleLandscape->LandscapeBlueprintBrushes[Index])->TextureByNameMap.Find(Layer->Name);
            }
        }
    }
    return nullptr;
}

void ASimpleLandscapePart::GetAllInfoByProceduralComponent(TArray<FVector>& Vertices, TArray<uint32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs)
{
    Vertices.Empty();
    Triangles.Empty();
    Normals.Empty();
    UVs.Empty();

    FProcMeshSection* Section = ProceduralMeshComponent->GetProcMeshSection(0);
    if (Section)
    {
        Triangles = Section->ProcIndexBuffer;

        for (FProcMeshVertex& ProcMeshVertex : Section->ProcVertexBuffer)
        {
            Vertices.Add(ProcMeshVertex.Position);
            Normals.Add(ProcMeshVertex.Normal);
            UVs.Add(ProcMeshVertex.UV0);
        }
    }
}

void ASimpleLandscapePart::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if WITH_EDITOR
    if (!SimpleLandscape.IsValid())
    {
        return;
    }

    FVector SpawnPosition = FVector(GridPosition.X * PartSize + SimpleLandscape->GetActorLocation().X, GridPosition.Y * PartSize + SimpleLandscape->GetActorLocation().Y, SimpleLandscape->GetActorLocation().Z);
    SetActorLocation(SpawnPosition);
    SetActorRotation(FRotator(0, 0, 0));
    if (IsValid(ProceduralMeshComponent))
    {
        ProceduralMeshComponent->SetWorldLocation(GetActorLocation());
    }
    if (IsValid(StaticMeshComponent))
    {
        StaticMeshComponent->SetWorldLocation(GetActorLocation());
    }
#endif
}


void ASimpleLandscapePart::ApplyHeightmap()
{
    TArray<FVector> Vertices;
    TArray<uint32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;

    GetAllInfoByProceduralComponent(Vertices, Triangles, Normals, UVs);

    if (!ProceduralMeshComponent)
    {
        return;
    }

    if (!SimpleLandscape)
    {
        UE_LOG(LogTemp, Warning, TEXT("SimpleLandscape is null!"));
        return;
    }

    if (!SimpleLandscape->GetHeightmapLayer())
    {
        UE_LOG(LogTemp, Warning, TEXT("HeightmapLayer is null!"));
        return;
    }

    const int GridResolution = PartSize / GridStep + 1;

    if (IsValid(HeightmapTexture))
    {
        FTexturePlatformData* TextureData = HeightmapTexture->GetPlatformData();
        const int32 TextureWidth = TextureData->SizeX;
        const int32 TextureHeight = TextureData->SizeY;

        if (TextureWidth != GridResolution || TextureHeight != GridResolution)
        {
            UE_LOG(LogTemp, Warning, TEXT("Heightmap dimensions do not match PartSize!"));
            return;
        }

        if (Vertices.Num() != GridResolution * GridResolution)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed with Vertices"));
            return;
        }

        if (!TextureData || TextureData->Mips.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("TextureData is invalid or has no Mips!"));
            return;
        }

        if (!HeightmapTexture || !HeightmapTexture->GetResource())
        {
            UE_LOG(LogTemp, Error, TEXT("HeightmapTexture is null or has no resource!"));
            return;
        }


        FColor* MipData = static_cast<FColor*>(TextureData->Mips[0].BulkData.Lock(LOCK_READ_ONLY));
        if (!MipData)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to lock Heightmap texture data!"));
            return;
        }


        for (int32 X = 0; X < GridResolution; ++X)
        {
            for (int32 Y = 0; Y < GridResolution; ++Y)
            {
                const FColor PixelColor = MipData[Y * TextureWidth + X];

                const float Height = (PixelColor.R - 128.0) * 100.0f + ((PixelColor.G - 128.0) / 128.0) * 100.0f;

                const int32 VertexIndex = X + Y * GridResolution;
                Vertices[VertexIndex].Z = Height;
            }
        }

        TextureData->Mips[0].BulkData.Unlock();

        ProceduralMeshComponent->UpdateMeshSection(0, Vertices, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>());
        ProceduralMeshComponent->ClearCollisionConvexMeshes();
        FNavigationSystem::UpdateComponentData(*ProceduralMeshComponent);
    }
    else
    {
        for (int32 X = 0; X < GridResolution; ++X)
        {
            for (int32 Y = 0; Y < GridResolution; ++Y)
            {
                const int32 VertexIndex = X + Y * GridResolution;
                Vertices[VertexIndex].Z = 0.0;
            }
        }

        ProceduralMeshComponent->UpdateMeshSection(0, Vertices, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>());
        ProceduralMeshComponent->ClearCollisionConvexMeshes();
        FNavigationSystem::UpdateComponentData(*ProceduralMeshComponent);
    }
}

void ASimpleLandscapePart::UpdateMaterials()
{
    if (SimpleLandscape->DebugLandscapeMode == EDebugLandscapeMode::None)
    {
        ProceduralMeshComponent->SetMaterial(0, DynamicMaterial);
    }
    else
    {
        uint8 Index = static_cast<uint8>(SimpleLandscape->DebugLandscapeMode);

        ProceduralMeshComponent->SetMaterial(0, DebugLandscapeMaterial);
        ProceduralMeshComponent->SetScalarParameterValueOnMaterials("DebugIndex", Index);
    }
}

float ASimpleLandscapePart::GetHeightByTextureCoord(FVector2D TextureCoord)
{
    TArray<FVector> Vertices;
    TArray<uint32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;

    GetAllInfoByProceduralComponent(Vertices, Triangles, Normals, UVs);

    if (!ProceduralMeshComponent)
    {
        return -1.0;
    }

    if (!SimpleLandscape)
    {
        UE_LOG(LogTemp, Warning, TEXT("SimpleLandscape is null!"));
        return -1.0;
    }
    if (Vertices.IsValidIndex(FMath::RoundToInt(TextureCoord.Y * PartSize / GridStep)) && Vertices.IsValidIndex(FMath::RoundToInt(TextureCoord.X)))
    {
        const float Height = Vertices[FMath::RoundToInt(TextureCoord.Y * PartSize / GridStep)].Z + Vertices[FMath::RoundToInt(TextureCoord.X)].Z;
        return Height;
    }
    return -1.0;
}

void ASimpleLandscapePart::SetupHeightmap(TPair<TSharedPtr<FSimpleLayer>, UTexture2D*> NewHeightmapTexture)
{
    HeightmapTexture = NewHeightmapTexture.Value;
    ApplyHeightmap();
}

void ASimpleLandscapePart::ClearHeightmap()
{
    TArray<FVector> Vertices;
    TArray<uint32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;

    GetAllInfoByProceduralComponent(Vertices, Triangles, Normals, UVs);

    if (!ProceduralMeshComponent)
    {
        return;
    }

    HeightmapTexture = nullptr;

    const int32 GridResolution = PartSize / GridStep;

    for (int32 X = 0; X <= GridResolution; ++X)
    {
        for (int32 Y = 0; Y <= GridResolution; ++Y)
        {
            const float Height = 0.0;
            const int32 VertexIndex = X + Y * (GridResolution + 1);
            Vertices[VertexIndex].Z = Height;
        }
    }

    ProceduralMeshComponent->UpdateMeshSection(0, Vertices, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>());
    /*ProceduralMeshComponent->bUseComplexAsSimpleCollision = true;
    FCollisionResponseContainer NewReponses;
    NewReponses.SetAllChannels(ECR_Block);
    ProceduralMeshComponent->SetCollisionResponseToChannels(NewReponses);
    ProceduralMeshComponent->UpdateCollisionProfile();*/
    ProceduralMeshComponent->ClearCollisionConvexMeshes();
}

void ASimpleLandscapePart::ApplyWeightmap(TPair<TSharedPtr<FSimpleLayer>, UTexture2D*> NewWeightmapTexture)
{
    WeightmapTextureByName.Add(NewWeightmapTexture.Key->Name, NewWeightmapTexture.Value);

    FMaterialParameterInfo ParamInfo;
    ParamInfo.Name = TEXT("Texture");
    ParamInfo.Association = EMaterialParameterAssociation::BlendParameter; // Задаем область действия
    ParamInfo.Index = NewWeightmapTexture.Key->MaterialIndexForWeightmap;
    
    DynamicMaterial->SetTextureParameterValueByInfo(ParamInfo, NewWeightmapTexture.Value);

}

void ASimpleLandscapePart::ClearWeightmaps()
{
    for (TPair<FName, UTexture2D*> Pair : WeightmapTextureByName)
    {
        TSharedPtr<FSimpleLayer> SimpleLayer = SimpleLandscape->GetWeightmapLayerByName(Pair.Key);
        if (SimpleLayer)
        {
            FMaterialParameterInfo ParamInfo;
            ParamInfo.Name = TEXT("Texture");
            ParamInfo.Association = EMaterialParameterAssociation::BlendParameter; // Задаем область действия
            ParamInfo.Index = SimpleLayer->MaterialIndexForWeightmap;

            DynamicMaterial->SetTextureParameterValueByInfo(ParamInfo, SimpleLandscape->BlackTexture);
        }
    }
    WeightmapTextureByName.Empty();
}



FVector ASimpleLandscapePart::GetWorldCoordinate(const FVector2D& LocalCoordinate, const FVector& PartOrigin) const
{
    return FVector(
        PartOrigin.X + LocalCoordinate.X * GridStep,
        PartOrigin.Y + LocalCoordinate.Y * GridStep,
        PartOrigin.Z 
    );
}


FVector2D ASimpleLandscapePart::GetGlobalCoordinate(const FVector& WorldCoordinate, const FVector& PartOrigin) const
{
    return FVector2D(
        (WorldCoordinate.X - PartOrigin.X),
        (WorldCoordinate.Y - PartOrigin.Y)
    );
}


void ASimpleLandscapePart::Initialization()
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;

    Vertices.Empty();
    Triangles.Empty();
    Normals.Empty();
    UVs.Empty();

    const int32 Size = PartSize / GridStep;

    for (int32 Y = 0; Y <= Size; ++Y)
    {
        for (int32 X = 0; X <= Size; ++X)
        {
            Vertices.Add(FVector(X * GridStep, Y * GridStep, 0));

            // UVs
            UVs.Add(FVector2D((float)X / Size, (float)Y / Size));

            if (X < Size && Y < Size)
            {
                int32 StartIndex = X + Y * (Size + 1);
                Triangles.Add(StartIndex);
                Triangles.Add(StartIndex + Size + 1);
                Triangles.Add(StartIndex + 1);

                Triangles.Add(StartIndex + 1);
                Triangles.Add(StartIndex + Size + 1);
                Triangles.Add(StartIndex + Size + 2);
            }
        }
    }

    Normals.Init(FVector(0, 0, 1), Vertices.Num());

    ProceduralMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
    FNavigationSystem::UpdateComponentData(*ProceduralMeshComponent);

#if WITH_EDITOR
    ProceduralMeshComponent->UpdateCollisionProfile();
#endif
    DynamicMaterial = UMaterialInstanceDynamic::Create(LandscapeMaterial, this);
    DebugLandscapeMaterial = UMaterialInstanceDynamic::Create(DebugLandscapeMaterial, this);
    UpdateMaterials();
}


FVector2D ASimpleLandscapePart::GetTextureSize()
{
    return FVector2D(PartSize / GridStep + 1, PartSize / GridStep + 1);
}

#if WITH_EDITOR
void ASimpleLandscape::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (GetWorld()->WorldType == EWorldType::Editor || GetWorld()->WorldType == EWorldType::EditorPreview)
    {
        RequestUpdateSettings();
    }
}
#endif


#if WITH_EDITOR
void ASimpleLandscape::ImportBrushFromArrayData()
{
    if (IsValid(ImportBrushArrayData))
    {
        LandscapeBlueprintBrushes.Empty();
        for (FBlueprintBrushStruct BlueprintBrushStruct : ImportBrushArrayData->LandscapeBlueprintBrushes)
        {
            TArray<USimpleLandscapeBlueprintBrush*> SimpleLandscapeBlueprintBrushes;
            UUpgradeBlueprintFunctionLibrary::CopyObjects(SimpleLandscapeBlueprintBrushes, BlueprintBrushStruct.Brushes, this);
            FBlueprintBrushStruct NewBlueprintBrushStruct;
            NewBlueprintBrushStruct.Brushes = SimpleLandscapeBlueprintBrushes;
            LandscapeBlueprintBrushes.Add(NewBlueprintBrushStruct);
        }
        RequestUpdateSettings();
    }
}

void ASimpleLandscape::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    FName PropertyName = PropertyChangedEvent.GetPropertyName();

    if (PropertyName == GET_MEMBER_NAME_CHECKED(ASimpleLandscape, DebugLandscapeMode))
    {
        for (TSoftObjectPtr<ASimpleLandscapePart> LandscapePart : LandscapeParts)
        {
            LandscapePart->UpdateMaterials();
        }
    }
}
#endif

TSharedPtr<FSimpleLayer> ASimpleLandscape::FindLayerByName(FName Name)
{
    for (TSharedPtr<FSimpleLayer> SimpleLayer : Layers)
    {
        if (SimpleLayer->Name == Name)
        {
            return SimpleLayer;
        }
    }
    return nullptr;
}

TSharedPtr<FSimpleLayer> ASimpleLandscape::GetHeightmapLayer()
{
    for (TSharedPtr<FSimpleLayer> SimpleLayer : Layers)
    {
        if (SimpleLayer->SimpleLayerType == ESimpleLayerType::Heightmap)
        {
            return SimpleLayer;
        }
    }
    return nullptr;
}

TSharedPtr<FSimpleLayer> ASimpleLandscape::GetWeightmapLayerByName(FName Name)
{
    for (TSharedPtr<FSimpleLayer> SimpleLayer : Layers)
    {
        if (SimpleLayer->SimpleLayerType == ESimpleLayerType::Weightmap && SimpleLayer->Name == Name)
        {
            return SimpleLayer;
        }
    }
    return nullptr;
}

TArray<TSharedPtr<FSimpleLayer>> ASimpleLandscape::GetWeightmapLayers()
{
    TArray<TSharedPtr<FSimpleLayer>> AnswerLayers;

    for (TSharedPtr<FSimpleLayer> SimpleLayer : Layers)
    {
        if (SimpleLayer->SimpleLayerType == ESimpleLayerType::Weightmap)
        {
            AnswerLayers.Add(SimpleLayer);
        }
    }
    return AnswerLayers;
}

void ASimpleLandscape::GenerateLandscapeParts()
{
    if (UseBakeStaticMesh)
    {
        return;
    }

    if (LandscapeParts.Num() == PartCountY * PartCountX)
    {
        return;
    }

    ClearAllLandscapePart();

    UWorld* World = GetWorld();

    for (int32 X = 0; X < PartCountX; X++)
    {
        for (int32 Y = 0; Y < PartCountY; Y++)
        {
            FVector SpawnPosition = FVector(X * PartSize + GetActorLocation().X, Y * PartSize + GetActorLocation().Y, GetActorLocation().Z);
    
            TSoftObjectPtr<ASimpleLandscapePart> NewPart = World->SpawnActor<ASimpleLandscapePart>(ASimpleLandscapePart::StaticClass(), FTransform(FRotator(0,0,0), SpawnPosition, FVector(1, 1, 1)));
            NewPart->PartSize = PartSize;
            NewPart->GridStep = GridStep;
            //NewPart->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

            NewPart->LandscapeMaterial = LandscapeMaterial;
            NewPart->DebugLandscapeMaterial = DebugLandscapeMaterial;
            NewPart->GridPosition = FVector2D(X, Y);
            NewPart->SimpleLandscape = this;

            LandscapeParts.Add(NewPart);
            NewPart->Initialization();

#if WITH_EDITOR
            NewPart->SetIsSpatiallyLoaded(true);
#endif

            if (LandscapeParts.Num() > WorldPartitionLoadGroupSize)
            {
                FVector2D MinRange;
                FVector2D MaxRange;
                GetGridBounds(LandscapeParts, MinRange, MaxRange);
            }

        }
    }
}

TSoftObjectPtr<ASimpleLandscapePart> ASimpleLandscape::GetLandscapePartByCoordIndex(int32 X, int32 Y) const
{
    if (X < 0 || X >= PartCountX || Y < 0 || Y >= PartCountY)
    {
        return nullptr;
    }

    int32 Index = Y * PartCountX + X;
    return LandscapeParts.IsValidIndex(Index) ? LandscapeParts[Index].Get() : nullptr;
}


bool ASimpleLandscape::GetLandscapePartAndGlobalCoordinate(const FVector& WorldCoordinate, TSoftObjectPtr<ASimpleLandscapePart>& OutPart, FVector2D& OutGlobalCoordinate) const
{
    FVector2D GlobalCoordinate;
    TransformWorldVectorToGlobalVector(WorldCoordinate, GlobalCoordinate);

    TArray<TSoftObjectPtr<ASimpleLandscapePart>> LandscapePartsInRange =  FindLandscapePartsInGlobalRange(GlobalCoordinate, GlobalCoordinate);
    if (LandscapePartsInRange.IsValidIndex(0))
    {
        OutPart = LandscapePartsInRange[0];
        TransformWorldVectorToGlobalVector(WorldCoordinate, OutGlobalCoordinate);
        return true;
    }

    OutPart = nullptr;
    OutGlobalCoordinate = FVector2D::ZeroVector;
    return false;
}

bool ASimpleLandscape::TransformWorldVectorToGlobalVector(FVector WorldVector, FVector2D& GlobalVector) const
{
    GlobalVector = FVector2D(WorldVector.X - GetActorLocation().X, WorldVector.Y - GetActorLocation().Y);
    if (GlobalVector.X > 0 && GlobalVector.X < PartSize*PartCountX && GlobalVector.Y > 0 && GlobalVector.Y < PartSize *PartCountY)
    {
        return true;
    }
    return false;
}

void ASimpleLandscape::UpdateSettings()
{
    if (UpdateSettingsTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(UpdateSettingsTickerHandle);
        UpdateSettingsTickerHandle.Reset();
    }

    SetupLayers();

    GenerateLandscapeParts();

    for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : LandscapeParts)
    {
        if (!SimpleLandscapePart.IsValid())
        {
            continue;
        }

        SimpleLandscapePart->PartSize = PartSize;
        SimpleLandscapePart->GridStep = GridStep;
        //SimpleLandscapePart->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

        SimpleLandscapePart->LandscapeMaterial = LandscapeMaterial;
        SimpleLandscapePart->SimpleLandscape = this;

        if (SimpleLandscapePart->ProceduralMeshComponent->GetProcMeshSection(0)->ProcVertexBuffer.Num() != (PartSize / GridStep + 1) * (PartSize / GridStep + 1) && !UseBakeStaticMesh)
        {
            SimpleLandscapePart->Initialization();
        }
    }

    int MinIndexWithNoValidBrush = -1;

    int Count = 0;
    for (FBlueprintBrushStruct BlueprintBrushStruct : LandscapeBlueprintBrushes)
    {
        for (USimpleLandscapeBlueprintBrush* SimpleLandscapeBlueprintBrush : BlueprintBrushStruct.Brushes)
        {
            if (!SimpleLandscapeBlueprintBrush)
            {
                if (MinIndexWithNoValidBrush == -1)
                {
                    MinIndexWithNoValidBrush = Count;
                }
                continue;
            }
            SimpleLandscapeBlueprintBrush->SimpleLandscape = this;
        }
        Count++;
    }

    TArray<FBlueprintBrushStruct> ValidBlueprintBrushStructs;

    if (MinIndexWithNoValidBrush != -1)
    {
        for (int Index = 0; Index < MinIndexWithNoValidBrush; Index++)
        {
            ValidBlueprintBrushStructs.Add(LandscapeBlueprintBrushes[Index]);
        }
    }
    else
    {
        ValidBlueprintBrushStructs = LandscapeBlueprintBrushes;
    }

    for (TSoftObjectPtr<ASimpleLandscapePart>  SimpleLandscapePart : LandscapeParts)
    {
        if (!SimpleLandscapePart.IsValid())
        {
            continue;
        }
        TArray<FBlueprintBrushStruct> KeysToRemove;

        for (TPair<FBlueprintBrushStruct, FTextureByNameMapStruct> Pair : SimpleLandscapePart->CacheLayerTextures)
        {
            if (!ValidBlueprintBrushStructs.Contains(Pair.Key))
            {
                KeysToRemove.Add(Pair.Key);
            }
        }

        for (FBlueprintBrushStruct Key : KeysToRemove)
        {
            for (TPair<FName, UTexture2D*> Pair : SimpleLandscapePart->CacheLayerTextures.Find(Key)->TextureByNameMap)
            {
                if (Pair.Value)
                {
                    //const TArray<UObject*> InObjectsToDelete = { Pair.Value };
                    //ObjectTools::ForceDeleteObjects(InObjectsToDelete, false);
                    FAssetRegistryModule::AssetDeleted(Pair.Value);
                }
            }
            SimpleLandscapePart->CacheLayerTextures.Remove(Key);
        }
    }

    FString ResultPath = FString(GetWorld()->GetCurrentLevel()->GetOutermost()->GetPathName() + FString("Textures/BlackTexture"));

    BlackTexture = LoadObject<UTexture2D>(nullptr, *ResultPath);

#if WITH_EDITOR
    if (!BlackTexture)
    {
        UTextureRenderTarget2D* TextureRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, 32, 32);
        TextureRenderTarget->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;

        TextureRenderTarget->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;

        TextureRenderTarget->SRGB = false;
        TextureRenderTarget->LODGroup = TEXTUREGROUP_Pixels2D;
        UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(TextureRenderTarget, ResultPath);
        FlushRenderingCommands();
        BlackTexture = LoadObject<UTexture2D>(nullptr, *ResultPath);

        UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
        EditorAssetSubsystem->SaveAsset(BlackTexture->GetPathName(), false);
    }
#endif

    DeleteAllNonValidCach();

    if (UpdateLandscapeChange)
    {
        ReciveCachOrRebuildFromLastCach(FVector2D(0, 0), FVector2D(PartCountX * PartSize, PartCountY * PartSize));
    }
}

bool ASimpleLandscape::CheckLandscapeInRange(FBox Box)
{
    for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : LandscapeParts)
    {
        FVector HitLocation;
        FVector HitNormal; 
        FName BoneName; 
        FHitResult OutHit;

        if (LandscapeParts[0]->ProceduralMeshComponent->K2_BoxOverlapComponent((Box.Min + Box.Max) / 2, Box, true, false, false, HitLocation, HitNormal, BoneName, OutHit))
        {
            return true;
        }
    }

    return false;
}

float ASimpleLandscape::GetHeightByTextureCoord(FVector2D TextureCoord)
{
    TArray<TSoftObjectPtr<ASimpleLandscapePart>> SimpleLandscapes = FindLandscapePartsInTextureRange(TextureCoord, TextureCoord);
    if (SimpleLandscapes.IsValidIndex(0))
    {
        return SimpleLandscapes[0]->GetHeightByTextureCoord(TextureCoord - SimpleLandscapes[0]->GridPosition * PartSize / GridStep);
    }
    return -1.0;
}

float ASimpleLandscape::GetHeightByGlobalCoord(FVector2D GlobalCoord)
{
    FVector2D TextureCoord;
    TransformGlobalVectorToTextureVector(GlobalCoord, TextureCoord);
    return GetHeightByTextureCoord(TextureCoord);
}

bool ASimpleLandscape::ProjectWorldPoint(FVector WorldCoord, FVector& ProjectWorldCoord)
{
    FVector2D GlobalCoord;
    bool Sucsess = TransformWorldVectorToGlobalVector(WorldCoord, GlobalCoord);
    WorldCoord.Z = GetHeightByGlobalCoord(GlobalCoord) + GetActorLocation().Z;
    ProjectWorldCoord = WorldCoord;
    return Sucsess;
}

void ASimpleLandscape::UpdateBrushSettings(FBlueprintBrushStruct BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange)
{
    if (!SetupUpdateBlueprintBrushStruct)
    {
        UpdateMinRange = MinRange;
        UpdateMaxRange = MaxRange;
    }
    else
    {
        bool bRangeExpanded = MinRange.X < UpdateMinRange.X || MinRange.Y < UpdateMinRange.Y ||
            MaxRange.X > UpdateMaxRange.X || MaxRange.Y > UpdateMaxRange.Y;
        if (bRangeExpanded)
        {
            UpdateMinRange = FVector2D(FMath::Min(TArray<double>{MinRange.X, UpdateMinRange.X}), FMath::Min(TArray<double>{MinRange.Y, UpdateMinRange.Y}));
            UpdateMaxRange = FVector2D(FMath::Max(TArray<double>{MaxRange.X, UpdateMaxRange.X}), FMath::Max(TArray<double>{MaxRange.Y, UpdateMaxRange.Y}));
        }
    }

    if (!SetupUpdateBlueprintBrushStruct || LandscapeBlueprintBrushes.Contains(BlueprintBrushStruct) && 
        LandscapeBlueprintBrushes.Contains(UpdateBlueprintBrushStruct) && 
        LandscapeBlueprintBrushes.Find(BlueprintBrushStruct) <= LandscapeBlueprintBrushes.Find(UpdateBlueprintBrushStruct))
    {
        UpdateBlueprintBrushStruct = BlueprintBrushStruct;
        SetupUpdateBlueprintBrushStruct = true;
    }

    if (UpdateBrushSettingsTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(UpdateBrushSettingsTickerHandle);
        UpdateBrushSettingsTickerHandle.Reset();
    }

    UpdateBrushSettingsTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this,
        [this, MinRange, MaxRange](float DeltaTime)
        {
            if (!SetupUpdateBlueprintBrushStruct)
            {
                return false;
            }
            if (EnableAsync)
            {
                FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, MinRange, MaxRange]()
                    {
                        RebuildBrush(UpdateBlueprintBrushStruct, UpdateMinRange, UpdateMaxRange);
                    }, TStatId(), NULL, ENamedThreads::GameThread);
            }
            else
            {
                RebuildBrush(UpdateBlueprintBrushStruct, UpdateMinRange, UpdateMaxRange);
            }
            UpdateBrushSettingsTickerHandle.Reset();
            SetupUpdateBlueprintBrushStruct = false;
            return false;
        }), 0.2f);
}


bool ASimpleLandscape::TransformGlobalVectorToWorldVector(FVector2D GlobalVector, FVector& WorldVector) const
{
    WorldVector = FVector(GlobalVector.X + GetActorLocation().X, GlobalVector.Y + GetActorLocation().Y, GetActorLocation().Z);
    return true;
}

bool ASimpleLandscape::TransformGlobalVectorToTextureVector(FVector2D GlobalVector, FVector2D& TextureVector) const
{
    TextureVector = GlobalVector / GridStep;
    if (GlobalVector.X > 0 && GlobalVector.X < PartSize * PartCountX && GlobalVector.Y > 0 && GlobalVector.Y < PartSize * PartCountY)
    {
        return true;
    }
    return false;
}

bool ASimpleLandscape::TransformTextureVectorToGlobalVector(FVector2D TextureVector, FVector2D& GlobalVector) const
{
    GlobalVector = TextureVector*GridStep;

    if (GlobalVector.X > 0 && GlobalVector.X < PartSize * PartCountX && GlobalVector.Y > 0 && GlobalVector.Y < PartSize * PartCountY)
    {
        return true;
    }
    return false;
}

bool ASimpleLandscape::FindBlueprintBrushStructByBlueprintBrushObject(int& Index, FBlueprintBrushStruct& BlueprintBrushStruct, USimpleLandscapeBlueprintBrush* SimpleLandscapeBlueprintBrush)
{
    Index = 0;
    for (FBlueprintBrushStruct LandscapeBlueprintBrush : LandscapeBlueprintBrushes)
    {
        if (LandscapeBlueprintBrush.Brushes.Contains(SimpleLandscapeBlueprintBrush))
        {
            BlueprintBrushStruct = LandscapeBlueprintBrush;
            Index = Index + 1;
            return true;
        }
    }
    return false;
}

TArray<TSoftObjectPtr<ASimpleLandscapePart>> ASimpleLandscape::FindLandscapePartsInGlobalRange(const FVector2D& MinRange, const FVector2D& MaxRange) const
{
    TArray<TSoftObjectPtr<ASimpleLandscapePart>> FoundParts;
    FBox2D BaseBox = FBox2D(MinRange, MaxRange);

    for (TSoftObjectPtr<ASimpleLandscapePart>  Part : LandscapeParts)
    {
        if (Part)
        {
            FVector2D MinPartPosition = Part->GridPosition * PartSize;
            FVector2D MaxPartPosition = Part->GridPosition * PartSize + FVector2D(PartSize, PartSize);
            FBox2D PartBox = FBox2D(MinPartPosition, MaxPartPosition);

            if (PartBox.Intersect(BaseBox) || PartBox.IsInside(BaseBox) || BaseBox.IsInside(PartBox))
            {
                FoundParts.Add(Part.Get());
            }
        }
    }

    return FoundParts;
}

TArray<TSoftObjectPtr<ASimpleLandscapePart>> ASimpleLandscape::GetNearestLandscapePartsByParts(const TArray <TSoftObjectPtr<ASimpleLandscapePart>>& InputLandscapeParts) const
{
    TArray<TSoftObjectPtr<ASimpleLandscapePart>> AnswerNearestLandscapeParts = InputLandscapeParts;
    for (TSoftObjectPtr<ASimpleLandscapePart> InputLandscapePart : InputLandscapeParts)
    {
        TArray<TSoftObjectPtr<ASimpleLandscapePart>> NearestLandscapeParts = GetNearestLandscapePartsByPart(InputLandscapePart);
        for (TSoftObjectPtr<ASimpleLandscapePart> NearLandscapePart : NearestLandscapeParts)
        {
            if (!AnswerNearestLandscapeParts.Contains(NearLandscapePart))
            {
                AnswerNearestLandscapeParts.Add(NearLandscapePart);
            }
        }
    }
    return AnswerNearestLandscapeParts;
}

TArray<TSoftObjectPtr<ASimpleLandscapePart>> ASimpleLandscape::GetNearestLandscapePartsByPart(TSoftObjectPtr<ASimpleLandscapePart> InputLandscapePart) const
{
    TArray<TSoftObjectPtr<ASimpleLandscapePart>> Neighbors;

    if (!InputLandscapePart)
    {
        return Neighbors;
    }

    FVector2D GridPos = InputLandscapePart->GridPosition;

    for (int32 OffsetX = -1; OffsetX <= 1; OffsetX++)
    {
        for (int32 OffsetY = -1; OffsetY <= 1; OffsetY++)
        {
            if (OffsetX == 0 && OffsetY == 0)
            {
                continue;
            }

            int32 NeighborX = GridPos.X + OffsetX;
            int32 NeighborY = GridPos.Y + OffsetY;

            TSoftObjectPtr<ASimpleLandscapePart> Neighbor = GetLandscapePartByCoordIndex(NeighborX, NeighborY).Get();
            if (Neighbor)
            {
                Neighbors.Add(Neighbor);
            }
        }
    }

    return Neighbors;
}


TArray<TSoftObjectPtr<ASimpleLandscapePart>> ASimpleLandscape::FindLandscapePartsInTextureRange(const FVector2D& MinRange, const FVector2D& MaxRange) const
{
    TArray<TSoftObjectPtr<ASimpleLandscapePart>> FoundParts;

    for (TSoftObjectPtr<ASimpleLandscapePart> Part : LandscapeParts)
    {
        if (Part)
        {
            FVector2D MinPartPosition = Part->GridPosition * PartSize / GridStep;
            FVector2D MaxPartPosition = Part->GridPosition * PartSize / GridStep + FVector2D(PartSize / GridStep, PartSize / GridStep);

            if (MaxPartPosition.X >= MinRange.X && MinPartPosition.X <= MaxRange.X &&
                MaxPartPosition.Y >= MinRange.Y && MinPartPosition.Y <= MaxRange.Y)
            {
                FoundParts.Add(Part.Get());
            }
        }
    }

    return FoundParts;
}



FVector ASimpleLandscape::GetWorldCoordinateFromPart(TSoftObjectPtr<ASimpleLandscapePart> Part, const FVector2D& LocalCoordinate) const
{
    if (!Part || !LandscapeParts.Contains(Part))
    {
        return FVector::ZeroVector;
    }

    int32 PartIndex = LandscapeParts.IndexOfByKey(Part);
    int32 PartIndexX = PartIndex % PartCountX;
    int32 PartIndexY = PartIndex / PartCountX;

    FVector PartOrigin(PartIndexX * PartSize * GridStep, PartIndexY * PartSize * GridStep, 0);
    return Part->GetWorldCoordinate(LocalCoordinate, PartOrigin);
}

void ASimpleLandscape::RequestRebuildBrush(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange)
{
    if (UpdateBrushChange)
    {
        UpdateBrushSettings(BlueprintBrushStruct, MinRange, MaxRange);
    }
}


void ASimpleLandscape::RequestUpdateSettings()
{
    if (UpdateSettingsTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(UpdateSettingsTickerHandle);
        UpdateSettingsTickerHandle.Reset();
    }

    UpdateSettingsTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
        {
            if (EnableAsync)
            {
                FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this]()
                    {
                        UpdateSettings();
                    }, TStatId(), NULL, ENamedThreads::GameThread);
            }
            else
            {
                UpdateSettings();
            }

            UpdateSettingsTickerHandle.Reset(); 
            return false;
        }), 0.2f);
}


void ASimpleLandscape::RebuildAllBrush()
{
    if(LandscapeBlueprintBrushes.Num()>0)
    {
        RebuildBrush(LandscapeBlueprintBrushes[LandscapeBlueprintBrushes.Num() - 1], FVector2D(0, 0), FVector2D(PartCountX * PartSize, PartCountY * PartSize));
    }
}


TMap<TSharedPtr<FSimpleLayer>, UTextureCombinator*> ASimpleLandscape::GetTextureCombinerByCache(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange)
{
    TMap<TSharedPtr<FSimpleLayer>, UTextureCombinator*> TextureCombiners;

    TArray<TSharedPtr<FSimpleLayer>> SimpleLayers;

    for (USimpleLandscapeBlueprintBrush* Brush : BlueprintBrushStruct.Brushes)
    {
        if (!IsValid(Brush))
        {
            continue;
        }
        if (Brush->AffectHeightmap)
        {
            if (!SimpleLayers.Contains(GetHeightmapLayer()))
            {
                SimpleLayers.Add(GetHeightmapLayer());
            }
        }
        if (Brush->AffectWeightmap)
        {
            for (FName AffectedWeightmapLayerName : Brush->AffectedWeightmapLayers)
            {
                TSharedPtr<FSimpleLayer> WeightmapSimpleLayer = GetWeightmapLayerByName(AffectedWeightmapLayerName);
                if (WeightmapSimpleLayer.IsValid() && !SimpleLayers.Contains(WeightmapSimpleLayer))
                {
                    SimpleLayers.Add(GetWeightmapLayerByName(AffectedWeightmapLayerName));
                }
            }
        }
    }

    TArray<TSoftObjectPtr<ASimpleLandscapePart>> LandscapePartsInRange = FindLandscapePartsInGlobalRange(MinRange - FVector2D(GridStep, GridStep), MaxRange + FVector2D(GridStep, GridStep));

    FVector2D OutMinRange;
    FVector2D OutMaxRange;
    NormalizeGlobalRangeByLandscapeParts(MinRange - FVector2D(GridStep, GridStep), MaxRange + FVector2D(GridStep, GridStep), OutMinRange, OutMaxRange);

    FVector2D TextureOutMinRange;
    FVector2D TextureOutMaxRange;

    TransformGlobalVectorToTextureVector(OutMinRange, TextureOutMinRange);
    TransformGlobalVectorToTextureVector(OutMaxRange, TextureOutMaxRange);

    for (TSharedPtr<FSimpleLayer> SimpleLayer : SimpleLayers)
    {
        UTextureCombinator* TextureCombinator = NewObject<UTextureCombinator>(this, "");

        TextureCombinator->RenderTargetInitialization(nullptr, FVector2D(TextureOutMaxRange.X- TextureOutMinRange.X, TextureOutMaxRange.Y - TextureOutMinRange.Y) + FVector2D(1, 1));
        TextureCombiners.Add(SimpleLayer, TextureCombinator);
    }

    for(TPair<TSharedPtr<FSimpleLayer>, UTextureCombinator*> SimpleLayer : TextureCombiners)
    {
        for (TSoftObjectPtr<ASimpleLandscapePart> LandscapePart : LandscapePartsInRange)
        {
            
            if (LandscapeBlueprintBrushes.Find(BlueprintBrushStruct)!=0)
            {
                UTexture2D* CacheTexture = LandscapePart->GetLastCachTextureByLayerAndBrush(SimpleLayer.Key, BlueprintBrushStruct);
                if (CacheTexture)
                {
                    SimpleLayer.Value->AddTextureCombinator(CacheTexture, FVector2D(PartSize/GridStep, PartSize / GridStep) * LandscapePart->GridPosition, LandscapePart->GetTextureSize(), 0);
                }
                else
                {
                    if (SimpleLayer.Key == GetHeightmapLayer())
                    {
                        FVector2D Size = LandscapePart->GetTextureSize();

                        UTextureRenderTarget2D* TextureRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, Size.X, Size.Y, RTF_RGBA16f, FLinearColor(128.0f / 255.0f, 128.0f / 255.0f, 0.0f, 1.0f));
                        SimpleLayer.Value->AddTextureCombinator(TextureRenderTarget, (Size - FVector2D(1, 1)) * LandscapePart->GridPosition, Size, 0);
                    }
                }
            }
            else
            {
                if (SimpleLayer.Key == GetHeightmapLayer())
                {
                    FVector2D Size = LandscapePart->GetTextureSize();

                    UTextureRenderTarget2D* TextureRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, Size.X, Size.Y, RTF_RGBA16f, FLinearColor(128.0f / 255.0f, 128.0f / 255.0f, 0.0f, 1.0f));
                    SimpleLayer.Value->AddTextureCombinator(TextureRenderTarget, (Size - FVector2D(1, 1)) * LandscapePart->GridPosition, Size, 0);
                }
            }
        }
    }
    return TextureCombiners;
}


void ASimpleLandscape::NormalizeGlobalRangeByLandscapeParts(const FVector2D InMinRange, const FVector2D InMaxRange, FVector2D& OutMinRange, FVector2D& OutMaxRange)
{
    
    OutMinRange = FVector2D(FMath::CeilToInt(InMinRange.X / PartSize) * PartSize , FMath::CeilToInt(InMinRange.Y / PartSize) * PartSize);
    OutMaxRange = FVector2D(FMath::CeilToInt(InMaxRange.X / PartSize) * PartSize, FMath::CeilToInt(InMaxRange.Y / PartSize) * PartSize);

    OutMinRange = FVector2D(FMath::Clamp(OutMinRange.X, 0, PartCountX * PartSize), FMath::Clamp(OutMinRange.Y, 0, PartCountY * PartSize));
    OutMaxRange = FVector2D(FMath::Clamp(OutMaxRange.X, 0, PartCountX * PartSize), FMath::Clamp(OutMaxRange.Y, 0, PartCountY * PartSize));
    /*
    TArray<TSoftObjectPtr<ASimpleLandscapePart>> LandscapePartsInRange = FindLandscapePartsInGlobalRange(InMinRange, InMaxRange);
    FVector2D ResultOutMinRange = FVector2D(1000000000000000, 1000000000000000);
    FVector2D ResultOutMaxRange = FVector2D(-1000000000000000, -1000000000000000);

    for (TSoftObjectPtr<ASimpleLandscapePart> LandscapePart : LandscapePartsInRange)
    {
        FVector2D GridStart = LandscapePart->GridPosition * LandscapePart->PartSize;
        FVector2D GridEnd = LandscapePart->GridPosition * LandscapePart->PartSize + FVector2D(PartSize, PartSize);

        ResultOutMinRange.X = FMath::Min(ResultOutMinRange.X, GridStart.X);
        ResultOutMinRange.Y = FMath::Min(ResultOutMinRange.Y, GridStart.Y);

        ResultOutMaxRange.X = FMath::Max(ResultOutMaxRange.X, GridEnd.X);
        ResultOutMaxRange.Y = FMath::Max(ResultOutMaxRange.Y, GridEnd.Y);
    }

    OutMinRange = ResultOutMinRange;
    OutMaxRange = ResultOutMaxRange;
    */
}

void ASimpleLandscape::GetGridBounds(const TArray< TSoftObjectPtr<ASimpleLandscapePart>>& InputLandscapeParts, FVector2D& OutMin, FVector2D& OutMax) const
{
    if (InputLandscapeParts.Num() == 0)
    {
        OutMin = FVector2D(0, 0);
        OutMax = FVector2D(0, 0);
        return;
    }

    OutMin = FVector2D(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
    OutMax = FVector2D(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());

    for (const TSoftObjectPtr<ASimpleLandscapePart> Part : InputLandscapeParts)
    {
        if (!Part.IsValid())
        {
            continue;
        }

        OutMin.X = FMath::Min(OutMin.X, Part->GridPosition.X);
        OutMin.Y = FMath::Min(OutMin.Y, Part->GridPosition.Y);

        OutMax.X = FMath::Max(OutMax.X, Part->GridPosition.X);
        OutMax.Y = FMath::Max(OutMax.Y, Part->GridPosition.Y);

        OutMin.X = FMath::Min(OutMin.X, Part->GridPosition.X + 1);
        OutMin.Y = FMath::Min(OutMin.Y, Part->GridPosition.Y + 1);

        OutMax.X = FMath::Max(OutMax.X, Part->GridPosition.X + 1);
        OutMax.Y = FMath::Max(OutMax.Y, Part->GridPosition.Y + 1);
    }

    OutMin = OutMin * FVector2D(PartSize, PartSize);
    OutMax = OutMax * FVector2D(PartSize, PartSize);
}

bool ASimpleLandscape::IsValidCach_Iternal(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange)
{
    TMap<TSoftObjectPtr<ASimpleLandscapePart>, TArray<TSharedPtr<FSimpleLayer>>> LayersByLandscapePart;
    TArray<TSoftObjectPtr<ASimpleLandscapePart>> LandscapePartsInRange = FindLandscapePartsInGlobalRange(MinRange - FVector2D(GridStep, GridStep), MaxRange + FVector2D(GridStep, GridStep));
    FVector2D OutMinRange;
    FVector2D OutMaxRange;
    GetGridBounds(LandscapePartsInRange, OutMinRange, OutMaxRange);
    TArray<FBrushRangeStruct> BrushRangeStructs;

    if (!IsAllValidIndex(BlueprintBrushStruct))
    {
        return false;
    }

    for (USimpleLandscapeBlueprintBrush* Brush : BlueprintBrushStruct.Brushes)
    {
        Brush->GetApplySize(OutMinRange, OutMaxRange, BrushRangeStructs);
        for (FBrushRangeStruct& BrushRangeStruct : BrushRangeStructs)
        {
            FVector2D StartPointGlobalVector;
            FVector2D EndPointGlobalVector;
            TransformTextureVectorToGlobalVector(BrushRangeStruct.StartPoint, StartPointGlobalVector);
            TransformTextureVectorToGlobalVector(BrushRangeStruct.EndPoint, EndPointGlobalVector);
            TArray<TSoftObjectPtr<ASimpleLandscapePart>> TriggeredLandscapeParts = FindLandscapePartsInGlobalRange(StartPointGlobalVector - FVector2D(GridStep, GridStep), EndPointGlobalVector + FVector2D(GridStep, GridStep));

            if (Brush->AffectHeightmap)
            {
                for (TSoftObjectPtr<ASimpleLandscapePart> TriggeredLandscapePart : TriggeredLandscapeParts)
                {
                    if (!LayersByLandscapePart.Find(TriggeredLandscapePart))
                    {
                        LayersByLandscapePart.Add(TriggeredLandscapePart);
                    }
                    LayersByLandscapePart.Find(TriggeredLandscapePart)->Add(GetHeightmapLayer());
                }
            }
            if (Brush->AffectWeightmap)
            {
                for (FName AffectedWeightmapLayerName : Brush->AffectedWeightmapLayers)
                {
                    if (TSharedPtr<FSimpleLayer> WeightmapLayer = GetWeightmapLayerByName(AffectedWeightmapLayerName))
                    {
                        for (TSoftObjectPtr<ASimpleLandscapePart> TriggeredLandscapePart : TriggeredLandscapeParts)
                        {
                            if (!LayersByLandscapePart.Find(TriggeredLandscapePart))
                            {
                                LayersByLandscapePart.Add(TriggeredLandscapePart);
                            }
                            LayersByLandscapePart.Find(TriggeredLandscapePart)->Add(WeightmapLayer);
                        }
                    }
                }
            }
        }
    }

    if (LandscapePartsInRange.Num() <= 0)
    {
        return false;
    }

    for (TSoftObjectPtr<ASimpleLandscapePart> LandscapePart : LandscapePartsInRange)
    {
        if (!LandscapePart->CacheLayerTextures.Contains(BlueprintBrushStruct))
        {
            if (LayersByLandscapePart.Contains(LandscapePart) && LayersByLandscapePart.Find(LandscapePart)->Num() > 0)
            {
                return false;
            }
        }
        else
        {
            if (!(LayersByLandscapePart.Contains(LandscapePart) && LayersByLandscapePart.Find(LandscapePart)->Num() > 0))
            {
                return false;
            }
        }

        TMap<FName, UTexture2D*>* LandscapeMaps = &LandscapePart->CacheLayerTextures.Find(BlueprintBrushStruct)->TextureByNameMap;

        if (!LayersByLandscapePart.Contains(LandscapePart))
        {
            continue;
        }

        for (TSharedPtr<FSimpleLayer> SimpleLayer : *LayersByLandscapePart.Find(LandscapePart))
        {
            if (!LandscapeMaps->Contains(SimpleLayer->Name))
            {
                return false;
            }
            UTexture2D* Texture = *LandscapeMaps->Find(SimpleLayer->Name);
            if (!Texture)
            {
                return false;
            }

            FTexturePlatformData* TextureData = Texture->GetPlatformData();
            const int32 TextureWidth = TextureData->SizeX;
            const int32 TextureHeight = TextureData->SizeY;
            if (TextureWidth != PartSize / GridStep + 1 || TextureHeight != PartSize / GridStep + 1)
            {
                return false;
            }

            if (SimpleLayer->SimpleLayerType != ESimpleLayerType::Heightmap && (Texture->LODGroup == TEXTUREGROUP_Pixels2D) == UseBlendTextureForWeightmaps)
            {
                return false;
            }
        }
    }
    return true;
};


bool ASimpleLandscape::IsValidCach(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange)
{
    int Index = LandscapeBlueprintBrushes.Find(BlueprintBrushStruct);

    if (IsValidCach_Iternal(BlueprintBrushStruct, MinRange, MaxRange))
    {
        if (Index == 0)
        {
            return true;
        }
        else
        {
            return IsValidCach(LandscapeBlueprintBrushes[Index-1], MinRange, MaxRange);
        }
    }
    else
    {
        return false;
    }
}


bool ASimpleLandscape::IsAllValidIndex(const FBlueprintBrushStruct& BlueprintBrushStruct)
{
    for (USimpleLandscapeBlueprintBrush* Brush : BlueprintBrushStruct.Brushes)
    {
        if (!IsValid(Brush))
        {
            return false;
        }
    }
    return true;
}



void ASimpleLandscape::RebuildBrush(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange, int DepthRecursion)
{
    if (UpdateBrushSettingsTickerHandle.IsValid())
    {
        UpdateBrushSettingsTickerHandle.Reset();
    }
    for (FBlueprintBrushStruct LandscapeBlueprintBrush : LandscapeBlueprintBrushes)
    {
        if (!IsAllValidIndex(LandscapeBlueprintBrush))
        {
            return;
        }
    }

    if (!IsValidRange(MinRange, MaxRange))
    {
        return;
    }

    if (UseBakeStaticMesh)
    {
        return;
    }

    if (!LandscapeBlueprintBrushes.Contains(BlueprintBrushStruct))
    {
        return;
    }

    if (DepthRecursion > MaxDepthRebuildRecursion)
    {
        UE_LOG(LogTemp, Warning, TEXT("DepthRecursion Fatal Rebuild"));
        return;
    }
    int Index = LandscapeBlueprintBrushes.Find(BlueprintBrushStruct);

    if (LandscapeBlueprintBrushes[0] == BlueprintBrushStruct || Index > 0 && IsValidCach(LandscapeBlueprintBrushes[Index - 1], MinRange, MaxRange))
    {
        RebuildBrush_Iternal(BlueprintBrushStruct, MinRange, MaxRange);
        if (LandscapeBlueprintBrushes.IsValidIndex(Index + 1) && LandscapeBlueprintBrushes[Index + 1].Brushes.Num()>0 && IsAllValidIndex(LandscapeBlueprintBrushes[Index + 1]))
        {
            RebuildBrush(LandscapeBlueprintBrushes[Index + 1], MinRange, MaxRange, DepthRecursion + 1);
        }
        else
        {
            ApplyHeightmapAndWeightmapsByCache(MinRange, MaxRange);
        }
    }
    else
    {
        RebuildBrush(LandscapeBlueprintBrushes[Index - 1], MinRange, MaxRange, DepthRecursion + 1);
    }
}

void ASimpleLandscape::ReciveCachOrRebuildFromLastCach(const FVector2D MinRange, const FVector2D MaxRange)
{
    if (LandscapeBlueprintBrushes.Num() > 0)
    {
        if (IsValidCach(LandscapeBlueprintBrushes[LandscapeBlueprintBrushes.Num() - 1], MinRange, MaxRange))
        {
            ApplyHeightmapAndWeightmapsByCache(MinRange, MaxRange);
        }
        else
        {
            RebuildBrush(LandscapeBlueprintBrushes[LandscapeBlueprintBrushes.Num() - 1], MinRange, MaxRange);
        }
    }
}

bool ASimpleLandscape::IsValidRange(const FVector2D& MinRange, const FVector2D& MaxRange) const
{
    if (MinRange == MaxRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("IsValidRange: MinRange and MaxRange are identical!"));
        return false;
    }

    if (MinRange.X > MaxRange.X)
    {
        UE_LOG(LogTemp, Warning, TEXT("IsValidRange: MinRange.X (%f) is greater than MaxRange.X (%f)!"), MinRange.X, MaxRange.X);
        return false;
    }

    if (MinRange.Y > MaxRange.Y)
    {
        UE_LOG(LogTemp, Warning, TEXT("IsValidRange: MinRange.Y (%f) is greater than MaxRange.Y (%f)!"), MinRange.Y, MaxRange.Y);
        return false;
    }

    if (!FMath::IsFinite(MinRange.X) || !FMath::IsFinite(MinRange.Y) ||
        !FMath::IsFinite(MaxRange.X) || !FMath::IsFinite(MaxRange.Y))
    {
        UE_LOG(LogTemp, Error, TEXT("IsValidRange: MinRange or MaxRange contains infinite or NaN values!"));
        return false;
    }

    return true;
}


void ASimpleLandscape::ApplyHeightmapAndWeightmapsByCache(const FVector2D MinRange, const FVector2D MaxRange)
{
    if (!IsValidRange(MinRange, MaxRange))
    {
        return;
    }
    if (UseBakeStaticMesh)
    {
        return;
    }

    TArray<TSoftObjectPtr<ASimpleLandscapePart>> SimpleLandscapeParts = FindLandscapePartsInGlobalRange(MinRange - FVector2D(GridStep,GridStep), MaxRange + FVector2D(GridStep, GridStep));

    for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : SimpleLandscapeParts)
    {
        SimpleLandscapePart->ClearWeightmaps();
        SimpleLandscapePart->ClearHeightmap();

        if (!(LandscapeBlueprintBrushes.Num()>0) || SimpleLandscapePart->CacheLayerTextures.Num()==0)
        {
            continue;
        }

        for (TSharedPtr<FSimpleLayer> Layer : Layers)
        {
            UTexture2D* CachTexture = SimpleLandscapePart->GetLastCachTextureByLayer(Layer);

            if (CachTexture)
            {
                if (Layer->SimpleLayerType == ESimpleLayerType::Heightmap)
                {
                    SimpleLandscapePart->SetupHeightmap(TPair<TSharedPtr<FSimpleLayer>, UTexture2D*>(Layer, CachTexture));
                }
                else if (Layer->SimpleLayerType == ESimpleLayerType::Weightmap)
                {
                    SimpleLandscapePart->ApplyWeightmap(TPair<TSharedPtr<FSimpleLayer>, UTexture2D*>(Layer, CachTexture));
                }
            }
        }
    }
}

void ASimpleLandscape::ClearAllInvalidSource(TSoftObjectPtr<ASimpleLandscapePart> LandscapePart)
{
    if (!LandscapePart)
    {
        return;
    }

    LandscapePart->AffectedSource.RemoveAll([](UObject* Object)
        {
            return !IsValid(Object);
        });
}

void ASimpleLandscape::RebuildBrush_Iternal(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(AllRebuildBrushTimer);

    TMap<TSharedPtr<FSimpleLayer>, UTextureCombinator*> TextureCombinatorByLayerMap = GetTextureCombinerByCache(BlueprintBrushStruct, MinRange, MaxRange);
    TArray<TSoftObjectPtr<ASimpleLandscapePart>> LandscapePartsInRange = FindLandscapePartsInGlobalRange(MinRange - FVector2D(GridStep, GridStep), MaxRange + FVector2D(GridStep, GridStep));

    for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : LandscapePartsInRange)
    {
        ClearAllInvalidSource(SimpleLandscapePart);
    }

    FVector2D OutMinRange;
    FVector2D OutMaxRange;
    GetGridBounds(LandscapePartsInRange, OutMinRange, OutMaxRange);

    int Count = 0;

    TMap<TSoftObjectPtr<ASimpleLandscapePart>, TArray<TSharedPtr<FSimpleLayer>>> LayersByLandscapePart;

    for (USimpleLandscapeBlueprintBrush* Brush : BlueprintBrushStruct.Brushes)
    {
        if (!Brush)
        {
            return;
        }
        
        Count++;
        TArray<FBrushRangeStruct> BrushRangeStructs;
        Brush->GetApplySize(OutMinRange, OutMaxRange, BrushRangeStructs);

        TArray<FSimpleLandscapeBrushParameters> SimpleLandscapeBrushParametersArray;
        {
            TRACE_CPUPROFILER_EVENT_SCOPE(ParametersBakingTimer);
            for (FBrushRangeStruct& BrushRangeStruct : BrushRangeStructs)
            {
                FVector2D StartPointGlobalVector;
                FVector2D EndPointGlobalVector;
                TransformTextureVectorToGlobalVector(BrushRangeStruct.StartPoint, StartPointGlobalVector);
                TransformTextureVectorToGlobalVector(BrushRangeStruct.EndPoint, EndPointGlobalVector);
                TArray<TSoftObjectPtr<ASimpleLandscapePart>> TriggeredLandscapeParts = FindLandscapePartsInGlobalRange(StartPointGlobalVector - FVector2D(GridStep, GridStep), EndPointGlobalVector + FVector2D(GridStep, GridStep));

                bool OnceSucsess = false;
                for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : LandscapePartsInRange)
                {
                    if (!TriggeredLandscapeParts.Contains(SimpleLandscapePart) && SimpleLandscapePart->AffectedSource.Contains(BrushRangeStruct.Object))
                    {
                        SimpleLandscapePart->AffectedSource.Remove(BrushRangeStruct.Object);
                    }
                    else
                    {
                        OnceSucsess = true;
                    }
                }

                for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : TriggeredLandscapeParts)
                {
                    SimpleLandscapePart->AffectedSource.AddUnique(BrushRangeStruct.Object);
                }

                if (!OnceSucsess)
                {
                    continue;
                }

                if (!IsValidRange(BrushRangeStruct.StartPoint, BrushRangeStruct.EndPoint))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Non Valid Range"));
                    continue;
                }

                BrushRangeStruct.EndPoint = BrushRangeStruct.EndPoint + FVector2D(1, 1);
                if (Brush->AffectHeightmap)
                {
                    FSimpleLandscapeBrushParameters SimpleLandscapeBrushParameters;
                    SimpleLandscapeBrushParameters.BrushRangeStruct = BrushRangeStruct;
                    TSharedPtr<FSimpleLayer> HeightmapLayer = GetHeightmapLayer();
                    UTextureCombinator* TextureCombinator = *TextureCombinatorByLayerMap.Find(HeightmapLayer);

                    UTextureRenderTarget2D* TextureRenderTarget = TextureCombinator->BakePart(BrushRangeStruct.StartPoint, BrushRangeStruct.EndPoint - BrushRangeStruct.StartPoint);

#if WITH_EDITOR
                    TextureRenderTarget->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
#endif
                    TextureRenderTarget->SRGB = false;
                    TextureRenderTarget->LODGroup = TEXTUREGROUP_Pixels2D;
                    SimpleLandscapeBrushParameters.SimpleLayerType = ESimpleLayerType::Heightmap;
                    SimpleLandscapeBrushParameters.TextureRenderTarget = TextureRenderTarget;
                    SimpleLandscapeBrushParameters.SetLayer(HeightmapLayer);
                    SimpleLandscapeBrushParametersArray.Add(SimpleLandscapeBrushParameters);
                    for (TSoftObjectPtr<ASimpleLandscapePart> TriggeredLandscapePart : TriggeredLandscapeParts)
                    {
                        if (!LayersByLandscapePart.Find(TriggeredLandscapePart))
                        {
                            LayersByLandscapePart.Add(TriggeredLandscapePart);
                        }
                        LayersByLandscapePart.Find(TriggeredLandscapePart)->Add(HeightmapLayer);
                    }

                   
                }
                if (Brush->AffectWeightmap)
                {
                    for(FName AffectedWeightmapLayerName: Brush->AffectedWeightmapLayers)
                    {
                        if (TSharedPtr<FSimpleLayer> WeightmapLayer = GetWeightmapLayerByName(AffectedWeightmapLayerName))
                        {
                            FSimpleLandscapeBrushParameters SimpleLandscapeBrushParameters;
                            SimpleLandscapeBrushParameters.BrushRangeStruct = BrushRangeStruct;
                            UTextureCombinator* TextureCombinator = *TextureCombinatorByLayerMap.Find(WeightmapLayer);
                            UTextureRenderTarget2D* TextureRenderTarget = TextureCombinator->BakePart(BrushRangeStruct.StartPoint, BrushRangeStruct.EndPoint - BrushRangeStruct.StartPoint);
                            if (!UseBlendTextureForWeightmaps)
                            {
#if WITH_EDITOR
                                TextureRenderTarget->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
#endif
                                TextureRenderTarget->SRGB = false;
                                TextureRenderTarget->LODGroup = TEXTUREGROUP_Pixels2D;
                            }
                            SimpleLandscapeBrushParameters.SimpleLayerType = ESimpleLayerType::Weightmap;
                            SimpleLandscapeBrushParameters.TextureRenderTarget = TextureRenderTarget;
                            SimpleLandscapeBrushParameters.SetLayer(WeightmapLayer);
                            SimpleLandscapeBrushParametersArray.Add(SimpleLandscapeBrushParameters);

                            for (TSoftObjectPtr<ASimpleLandscapePart> TriggeredLandscapePart : TriggeredLandscapeParts)
                            {
                                if (!LayersByLandscapePart.Find(TriggeredLandscapePart))
                                {
                                    LayersByLandscapePart.Add(TriggeredLandscapePart);
                                }
                                LayersByLandscapePart.Find(TriggeredLandscapePart)->Add(WeightmapLayer);
                            }
                        }
                    }
                }   
            }
        }
        {
            TRACE_CPUPROFILER_EVENT_SCOPE(BrushBakingTimer);
            Brush->ProxyRenderLayer(SimpleLandscapeBrushParametersArray);
        }

        for (FSimpleLandscapeBrushParameters SimpleLandscapeBrushParameter : SimpleLandscapeBrushParametersArray)
        {
            if (TextureCombinatorByLayerMap.Contains(FindLayerByName(SimpleLandscapeBrushParameter.LayerName)))
            {
                UTextureCombinator* TextureCombinator = *TextureCombinatorByLayerMap.Find(FindLayerByName(SimpleLandscapeBrushParameter.LayerName));
                FVector2D Size = SimpleLandscapeBrushParameter.BrushRangeStruct.EndPoint - SimpleLandscapeBrushParameter.BrushRangeStruct.StartPoint;
                TextureCombinator->AddTextureCombinator(SimpleLandscapeBrushParameter.TextureRenderTarget, SimpleLandscapeBrushParameter.BrushRangeStruct.StartPoint, Size, Count);
            }
        }
    }
    {

        TRACE_CPUPROFILER_EVENT_SCOPE(ConvertToTextureTimer);
        for (TSoftObjectPtr<ASimpleLandscapePart> LandscapePart : LandscapePartsInRange)
        {
            TMap<FName, UTexture2D*> PairToRemoves;
            if (LandscapePart->CacheLayerTextures.Contains(BlueprintBrushStruct))
            {
                PairToRemoves = LandscapePart->CacheLayerTextures.Find(BlueprintBrushStruct)->TextureByNameMap;
            }

            LandscapePart->CacheLayerTextures.Remove(BlueprintBrushStruct);

            for (TPair<TSharedPtr<FSimpleLayer>, UTextureCombinator*> Pair : TextureCombinatorByLayerMap)
            {
                if (!LayersByLandscapePart.Contains(LandscapePart))
                {
                    continue;
                }
                if (!LayersByLandscapePart.Find(LandscapePart)->Contains(Pair.Key))
                {
                    continue;
                }

                FVector2D TextureVector;
                TransformGlobalVectorToTextureVector(LandscapePart->GridPosition * PartSize, TextureVector);
                UTextureRenderTarget2D* NewRenderTargetTexture = Pair.Value->BakePart(TextureVector, FVector2D(PartSize/GridStep + 1, PartSize/GridStep + 1));
                FString ResultPath = FString(GetWorld()->GetCurrentLevel()->GetOutermost()->GetPathName() + FString("Textures") + "/" + GetName() + "/" +
                    LandscapePart->GetName() + "_" + Pair.Key->Name.ToString() + "_Index_" +
                    FString::FromInt(LandscapeBlueprintBrushes.Find(BlueprintBrushStruct)));
                UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *ResultPath);

                UTexture2D* NewTexture = nullptr;

                if (ExistingTexture)
                {
                    UKismetRenderingLibrary::ConvertRenderTargetToTexture2DEditorOnly(GetWorld(), NewRenderTargetTexture, ExistingTexture);
                    NewTexture = ExistingTexture;
                }
                else
                {
                    NewTexture = UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(NewRenderTargetTexture, ResultPath);
                }
                NewTexture->SRGB = false;
#if WITH_EDITOR
                NewTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
#endif
                NewTexture->AddressX = TextureAddress::TA_Clamp;
                NewTexture->AddressY = TextureAddress::TA_Clamp;

                if (Pair.Key->SimpleLayerType == ESimpleLayerType::Heightmap || !UseBlendTextureForWeightmaps)
                {
                    NewTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
                    NewTexture->LODGroup = TEXTUREGROUP_Pixels2D;
                }
                else
                {
                    NewTexture->CompressionSettings = TextureCompressionSettings::TC_Default;
                    NewTexture->LODGroup = TEXTUREGROUP_World;
                }

                NewTexture->UpdateResource();

                UPackage* Package = NewTexture->GetOutermost(); 
                if (!Package)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to get package for texture."));
                    return;
                }

                Package->FullyLoad(); 
                Package->MarkPackageDirty();

                FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
                FSavePackageArgs SaveArgs;
				SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
                UPackage::SavePackage(Package, NewTexture, *PackageFileName, SaveArgs);


                if (!LandscapePart->CacheLayerTextures.Contains(BlueprintBrushStruct))
                {
                    LandscapePart->CacheLayerTextures.Add(BlueprintBrushStruct);
                }
                LandscapePart->CacheLayerTextures.Find(BlueprintBrushStruct)->TextureByNameMap.Add(Pair.Key->Name, NewTexture);
                PairToRemoves.Remove(Pair.Key->Name);
            }

            for (TPair<FName, UTexture2D*> PairToRemove : PairToRemoves)
            {
                if (PairToRemove.Value)
                {
                    FAssetRegistryModule::AssetDeleted(PairToRemove.Value);
                }
            }
        }

        FlushRenderingCommands();
    }
}
 //WITH_EDITOR

void USimpleLandscapeBlueprintBrush::ProxyRenderLayer(TArray<FSimpleLandscapeBrushParameters>& SimpleLandscapeBrushParameters)
{
    for (FSimpleLandscapeBrushParameters SimpleLandscapeBrushParameter : SimpleLandscapeBrushParameters)
    {
        RenderLayer(SimpleLandscapeBrushParameter);
    }
}

#if WITH_EDITOR
void USimpleLandscapeBlueprintBrush::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (IsValid(SimpleLandscape) && SimpleLandscape->UpdateBrushChange)
    {
        FBlueprintBrushStruct BlueprintBrushStruct;
        SimpleLandscape->FindBrushStructWithBrushObject(this, BlueprintBrushStruct);
        SimpleLandscape->UpdateBrushSettings(BlueprintBrushStruct,FVector2D(0,0), FVector2D(SimpleLandscape->PartCountX* SimpleLandscape->PartSize, SimpleLandscape->PartCountY * SimpleLandscape->PartSize));
    }

}
#endif


UWorld* USimpleLandscapeBlueprintBrush::GetWorld() const
{
    if (SimpleLandscape&&SimpleLandscape->GetWorld())
    {
        return SimpleLandscape->GetWorld();
    }
    else
    {
        return Super::GetWorld();
    }
}

bool ASimpleLandscape::FindBrushStructWithBrushObject(USimpleLandscapeBlueprintBrush* SimpleLandscapeBlueprintBrush, FBlueprintBrushStruct& BlueprintBrushStruct)
{
    for (FBlueprintBrushStruct LocalBlueprintBrushStruct : LandscapeBlueprintBrushes)
    {
        if (LocalBlueprintBrushStruct.Brushes.Contains(SimpleLandscapeBlueprintBrush))
        {
            BlueprintBrushStruct = LocalBlueprintBrushStruct;
            return true;
        }
    }
    return false;
}

bool ASimpleLandscape::FindBrushStructsWithBrushObjectClass(TSubclassOf<USimpleLandscapeBlueprintBrush> SimpleLandscapeBlueprintBrush, TArray<FBlueprintBrushStruct>& BlueprintBrushStructs)
{
    bool bFound = false;

    for (FBlueprintBrushStruct& LocalBlueprintBrushStruct : LandscapeBlueprintBrushes)
    {
        for (USimpleLandscapeBlueprintBrush* Brush : LocalBlueprintBrushStruct.Brushes)
        {
            if (Brush && Brush->IsA(SimpleLandscapeBlueprintBrush))
            {
                BlueprintBrushStructs.Add(LocalBlueprintBrushStruct);
                bFound = true;
                break; 
            }
        }
    }

    return bFound;
}

bool ASimpleLandscape::FindBrushStructsWithBrushObjectClassAndTag(TSubclassOf<USimpleLandscapeBlueprintBrush> SimpleLandscapeBlueprintBrush, FName FindTag, TArray<FBlueprintBrushStruct>& BlueprintBrushStructs)
{
    bool bFound = false;

    for (FBlueprintBrushStruct& LocalBlueprintBrushStruct : LandscapeBlueprintBrushes)
    {
        for (USimpleLandscapeBlueprintBrush* Brush : LocalBlueprintBrushStruct.Brushes)
        {
            if (Brush && Brush->IsA(SimpleLandscapeBlueprintBrush) && Brush->Tags.Contains(FindTag))
            {
                BlueprintBrushStructs.Add(LocalBlueprintBrushStruct);
                bFound = true;
                break;
            }
        }
    }

    return bFound;
}

bool ASimpleLandscape::FindBrushStructsWithBrushTag(FName FindTag, TArray<FBlueprintBrushStruct>& BlueprintBrushStructs)
{
    bool bFound = false;

    for (FBlueprintBrushStruct& LocalBlueprintBrushStruct : LandscapeBlueprintBrushes)
    {
        for (USimpleLandscapeBlueprintBrush* Brush : LocalBlueprintBrushStruct.Brushes)
        {
            if (Brush && Brush->Tags.Contains(FindTag))
            {
                BlueprintBrushStructs.Add(LocalBlueprintBrushStruct);
                bFound = true;
                break;
            }
        }
    }

    return bFound;
}


void ASimpleLandscape::ClearAllCacheInLandscapeParts()
{
    for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : LandscapeParts)
    {
        if (!SimpleLandscapePart.IsValid())
        {
            continue;
        }
        for (TPair<FBlueprintBrushStruct, FTextureByNameMapStruct> TextureByNameMapStruct : SimpleLandscapePart->CacheLayerTextures)
        {
            for (TPair<FName, UTexture2D*> TextureByNamePair : TextureByNameMapStruct.Value.TextureByNameMap)
            {
                if (TextureByNamePair.Value)
                {
                    //const TArray<UObject*> InObjectsToDelete = { TextureByNamePair.Value };
                    //ObjectTools::ForceDeleteObjects(InObjectsToDelete, false);
                    FAssetRegistryModule::AssetDeleted(TextureByNamePair.Value);
                }
            }     
        }
        SimpleLandscapePart->CacheLayerTextures.Empty();
        SimpleLandscapePart->ClearHeightmap();
        SimpleLandscapePart->ClearWeightmaps();
    }
}


#if WITH_EDITOR
void ASimpleLandscape::BackingToStaticMesh()
{
    if (UseBakeStaticMesh)
    {
        return;
    }
    for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : LandscapeParts)
    {
        FString ResultPath = FString(GetWorld()->GetCurrentLevel()->GetOutermost()->GetPathName() + FString("StaticMeshes") + "/" + GetName());
        UStaticMesh* StaticMesh = UUpgradeBlueprintFunctionLibrary::ConvertProceduralToStatic(SimpleLandscapePart->ProceduralMeshComponent, SimpleLandscapePart->GetName() + "Mesh", ResultPath);
        SimpleLandscapePart->StaticMeshComponent->SetStaticMesh(StaticMesh);
        SimpleLandscapePart->ProceduralMeshComponent->ClearMeshSection(0);
    }
    UseBakeStaticMesh = true;
}
#endif

#if WITH_EDITOR
void ASimpleLandscape::ClearAllStaticMesh()
{
    if (!UseBakeStaticMesh)
    {
        return;
    }
    for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : LandscapeParts)
    {
        UStaticMesh* StaticMesh = SimpleLandscapePart->StaticMeshComponent->GetStaticMesh();
        if(StaticMesh)
        { 
            SimpleLandscapePart->StaticMeshComponent->SetStaticMesh(nullptr);
            const TArray<UObject*> InObjectsToDelete = { StaticMesh };

            if (EnableAsync)
            {
                FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, InObjectsToDelete]()
                    {
                        ObjectTools::ForceDeleteObjects(InObjectsToDelete, false);
                    }, TStatId(), NULL, ENamedThreads::GameThread);
            }
            else
            {
                ObjectTools::ForceDeleteObjects(InObjectsToDelete, false);
            }

            SimpleLandscapePart->Initialization();
        }
    }
    UseBakeStaticMesh = false;
}
#endif

void ASimpleLandscape::ClearHeightmapInLandscapeParts()
{
    for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : LandscapeParts)
    {
        SimpleLandscapePart->ClearHeightmap();
    }
}

void ASimpleLandscape::ClearWeightmapsInLandscapeParts()
{
    for (TSoftObjectPtr<ASimpleLandscapePart>  SimpleLandscapePart : LandscapeParts)
    {
        SimpleLandscapePart->ClearWeightmaps();
    }
}

void ASimpleLandscape::ClearAllLandscapePart()
{
    ClearAllCacheInLandscapeParts();
    int k = 0;
    while(LandscapeParts.IsValidIndex(k))
    {
        TSoftObjectPtr<ASimpleLandscapePart> RemoveActor = LandscapeParts[k];
        if (RemoveActor)
        {
            RemoveActor->Destroy();
        }
        k++;
    }
    LandscapeParts.Empty();
}

void ASimpleLandscape::K2_DrawMaterialCustom(UCanvas* Canvas, ESimpleElementBlendModeBlueprint  SimpleElementBlendMode, UMaterialInterface* RenderMaterial, FVector2D ScreenPosition, FVector2D ScreenSize, FVector2D CoordinatePosition, FVector2D CoordinateSize, float Rotation, FVector2D PivotPoint)
{
    if (RenderMaterial
        && ScreenSize.X > 0.0f
        && ScreenSize.Y > 0.0f
        // Canvas can be NULL if the user tried to draw after EndDrawCanvasToRenderTarget
        && Canvas)
    {
        // This is a user-facing function, so we'd rather make sure that shaders are ready by the time we render, in order to ensure we don't draw with a fallback material :
        RenderMaterial->EnsureIsComplete();

         if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
        {
            // Should be moved earlier
            // Should be moved earlier
             FPSOPrecacheParams PSOPrecacheParams;
             PSOPrecacheParams.bCanvasMaterial = true;
             PSOPrecacheParams.BasePassPixelFormat = (Canvas->Canvas->GetRenderTarget() && Canvas->Canvas->GetRenderTarget()->GetRenderTargetTexture()) ? Canvas->Canvas->GetRenderTarget()->GetRenderTargetTexture()->GetDesc().Format : PF_Unknown;
             RenderMaterial->PrecachePSOs(&FLocalVertexFactory::StaticType, PSOPrecacheParams);
        }

        FCanvasTileItem TileItem(ScreenPosition, RenderMaterial->GetRenderProxy(), ScreenSize, CoordinatePosition, CoordinatePosition + CoordinateSize);
        TileItem.Rotation = FRotator(0, Rotation, 0);
        TileItem.PivotPoint = PivotPoint;
        TileItem.BlendMode = ESimpleElementBlendMode(SimpleElementBlendMode);
        Canvas->DrawItem(TileItem);
    }
}

void ASimpleLandscape::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    ClearAllLandscapePart();
}

TArray<UTexture2D*> ASimpleLandscape::LoadAllTexturesInSameFolder(const FString& AssetPath)
{
    TArray<UTexture2D*> LoadedTextures;
    FString LFolderPath = FPaths::GetPath(AssetPath); // Получаем путь к папке
    FString FullPath;

    if (!FPackageName::TryConvertLongPackageNameToFilename(LFolderPath, FullPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to convert package name to filename for: %s"), *LFolderPath);
        return LoadedTextures;
    }

    TArray<FString> AssetFiles;
    IFileManager::Get().FindFiles(AssetFiles, *(FullPath / TEXT("*.uasset")), true, false);

    for (const FString& AssetFile : AssetFiles)
    {
        FString AssetName = FPaths::GetBaseFilename(AssetFile);
        FString AssetFullPath = LFolderPath / AssetName;

        UE_LOG(LogTemp, Log, TEXT("Trying to load asset: %s"), *AssetFullPath);

        UObject* LoadedAsset = LoadObject<UObject>(nullptr, *AssetFullPath);
        if (LoadedAsset && LoadedAsset->IsA<UTexture2D>())
        {
            UTexture2D* Texture = Cast<UTexture2D>(LoadedAsset);
            if (Texture)
            {
                LoadedTextures.Add(Texture);
                UE_LOG(LogTemp, Log, TEXT("Loaded texture: %s"), *Texture->GetName());
            }
        }
    }

    return LoadedTextures;
}

void ASimpleLandscape::BeginDestroy()
{
    Super::BeginDestroy();

    
}

void ASimpleLandscape::DeleteAllNonValidCach()
{
#if WITH_EDITOR
    if (!GetWorld() || !GIsEditor)
    {
        return;
    }

    FString ResultPath = FString(GetWorld()->GetCurrentLevel()->GetOutermost()->GetPathName() + FString("Textures") + "/" + GetName() + "/");

    TArray<UTexture2D*> CacheLayerTextures;
    for (TSoftObjectPtr<ASimpleLandscapePart> SimpleLandscapePart : LandscapeParts)
    {
        if (!SimpleLandscapePart.IsValid())
        {
            continue;
        }
        for (TPair<FBlueprintBrushStruct, FTextureByNameMapStruct> Pair : SimpleLandscapePart->CacheLayerTextures)
        {
            for (TPair<FName, UTexture2D*> TextureByValue : Pair.Value.TextureByNameMap)
            {
                CacheLayerTextures.Add(TextureByValue.Value);
            }
        }
    }
    TArray<UTexture2D*> Textures = LoadAllTexturesInSameFolder(ResultPath);

    TArray<UObject*> InObjectsToDelete;

    for (UTexture2D* Texture : Textures)
    {
        if (!CacheLayerTextures.Contains(Texture))
        {
            InObjectsToDelete.Add(Texture);
        }
    }

    if (EnableAsync)
    {
        FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, InObjectsToDelete]()
            {
                ObjectTools::ForceDeleteObjects(InObjectsToDelete, false);
            }, TStatId(), NULL, ENamedThreads::GameThread);
    }
    else
    {
        ObjectTools::ForceDeleteObjects(InObjectsToDelete, false);
    }
#endif
}



void ASimpleLandscape::Destroyed()
{
    Super::Destroyed();
    ClearAllLandscapePart();
}

bool ASimpleLandscape::AreLayersEqual() const
{
    if (Layers.Num() != DefaultLayers.Num())
    {
        return false;
    }

    for (int32 i = 0; i < DefaultLayers.Num(); i++)
    {
        if (*Layers[i].Get() != DefaultLayers[i])
        {
            return false;
        }
    }

    return true;
}


void ASimpleLandscape::SetupLayers()
{
    if (!AreLayersEqual() || DefaultLayers.Num() == 0 || Layers.Num() == 0)
    {
        ClearLayers();

        FSimpleLayer SimpleLayer;
        SimpleLayer.Name = "Heightmap";
        SimpleLayer.SimpleLayerType = ESimpleLayerType::Heightmap;

        DefaultLayers.Add(SimpleLayer);
        SetupSimpleLayersByMaterialLayers();

        for (FSimpleLayer Layer : DefaultLayers)
        {
            Layers.Add(MakeShared<FSimpleLayer>(Layer));
        }
    }
}

void ASimpleLandscape::SetupSimpleLayersByMaterialLayers()
{
#if WITH_EDITOR
    if (!IsValid(LandscapeMaterial))
    {
        return;
    }
    static FName LayersParamName(TEXT("MaterialLayers"));
    FMaterialLayersFunctions MaterialLayersFunctions;
    LandscapeMaterial->GetMaterialLayers(MaterialLayersFunctions);

    for (int32 i = 1; i < MaterialLayersFunctions.Layers.Num(); i++)
    {
        FSimpleLayer SimpleLayer;
        SimpleLayer.Name = FName(MaterialLayersFunctions.GetLayerName(i).ToString());
        SimpleLayer.SimpleLayerType = ESimpleLayerType::Weightmap;
        SimpleLayer.MaterialIndexForWeightmap = i - 1;

        DefaultLayers.Add(SimpleLayer);
    }

    FSimpleLayer SimpleLayer;
    SimpleLayer.Name = FName(MaterialLayersFunctions.GetLayerName(0).ToString());
    SimpleLayer.SimpleLayerType = ESimpleLayerType::Weightmap;
    SimpleLayer.MaterialIndexForWeightmap = MaterialLayersFunctions.Layers.Num() - 1;

    DefaultLayers.Add(SimpleLayer);
#endif
}

void ASimpleLandscape::ClearLayers()
{
    DefaultLayers.Empty();
    Layers.Empty();
}

