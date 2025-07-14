


#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Framework/Application/IInputProcessor.h"
#include "LandscapeProxy.h"
#include "Engine/GameViewportClient.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Containers/Ticker.h"
#include "HelperBlueprintFunctionLibrary.generated.h"

/**
 * 
 */

class FReciveInputProcessor;
class UReciveInputObject;
class USoundCue;
class ALandscapeProxy;
class UGameViewportClient;
class UWaterBodyComponent;
class AWaterBody;
class UWaterSplineComponent;
class FMemoryWriter;
class FMemoryReader;

struct FMagicSaveGameArchive : public FObjectAndNameAsStringProxyArchive
{
	FMagicSaveGameArchive(FArchive& InInnerArchive, bool bInLoadIfFindFails)
		: FObjectAndNameAsStringProxyArchive(InInnerArchive, bInLoadIfFindFails)
	{
		ArIsSaveGame = true;
	}
};
class AInstancedFoliageActor;


UENUM()
enum ETestEnum : uint8
{
	TestEnum, 
	TestEnum2
};



UENUM(BlueprintType)
enum class EEffectApplicationMode : uint8
{
	Add UMETA(DisplayName = "Add"),
	Set UMETA(DisplayName = "Set"),
	Multiply UMETA(DisplayName = "Multiply")
};

template <typename T>
struct FEffectData
{
	T Value;
	EEffectApplicationMode ApplicationMode;
	int Priority = 0;

	FEffectData() {}
	FEffectData(T InValue, EEffectApplicationMode InApplicationMode, int InPriority)
		: Value(InValue), ApplicationMode(InApplicationMode), Priority(InPriority) {
	}

	bool operator==(const FEffectData& Other) const
	{
		return Value == Other.Value
			&& ApplicationMode == Other.ApplicationMode
			&& Priority == Other.Priority;
	}
};

template <typename T>
struct FEffectStack
{
	TMap<FName, FEffectData<T>> EffectMap;
	TArray<FEffectData<T>> SortedEffects;
	T DefaultValue;

	void AddEffect(FName Key, T Value, EEffectApplicationMode Mode, int Priority = 0)
	{
		FEffectData<T> NewEffect(Value, Mode, Priority);

		EffectMap.Add(Key, NewEffect);

		int Count = 0;

		for (FEffectData<T> EffectData : SortedEffects)
		{
			if (EffectData.Priority > Priority)
			{
				SortedEffects.Insert(NewEffect, FMath::Max(Count, 0));
				return;
			}
			Count++;
		}
		SortedEffects.Add(NewEffect);
	}

	void RemoveEffect(FName Key)
	{
		if (FEffectData<T>* FoundEffect = EffectMap.Find(Key))
		{
			SortedEffects.Remove(*FoundEffect);
			EffectMap.Remove(Key);
		}
	}

	T CalculateTotalEffect() const
	{
		T Result = DefaultValue;
		for (const FEffectData<T>& Effect : SortedEffects)
		{
			switch (Effect.ApplicationMode)
			{
			case EEffectApplicationMode::Add:
				if constexpr (std::is_same_v<T, bool>)
				{
					Result = Result || Effect.Value;
				}
				else
				{
					Result += Effect.Value;
				}
				break;
			case EEffectApplicationMode::Set:
				Result = Effect.Value;
				break;
			case EEffectApplicationMode::Multiply:
				if constexpr (std::is_same_v<T, bool>)
				{
					Result = Result && Effect.Value;
				}
				else
				{
					Result *= Effect.Value;
				}
				break;
			}
		}
		return Result;
	}

};


USTRUCT(BlueprintType)
struct FFloatEffectStackBlueprint
{
	GENERATED_BODY()

public:

	FEffectStack<float> EffectStack;
};


USTRUCT(BlueprintType)
struct FIntEffectStackBlueprint
{
	GENERATED_BODY()

public:

	FEffectStack<int> EffectStack;
};

USTRUCT(BlueprintType)
struct FBoolEffectStackBlueprint
{
	GENERATED_BODY()

public:

	FEffectStack<bool> EffectStack;
};


USTRUCT(BlueprintType)
struct FLinearColorEffectStackBlueprint
{
	GENERATED_BODY()

public:

	FEffectStack<FLinearColor> EffectStack;
};


UCLASS()
class UEffectStackFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Float")
	static void AddFloatValueToStack(UPARAM(ref) FFloatEffectStackBlueprint& Stack, FName EffectName, float Value, EEffectApplicationMode Mode, int Priority);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Float")
	static void RemoveFloatValueFromStack(UPARAM(ref) FFloatEffectStackBlueprint& Stack, FName EffectName);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Float")
	static float CalculateFloatTotalEffect(UPARAM(ref) FFloatEffectStackBlueprint& Stack);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Float")
	static bool HasFloatEffect(const FFloatEffectStackBlueprint& Stack, FName EffectName);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Float")
	static bool GetFloatEffectValue(const FFloatEffectStackBlueprint& Stack, FName EffectName, float& OutValue, EEffectApplicationMode& OutMode);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Float")
	static void SetFloatDefaultVariableStack(UPARAM(ref)FFloatEffectStackBlueprint& Stack, float DefaultStackVariable);


	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Int")
	static void AddIntValueToStack(UPARAM(ref) FIntEffectStackBlueprint& Stack, FName EffectName, int Value, EEffectApplicationMode Mode, int Priority);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Int")
	static void RemoveIntValueFromStack(UPARAM(ref) FIntEffectStackBlueprint& Stack, FName EffectName);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Int")
	static int CalculateIntTotalEffect(const FIntEffectStackBlueprint& Stack);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Int")
	static bool HasIntEffect(const FIntEffectStackBlueprint& Stack, FName EffectName);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Int")
	static bool GetIntEffectValue(const FIntEffectStackBlueprint& Stack, FName EffectName, int& OutValue, EEffectApplicationMode& OutMode);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Int")
	static void SetIntDefaultVariableStack(UPARAM(ref) FIntEffectStackBlueprint& Stack, int DefaultStackVariable);


	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Color")
	static void AddColorValueToStack(UPARAM(ref) FLinearColorEffectStackBlueprint& Stack, FName EffectName, FLinearColor Value, EEffectApplicationMode Mode, int Priority);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Color")
	static void RemoveColorValueFromStack(UPARAM(ref) FLinearColorEffectStackBlueprint& Stack, FName EffectName);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Color")
	static FLinearColor CalculateColorTotalEffect(const FLinearColorEffectStackBlueprint& Stack);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Color")
	static bool HasColorEffect(const FLinearColorEffectStackBlueprint& Stack, FName EffectName);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Color")
	static bool GetColorEffectValue(const FLinearColorEffectStackBlueprint& Stack, FName EffectName, FLinearColor& OutValue, EEffectApplicationMode& OutMode);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Color")
	static void SetColorDefaultVariableStack(UPARAM(ref) FLinearColorEffectStackBlueprint& Stack, FLinearColor DefaultStackVariable);


	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Bool")
	static void AddBoolValueToStack(UPARAM(ref) FBoolEffectStackBlueprint& Stack, FName EffectName, bool Value, EEffectApplicationMode Mode, int Priority);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Bool")
	static void RemoveBoolValueFromStack(UPARAM(ref) FBoolEffectStackBlueprint& Stack, FName EffectName);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Bool")
	static bool CalculateBoolTotalEffect(const FBoolEffectStackBlueprint& Stack);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Bool")
	static bool HasBoolEffect(const FBoolEffectStackBlueprint& Stack, FName EffectName);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Bool")
	static bool GetBoolEffectValue(const FBoolEffectStackBlueprint& Stack, FName EffectName, bool& OutValue, EEffectApplicationMode& OutMode);

	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Bool")
	static void SetBoolDefaultVariableStack(UPARAM(ref) FBoolEffectStackBlueprint& Stack, bool DefaultStackVariable);


};

UENUM(BlueprintType)
enum class EGraphicsRHI : uint8
{
	DEFAULT	UMETA(DisplayName = "Default"),
	RHI_DX11	UMETA(DisplayName = "DirectX 11"),
	RHI_DX12	UMETA(DisplayName = "DirectX 12"),
	RHI_VULKAN	UMETA(DisplayName = "Vulkan")
};

UCLASS()
class UHelperBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
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
	static TArray<uint8> SerializeObject(UObject* Object);

	UFUNCTION(BlueprintCallable)
	static void DeserializeObject(UObject* Object, TArray<uint8> ByteArray);

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
#if WITH_EDITOR
	UFUNCTION(BlueprintCallable)
	static bool WorldPositionToLandscapePosition(FVector Position, ALandscape* Landscape, FVector2D& AnswerPosition);

	UFUNCTION(BlueprintCallable)
	static bool LandscapePositionToWorldPosition(FVector2D LocalPosition, ALandscape* Landscape, FVector& WorldPosition);

	UFUNCTION(BlueprintCallable)
	static void SetEnableNaniteStaticMesh(UStaticMesh* StaticMesh, bool NaniteEnable = false);
#endif //WITH_EDITOR

	UFUNCTION(BlueprintCallable)
	static int GetMaxEnumValueSimple(UEnum* Enum);

	UFUNCTION(BlueprintCallable)
	static UEnum* GetEnumFieldObject(const FString enumName);

	UFUNCTION(BlueprintCallable, Category = "GraphicsRHIManager")
	static void SetGraphicsRHI(EGraphicsRHI GraphicsRHI);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GraphicsRHIManager")
	static EGraphicsRHI GetCurrentGraphicsRHI();

	UFUNCTION(BlueprintCallable)
	static void SetupMaxFPS(int MaxFPS);

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "BaseClass"))
	static  TArray<TSubclassOf<UObject>> GetAllDerivedClasses(TSubclassOf<UObject> BaseClass);

	UFUNCTION(BlueprintCallable)
	static bool IsLastInputIsGamepad();

	UFUNCTION(BlueprintCallable)
	static void ForceUpdateWater(AWaterBody* WaterBodyActor);

	UFUNCTION(BlueprintCallable)
	static void SetRiverSplinePointParameters(UWaterSplineComponent* WaterSplineComponent, int PointIndex, float Depth, float RiverWidth, float Velocity);

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "SceneComponentClass"))
	static UActorComponent* AddComponentToActor(AActor* Actor, TSubclassOf<UActorComponent> SceneComponentClass);

	UFUNCTION(BlueprintCallable, Category = "Input")
	static TArray<FKey> GetAllKeys();

	UFUNCTION(BlueprintCallable, Category = "Input")
	static FKey ConvertNameToKey(FName KeyName);

	UFUNCTION(BlueprintCallable)
	static FString SplitByUpperCase(const FString InputString);

	UFUNCTION(BlueprintCallable)
	static TArray<FString> OpenFileDialog(FString FileTypes = TEXT("All Files (*.*)|*.*"));

	UFUNCTION(BlueprintCallable)
	static bool IsPointInsideSpline2D(USplineComponent* Spline, const FVector2D& TestPoint);

	UFUNCTION(BlueprintCallable)
	static bool ExecuteProc(FString ExePath, FString Params, FString WorkingDirectory);

	UFUNCTION(BlueprintCallable, Category = "ExternalProcess")
	static UProcessReaderProxy* ExecuteProcWithOutput(FString ExePath, FString Params, FString WorkingDirectory);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static ANavMeshBoundsVolume* SpawnNavMeshBoundsVolume(const UObject* WorldContextObject, FVector Position);

	UFUNCTION(BlueprintCallable)
	static void NavUpdate(USceneComponent* SceneComponent);

};

UCLASS()
class UUpgradeViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;

	bool bIsLastInputIsGamepad = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProcessOutput, const FString&, Output);

UCLASS(BlueprintType)
class UProcessReaderProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnProcessOutput OnOutputReceived;

	void Init(void* InReadPipe, FProcHandle InProcHandle);

	virtual void BeginDestroy() override;

private:
	void* ReadPipe = nullptr;
	FProcHandle ProcHandle;
	FTSTicker::FDelegateHandle TickHandle;

	bool Tick(float DeltaTime);
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNewActorDelegate, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNewActorWithComponentsDelegate, AActor*, Actor);

UCLASS(Blueprintable)
class UReciveWorldObject : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void Setup(AActor* NewWorldContextObject);

	UFUNCTION()
	void BindCallNewActorDelegate(AActor* Actor);

	UFUNCTION()
	void BindCallNewActorWithComponentsDelegate(AActor* Actor);

	UPROPERTY()
	AActor* WorldContextObject;

	FDelegateHandle SpawnNewActorDelegateHandle;

	FDelegateHandle SpawnNewActorWithComponentsDelegateHandle;

	UFUNCTION(BlueprintCallable)
	void StopSetup();

	UPROPERTY(BlueprintAssignable)
	FNewActorDelegate NewActorDelegate;

	UPROPERTY(BlueprintAssignable)
	FNewActorWithComponentsDelegate NewActorWithComponentsDelegate;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKeyPressDelegate, FKey, Key);

UCLASS(Blueprintable)
class UReciveInputObject : public UObject
{
	GENERATED_BODY()

public:

	~UReciveInputObject();

	UPROPERTY(BlueprintReadOnly)
	bool StartSetup = false;

	UFUNCTION(BlueprintCallable)
	void Setup();

	UFUNCTION(BlueprintCallable)
	void StopSetup();

	UPROPERTY(BlueprintAssignable)
	FKeyPressDelegate KeyPressDelegate;

	TSharedPtr<FReciveInputProcessor> ReciveInputProcessor;

};


class FReciveInputProcessor : public IInputProcessor
{
public:

	FReciveInputProcessor() {};

	UReciveInputObject* ReciveInputObject;

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;

	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
};
