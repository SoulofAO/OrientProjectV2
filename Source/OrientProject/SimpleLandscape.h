#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Magic/AbstractClasses/UpgradeObject.h"
#include "TextureCombinator.h"
#include "Containers/UnrealString.h" 
#include "Containers/Ticker.h"
#include "Engine/DataAsset.h"
#include "SimpleLandscape.generated.h"

class UDataAsset;

USTRUCT(Blueprintable)
struct FBrushRangeStruct
{
    GENERATED_BODY()

    FBrushRangeStruct(FVector2D NewStartPoint = FVector2D(0,0), FVector2D NewEndPoint = FVector2D(0, 0))
        : StartPoint(NewStartPoint), EndPoint(NewEndPoint)
    {};
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D StartPoint;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D EndPoint;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UObject* Object;
};


struct FSimpleLandscapeLayerTextureCache
{
    UTexture2D* Texture2D;
};

USTRUCT(Blueprintable)
struct FBlueprintBrushStruct
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
    TArray<USimpleLandscapeBlueprintBrush*> Brushes;

    // Определяем оператор равенства
    bool operator==(const FBlueprintBrushStruct& Other) const
    {
        if (Brushes.Num() != Other.Brushes.Num())
        {
            return false;
        }

        for (int32 i = 0; i < Brushes.Num(); ++i)
        {
            if (Brushes[i] != Other.Brushes[i])
            {
                return false;
            }
        }

        return true;
    }

    // Определяем GetTypeHash
    friend uint32 GetTypeHash(const FBlueprintBrushStruct& BrushStruct)
    {
        uint32 Hash = 0;
        for (const auto& Brush : BrushStruct.Brushes)
        {
            Hash ^= GetTypeHash(Brush); // Или используйте другой способ хеширования для ваших указателей
        }
        return Hash;
    }
};


UENUM(Blueprintable)
enum class ESimpleLayerType : uint8
{
    Weightmap,
    Heightmap
};

UENUM(Blueprintable)
enum class EDebugLandscapeMode : uint8
{
    None,
    HeightmapFloat, 
    HeightmapColor
};

USTRUCT(BlueprintType)
struct FSimpleLayer
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    FName Name;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    ESimpleLayerType SimpleLayerType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    int MaterialIndexForWeightmap = -1;

    bool operator==(const FSimpleLayer& Other) const
    {
        return Name == Other.Name &&
            SimpleLayerType == Other.SimpleLayerType &&
            MaterialIndexForWeightmap == Other.MaterialIndexForWeightmap;
    }

    friend uint32 GetTypeHash(const FSimpleLayer& Layer)
    {
        return HashCombine(HashCombine(GetTypeHash(Layer.Name), GetTypeHash(Layer.SimpleLayerType)), GetTypeHash(Layer.MaterialIndexForWeightmap));
    }


    // Определяем оператор равенства
};

USTRUCT(Blueprintable)
struct FSimpleLandscapeBrushParameters
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FBrushRangeStruct BrushRangeStruct;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UTextureRenderTarget2D* TextureRenderTarget;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ESimpleLayerType SimpleLayerType;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName LayerName;

    void SetLayer(TSharedPtr<FSimpleLayer> NewLayer)
    {
        if (!NewLayer.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("NewLayerNonValid"));
            return;
        }
        Layer = NewLayer;
        LayerName = NewLayer->Name;
        SimpleLayerType = NewLayer->SimpleLayerType;
    }

    TSharedPtr<FSimpleLayer> Layer;
};



UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class USimpleLandscapeBlueprintBrush : public UEditInLineUpgradeObject
{
    GENERATED_BODY()
public:

    void ProxyRenderLayer(TArray<FSimpleLandscapeBrushParameters>& SimpleLandscapeBrushParameters);

    UFUNCTION(BlueprintImplementableEvent, meta = (ForceAsFunction))
    void RenderLayer(FSimpleLandscapeBrushParameters SimpleLandscapeBrushParameters);

    UFUNCTION(BlueprintImplementableEvent)
    void GetApplySize(FVector2D RangeIn, FVector2D RangeOut, TArray<FBrushRangeStruct>& OutBrushRangeStruct);

    UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
    bool AffectHeightmap;

    UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
    bool AffectWeightmap;

    UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
    bool AffectVisibilityLayer;

    UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
    TArray<FName> AffectedWeightmapLayers;

    UPROPERTY(BlueprintReadOnly)
    ASimpleLandscape* SimpleLandscape;

    UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
    TArray<FName> Tags;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    virtual UWorld* GetWorld() const override;
};


USTRUCT(Blueprintable)
struct FTextureByNameMapStruct
{
    GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    TMap<FName, UTexture2D*> TextureByNameMap;
};

UCLASS(Blueprintable)
class ASimpleLandscapePart : public AActor
{
    GENERATED_BODY()

public:

    virtual void Tick(float DeltaTime) override;

    ASimpleLandscapePart();

#if WITH_EDITOR
    virtual bool ShouldTickIfViewportsOnly() const override
    {
        return true; // Позволяет объекту тикать в редакторе
    }
#endif

    UTexture2D* GetLastCachTextureByLayer(TSharedPtr<FSimpleLayer> Layer);

    UTexture2D* GetLastCachTextureByLayerAndBrush(TSharedPtr<FSimpleLayer> Layer, const FBlueprintBrushStruct& BlueprintBrushStruct);

    void GetAllInfoByProceduralComponent(TArray<FVector>& Vertices, TArray<uint32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    int PartSize; 

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    int GridStep;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    FVector2D GridPosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UTexture2D* HeightmapTexture;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UObject*> AffectedSource;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TMap<FName, UTexture2D*> WeightmapTextureByName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    UMaterialInterface* LandscapeMaterial;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    UMaterialInterface* DebugLandscapeMaterial;

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Landscape")
    void ApplyHeightmap();

    UFUNCTION(BlueprintCallable)
    void UpdateMaterials();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Landscape")
    float GetHeightByTextureCoord(FVector2D TextureCoord);

    void SetupHeightmap(TPair<TSharedPtr<FSimpleLayer>,UTexture2D*> NewHeightmapTexture);

    void ClearHeightmap();

    void ApplyWeightmap(TPair<TSharedPtr<FSimpleLayer>, UTexture2D*> NewWeightmapTexture);

    void ClearWeightmaps();

    FVector GetWorldCoordinate(const FVector2D& LocalCoordinate, const FVector& PartOrigin) const;

    FVector2D GetGlobalCoordinate(const FVector& WorldCoordinate, const FVector& PartOrigin) const;

    void Initialization();

    FVector2D GetTextureSize();


    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* StaticMeshComponent;

    UPROPERTY(VisibleAnywhere)
    UProceduralMeshComponent* ProceduralMeshComponent;

    UPROPERTY(VisibleAnywhere)
    UMaterialInstanceDynamic* DynamicMaterial;

    UPROPERTY(VisibleAnywhere)
    UMaterialInstanceDynamic* DynamicDebugLandscapeMaterial;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    TMap<FBlueprintBrushStruct, FTextureByNameMapStruct> CacheLayerTextures;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<ASimpleLandscape> SimpleLandscape;
};

UCLASS(Blueprintable)
class USimpleLandscapeBrushsArrayData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    TArray<FBlueprintBrushStruct> LandscapeBlueprintBrushes;
};

UENUM(Blueprintable)
enum class ESimpleElementBlendModeBlueprint : uint8
{
    SE_BLEND_Opaque = 0,
    SE_BLEND_Masked,
    SE_BLEND_Translucent,
    SE_BLEND_Additive,
    SE_BLEND_Modulate,
    SE_BLEND_MaskedDistanceField,
    SE_BLEND_MaskedDistanceFieldShadowed,
    SE_BLEND_TranslucentDistanceField,
    SE_BLEND_TranslucentDistanceFieldShadowed,
    SE_BLEND_AlphaComposite,
    SE_BLEND_AlphaHoldout,
    // Like SE_BLEND_Translucent, but modifies destination alpha
    SE_BLEND_AlphaBlend,
    // Like SE_BLEND_Translucent, but reads from an alpha-only texture
    SE_BLEND_TranslucentAlphaOnly,
    SE_BLEND_TranslucentAlphaOnlyWriteAlpha,

    SE_BLEND_RGBA_MASK_START,
    SE_BLEND_RGBA_MASK_END = SE_BLEND_RGBA_MASK_START + 31, //Using 5bit bit-field for red, green, blue, alpha and desaturation

    SE_BLEND_MAX
};


UCLASS(Blueprintable)
class ASimpleLandscape : public AActor
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    virtual void OnConstruction(const FTransform& Transform);
#endif
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    bool UpdateLandscapeChange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    bool UpdateBrushChange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    EDebugLandscapeMode DebugLandscapeMode = EDebugLandscapeMode::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    int PartCountX = 2; // Количество частей по X

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    int PartCountY = 2; // Количество частей по Y

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    int PartSize = 2000; // Размер одной стороны части (в вершинах)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    int GridStep = 200; // Шаг между вершинами по X и Y

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    bool EnableAsync = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    int MaxDepthRebuildRecursion = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    bool SupportWorldPartitionLoad = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    int WorldPartitionLoadGroupSize = 4; 

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    bool UseBakeStaticMesh = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Landscape")
    bool UseBlendTextureForWeightmaps = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    TArray< TSoftObjectPtr<ASimpleLandscapePart>> LandscapeParts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    UMaterialInterface* LandscapeMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    UMaterialInterface* DebugLandscapeMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
    TArray<FBlueprintBrushStruct> LandscapeBlueprintBrushes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ImportBrushData")
    USimpleLandscapeBrushsArrayData* ImportBrushArrayData;
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "ImportBrushData")
    void ImportBrushFromArrayData();

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    TArray<FSimpleLayer> DefaultLayers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Landscape")
    UTexture2D* BlackTexture;

    FTSTicker::FDelegateHandle UpdateSettingsTickerHandle;

    FTSTicker::FDelegateHandle UpdateBrushSettingsTickerHandle;

    TArray<TSharedPtr<FSimpleLayer>> Layers;

    TSharedPtr<FSimpleLayer> FindLayerByName(FName Name);

    TSharedPtr<FSimpleLayer> GetHeightmapLayer();

    TSharedPtr<FSimpleLayer> GetWeightmapLayerByName(FName Name);

    TArray<TSharedPtr<FSimpleLayer>> GetWeightmapLayers();

    UFUNCTION(BlueprintCallable, Category = "Landscape")
    void GenerateLandscapeParts();

    UFUNCTION(BlueprintCallable, Category = "Landscape")
    TSoftObjectPtr<ASimpleLandscapePart> GetLandscapePartByCoordIndex(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "Landscape")
    bool GetLandscapePartAndGlobalCoordinate(const FVector& WorldCoordinate, TSoftObjectPtr<ASimpleLandscapePart>& OutPart, FVector2D& OutGlobalCoordinate) const;

    UFUNCTION(BlueprintCallable, Category = "Landscape", BlueprintPure)
    bool TransformWorldVectorToGlobalVector(FVector WorldVector, FVector2D& GlobalVector) const;

    UFUNCTION(BlueprintCallable)
    void UpdateSettings();

    UFUNCTION(BlueprintCallable)
    bool CheckLandscapeInRange(FBox Box);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Landscape")
    float GetHeightByTextureCoord(FVector2D TextureCoord);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Landscape")
    float GetHeightByGlobalCoord(FVector2D GlobalCoord);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Landscape")
    bool ProjectWorldPoint(FVector WorldCoord, FVector& ProjectWorldCoord);

    UFUNCTION(BlueprintCallable)
    void UpdateBrushSettings(FBlueprintBrushStruct BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange);

    FBlueprintBrushStruct UpdateBlueprintBrushStruct;

    bool SetupUpdateBlueprintBrushStruct = false;

    FVector2D UpdateMinRange;

    FVector2D UpdateMaxRange;

    UFUNCTION(BlueprintCallable, Category = "Landscape", BlueprintPure)
    bool TransformGlobalVectorToWorldVector(FVector2D GlobalVector, FVector& WorldVector) const;

    UFUNCTION(BlueprintCallable, Category = "Landscape", BlueprintPure)
    bool TransformGlobalVectorToTextureVector(FVector2D GlobalVector, FVector2D& TextureVector) const;

    UFUNCTION(BlueprintCallable, Category = "Landscape", BlueprintPure)
    bool TransformTextureVectorToGlobalVector(FVector2D TextureVector, FVector2D& GlobalVector) const;

    UFUNCTION(BlueprintCallable)
    bool FindBlueprintBrushStructByBlueprintBrushObject(int& Index, FBlueprintBrushStruct& BlueprintBrushStruct, USimpleLandscapeBlueprintBrush* SimpleLandscapeBlueprintBrush);

    UFUNCTION(BlueprintCallable, Category = "Landscape")
    TArray <TSoftObjectPtr<ASimpleLandscapePart>> FindLandscapePartsInGlobalRange(const FVector2D& MinRange, const FVector2D& MaxRange) const;

    UFUNCTION(BlueprintCallable, Category = "Landscape")
    TArray <TSoftObjectPtr<ASimpleLandscapePart>> GetNearestLandscapePartsByParts(const TArray<TSoftObjectPtr<ASimpleLandscapePart>>& InputLandscapeParts) const;

    UFUNCTION(BlueprintCallable, Category = "Landscape")
    TArray <TSoftObjectPtr<ASimpleLandscapePart>> GetNearestLandscapePartsByPart(TSoftObjectPtr<ASimpleLandscapePart> InputLandscapePart) const;

    UFUNCTION(BlueprintCallable, Category = "Landscape")
    TArray <TSoftObjectPtr<ASimpleLandscapePart>> FindLandscapePartsInTextureRange(const FVector2D& MinRange, const FVector2D& MaxRange) const;

    UFUNCTION(BlueprintCallable, Category = "Landscape")
    FVector GetWorldCoordinateFromPart(TSoftObjectPtr<ASimpleLandscapePart> Part, const FVector2D& LocalCoordinate) const;
    //Call In Some Time;
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Default|Layers")
    void RequestRebuildBrush(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange);

    //Requst From Brush Only
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Default|Layers")
    void RequestUpdateSettings();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Default|Layers")
    void RebuildAllBrush();

    TMap<TSharedPtr<FSimpleLayer>, UTextureCombinator*> GetTextureCombinerByCache(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange);

    UFUNCTION(BlueprintCallable)
    void NormalizeGlobalRangeByLandscapeParts(const FVector2D InMinRange, const FVector2D InMaxRange, FVector2D& OutMinRange, FVector2D& OutMaxRange);

    UFUNCTION(BlueprintCallable)
    void GetGridBounds(const TArray<TSoftObjectPtr<ASimpleLandscapePart>>& InputLandscapeParts, FVector2D& OutMin, FVector2D& OutMax) const;

    bool IsValidCach_Iternal(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange);

    UFUNCTION(BlueprintCallable)
    bool IsValidCach(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange);

    UFUNCTION(BlueprintCallable)
    bool IsAllValidIndex(const FBlueprintBrushStruct& BlueprintBrushStruct);
    
    UFUNCTION(BlueprintCallable)
    void RebuildBrush(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange, int DepthRecursion = 0);

    UFUNCTION(BlueprintCallable, CallInEditor)
    void ReciveCachOrRebuildFromLastCach(const FVector2D MinRange, const FVector2D MaxRange);

    bool IsValidRange(const FVector2D& MinRange, const FVector2D& MaxRange) const;

    UFUNCTION(BlueprintCallable, CallInEditor)
    void ApplyHeightmapAndWeightmapsByCache(const FVector2D MinRange, const FVector2D MaxRange);

    void ClearAllInvalidSource(TSoftObjectPtr<ASimpleLandscapePart> LandscapePart);

    void RebuildBrush_Iternal(const FBlueprintBrushStruct& BlueprintBrushStruct, const FVector2D MinRange, const FVector2D MaxRange);

    TArray<FBlueprintBrushStruct> FindBrushStructsWithPriority(int Priority);

    UFUNCTION(BlueprintCallable, CallInEditor)
    bool FindBrushStructWithBrushObject(USimpleLandscapeBlueprintBrush* SimpleLandscapeBlueprintBrush, FBlueprintBrushStruct& BlueprintBrushStruct);

    UFUNCTION(BlueprintCallable, CallInEditor)
    bool FindBrushStructsWithBrushObjectClass(TSubclassOf<USimpleLandscapeBlueprintBrush> SimpleLandscapeBlueprintBrush, TArray<FBlueprintBrushStruct>& BlueprintBrushStruct);

    UFUNCTION(BlueprintCallable)
    bool FindBrushStructsWithBrushObjectClassAndTag(TSubclassOf<USimpleLandscapeBlueprintBrush> SimpleLandscapeBlueprintBrush, FName FindTag, TArray<FBlueprintBrushStruct>& BlueprintBrushStructs);

    UFUNCTION(BlueprintCallable)
    bool FindBrushStructsWithBrushTag(FName FindTag, TArray<FBlueprintBrushStruct>& BlueprintBrushStructs);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Default|Layers")
    void ClearAllCacheInLandscapeParts();
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Default|StaticMesh")
    void BackingToStaticMesh();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Default|StaticMesh")
    void ClearAllStaticMesh();
#endif
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Default|Layers")
    void ClearHeightmapInLandscapeParts();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Default|Layers")
    void ClearWeightmapsInLandscapeParts();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Default")
    void ClearAllLandscapePart();

    UFUNCTION(BlueprintCallable, Category = "Default")
    static void K2_DrawMaterialCustom(UCanvas* Canvas, ESimpleElementBlendModeBlueprint SimpleElementBlendMode, UMaterialInterface* RenderMaterial, FVector2D ScreenPosition, FVector2D ScreenSize, FVector2D CoordinatePosition, FVector2D CoordinateSize, float Rotation, FVector2D PivotPoint);

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    TArray<UTexture2D*> LoadAllTexturesInSameFolder(const FString& AssetPath);

    //Exec when exit from editor or enter.
    virtual void BeginDestroy() override;

    void DeleteAllNonValidCach();

    //Exec when destroy actor on scene.
    virtual void Destroyed() override;

    bool AreLayersEqual() const;

    void SetupLayers();

    void SetupSimpleLayersByMaterialLayers();

    void ClearLayers();

};
