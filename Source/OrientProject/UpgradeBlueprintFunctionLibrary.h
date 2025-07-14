#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Templates/SubclassOf.h"
#include "Containers/Ticker.h"
#include "Delegates/Delegate.h"
#include "GameFramework/Actor.h"
#include "PCGCommon.h"
#include "Data/PCGSplineData.h"
#include "WorldPartition/WorldPartitionEditorLoaderAdapter.h"
#include "UpgradeBlueprintFunctionLibrary.generated.h"

/**
 * 
 */

struct FOrientSaveGameArchive : public FObjectAndNameAsStringProxyArchive
{
	FOrientSaveGameArchive(FArchive& InInnerArchive, bool bInLoadIfFindFails)
		: FObjectAndNameAsStringProxyArchive(InInnerArchive, bInLoadIfFindFails)
	{
		ArIsSaveGame = true;
	}
};

class UActorComponent;
class UProceduralMeshComponent;
class UStaticMesh;
class UPCGComponent;

UCLASS(Blueprintable)
class ORIENTPROJECT_API UWorldPartitionLoaderWrapper : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Partition")
	TObjectPtr<UWorldPartitionEditorLoaderAdapter> LoaderAdapter;
};


UCLASS()
class ORIENTPROJECT_API UUpgradeBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "ActorComponentClass"))
	static UActorComponent* AddTransactionalInstanceComponent(TSubclassOf<UActorComponent> ActorComponentClass, AActor* OwnerActor);

	UFUNCTION(BlueprintCallable)
	static void SetSoundClass(USoundBase* SoundBase, USoundClass* SoundClass);

	UFUNCTION(BlueprintCallable)
	static void MadeAssetDirty(UObject* Object);

	UFUNCTION(BlueprintCallable)
	static void SetSoundAttenuation(USoundCue* SoundCue, USoundAttenuation* SoundAttenuation);

	UFUNCTION(BlueprintCallable)
	static void CreatePhysicalProxy(UGeometryCollectionComponent* GeometryCollectionComponent);

	UFUNCTION(BlueprintCallable)
	static UObject* GetCDOObject(TSubclassOf<UObject> Object);

	UFUNCTION(BlueprintCallable)
	static void DirectlyDestroyComponent(UActorComponent* ActorComponentToDestroy);

	UFUNCTION(BlueprintCallable)
	static void DestroyController(AController* ControllerToDestroy);

	UFUNCTION(BlueprintCallable)
	static void SetGravityToCharacterMovement(UCharacterMovementComponent* CharacterMovementComponent, FVector NewGravity);

	UFUNCTION(BlueprintCallable)
	static FVector GetGravityFromCharacterMovement(UCharacterMovementComponent* CharacterMovementComponent);

	UFUNCTION(BlueprintCallable)
	static UActorComponent* GetDefaultComponentByActorClass(TSubclassOf<UActorComponent> ClassActorComponent, TSubclassOf<AActor> ClassActor);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FVector2D GetMinVector(const TArray<FVector2D>& Vectors);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FVector2D GetMaxVector(const TArray<FVector2D>& Vectors);

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable)
	static UStaticMesh* ConvertProceduralToStatic(UProceduralMeshComponent* ProceduralMesh, const FString& MeshName, const FString& PackagePath);
#endif

	/**
	 * Найти все Actor со Spline, чьи сплайны пересекают Bound Box.
	 * @param WorldContextObject Контекстный объект.
	 * @param BoundBox Ограничивающий бокс для проверки.
	 * @param OutActors Результат – список акторов со сплайнами, чьи сплайны пересекают Bound Box.
	 */
	UFUNCTION(BlueprintCallable, Category = "Spline", meta = (WorldContext = "WorldContextObject"))
	static void GetActorsWithSplinesOverlappingBox(const UObject* WorldContextObject,const FBox& BoundBox,TArray<AActor*>& OutActors, TSubclassOf<AActor> ActorClass, bool EqualExactly = true);
	UFUNCTION(BlueprintCallable, Category = "Spline", meta = (WorldContext = "WorldContextObject"))
	static void GetActorsWithSplinesOverlappingBoxByIntersect(const UObject* WorldContextObject, const FBox& BoundBox, TArray<AActor*>& OutActors, TSubclassOf<AActor> ActorClass, bool EqualExactly = true);

	UFUNCTION(BlueprintCallable, Category = "Spline", meta = (WorldContext = "WorldContextObject"))
	static void RefreshPCGComponent(UPCGComponent* PCGComponent, EPCGChangeType ChangeType = EPCGChangeType::None, bool bCancelExistingRefresh = false);

	UFUNCTION(BlueprintCallable, Category = "Spline", meta = (WorldContext = "WorldContextObject"))
	static void SetDirtyGenerated(UPCGComponent* PCGComponent);

	UFUNCTION(BlueprintCallable, Category = "Spline", meta = (WorldContext = "WorldContextObject"))
	static void PostEditMoveUpdate(UPCGComponent* PCGComponent, EPCGChangeType ChangeType, bool bCancelExistingRefresh);

	UFUNCTION(BlueprintCallable, Category = "Spline", meta = (WorldContext = "WorldContextObject"))
	static void CleanUpPCGComponent(UPCGComponent* PCGComponent);

	UFUNCTION(BlueprintCallable, Category = "Spline", meta = (WorldContext = "WorldContextObject"))
	static void ComponentChangedPCGComponent(UPCGComponent* PCGComponent);

	UFUNCTION(BlueprintCallable)
	static TArray<AActor*> FilterActorsByTag(const TArray<AActor*>& Actors, const FName& Tag);

	UFUNCTION(BlueprintCallable)
	static UObject* LoadObjectFromSoftPath(const FSoftObjectPath& SoftPath);

	UFUNCTION(BlueprintCallable)
	static bool IsPointInsideSpline(const TArray<FVector>& SplinePoints, const FVector& Point);

	UFUNCTION(BlueprintCallable)
	static void GetOriginalSplinePoint(UPCGSplineData* PCGSplineData, TArray<FVector>& Positions);

	UFUNCTION(BlueprintCallable)
	static void GetWorldOriginalSplinePoint(UPCGSplineData* PCGSplineData, TArray<FVector>& Positions);

	UFUNCTION(BlueprintCallable)
	static bool IsPointInTriangle2D(const FVector2D& P, const FVector2D& A, const FVector2D& B, const FVector2D& C);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UWorldPartitionLoaderWrapper* LoadCell(const UObject* WorldContextObject, FBox SelectionBox);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static void UnloadCell(const UObject* WorldContextObject, UWorldPartitionLoaderWrapper* WorldPartitionEditorLoaderAdapter);

	UFUNCTION(BlueprintCallable)
	static void RegisterActorInWorldPartition(AActor* Actor);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void InitializeWorldPartition(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void DeinitializeWorldPartition(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void ReInitWorld(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void ReBuildWorld(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable)
	static void ForceGarbageCollection();

	UFUNCTION(BlueprintCallable)
	static void SaveCurrentLevelProxy();

	UFUNCTION(BlueprintCallable)
	static void ClearAllWorldPartitionUnloadActors();

	UFUNCTION(BlueprintCallable)
	static bool CheckIsWorldPartitionEnable();


	template <typename T>
	static void CopyObjects(TArray<T*>& Destination, const TArray<T*>& Source, UObject* Outer)
	{
		Destination.Empty();

		for (T* Brush : Source)
		{
			if (IsValid(Brush))
			{
				T* NewBrush = DuplicateObject<T>(Brush, Outer);
				Destination.Add(NewBrush);
			}
		}
	}

	UFUNCTION(BlueprintCallable)
	static FString RemoveProblems(const FString& Input)
	{
		FString Result;
		bool bCapitalizeNext = true; // Флаг для заглавной буквы

		for (TCHAR Char : Input)
		{
			if (FChar::IsAlnum(Char))
			{
				// Если флаг установлен, делаем букву заглавной
				if (bCapitalizeNext)
				{
					Char = FChar::ToUpper(Char);
					bCapitalizeNext = false; // Сбрасываем флаг
				}
				Result.AppendChar(Char);
			}
			else
			{
				bCapitalizeNext = true; // Следующий символ должен быть заглавным
			}
		}
		return Result;
	}

};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEditorTimerObjectTrackDelegate);


UCLASS(Blueprintable, BlueprintType)
class ORIENTPROJECT_API UEditorTimerObjectTrack : public UObject
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "Editor", CallInEditor)
	void Initialization(float Time);

	UFUNCTION(BlueprintCallable, Category = "Editor", CallInEditor)
	void EndInitialization();

	UPROPERTY(BlueprintAssignable)
	FEditorTimerObjectTrackDelegate EditorTimerObjectTrackDelegate;

private:
	FTSTicker::FDelegateHandle TimerHandle;
};



class AActor;

UCLASS(Blueprintable, BlueprintType)
class ORIENTPROJECT_API AEditorTrackingActor : public AActor
{
	GENERATED_BODY()
public:
	virtual void OnConstruction(const FTransform& Transform) override;


	UFUNCTION(BlueprintImplementableEvent, Category = "Editor", CallInEditor)
	void OnConstructionStart();

	UFUNCTION(BlueprintImplementableEvent, Category = "Editor", CallInEditor)
	void OnConstructionFinished();

	UFUNCTION(BlueprintImplementableEvent, Category = "Editor", CallInEditor)
	void EditorDestroyed();

	virtual void Destroyed() override;

private:
	FTSTicker::FDelegateHandle TimerHandle;
	float DelayTime = 0.5f; 

	void StartConstructionTimer();
};

