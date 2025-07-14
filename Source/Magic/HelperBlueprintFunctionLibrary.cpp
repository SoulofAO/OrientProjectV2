#include "HelperBlueprintFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "AssetRegistry/ARFilter.h"
#include "Engine/World.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundCue.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "LandscapeLayerInfoObject.h"
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "Engine.h"
#include "LandscapeSplineSegment.h"
#include "Raster.h"
#include "LandscapeSubsystem.h"
#include "LandscapeProxy.h"
#include "Engine/GameViewportClient.h"
#include "WaterBodyComponent.h"
#include "WaterBodyActor.h"
#include "WaterSplineComponent.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Containers/Ticker.h"

void UHelperBlueprintFunctionLibrary::SetSoundClass(USoundBase* SoundBase, USoundClass* SoundClass)
{
	SoundBase->SoundClassObject = SoundClass;
}

void UHelperBlueprintFunctionLibrary::MadeAssetDirty(UObject* Object)
{
	Object->MarkPackageDirty();
}

void UHelperBlueprintFunctionLibrary::SetSoundAttenuation(USoundCue* SoundCue, USoundAttenuation* SoundAttenuation)
{
	SoundCue->AttenuationSettings = SoundAttenuation;
}

void UHelperBlueprintFunctionLibrary::CreatePhysicalProxy(UGeometryCollectionComponent* GeometryCollectionComponent)
{
	GeometryCollectionComponent->SetDynamicState(Chaos::EObjectStateType::Dynamic);
}

UObject* UHelperBlueprintFunctionLibrary::GetCDOObject(TSubclassOf<UObject> Object)
{
	return Object->GetDefaultObject();
}

TArray<uint8> UHelperBlueprintFunctionLibrary::SerializeObject(UObject* Object)
{
	TArray<uint8> ResultByte;

	FMemoryWriter MemoryWriter(ResultByte, true);

	FMagicSaveGameArchive Ar(MemoryWriter, false);

	Object->Serialize(Ar);

	return  ResultByte;
}

void UHelperBlueprintFunctionLibrary::DeserializeObject(UObject* Object, TArray<uint8> ByteArray)
{
	FMemoryReader MemoryReader(ByteArray, false);
	FMagicSaveGameArchive Ar(MemoryReader, false);
	Object->Serialize(Ar);
}

void UHelperBlueprintFunctionLibrary::DirectlyDestroyComponent(UActorComponent* ActorComponentToDestroy)
{
	if (IsValid(ActorComponentToDestroy))
	{
		ActorComponentToDestroy->DestroyComponent();
	}
}

void UHelperBlueprintFunctionLibrary::DestroyController(AController* ControllerToDestroy)
{
	if (IsValid(ControllerToDestroy))
	{
		ControllerToDestroy->Destroy(true,true);
	}
}


void UHelperBlueprintFunctionLibrary::SetGravityToCharacterMovement(UCharacterMovementComponent* CharacterMovementComponent, FVector NewGravity)
{
	CharacterMovementComponent->SetGravityDirection(NewGravity);
}

FVector UHelperBlueprintFunctionLibrary::GetGravityFromCharacterMovement(UCharacterMovementComponent* CharacterMovementComponent)
{
	return CharacterMovementComponent->GetGravityDirection();
}

UActorComponent* UHelperBlueprintFunctionLibrary::GetDefaultComponentByActorClass(TSubclassOf<UActorComponent> ClassActorComponent, TSubclassOf<AActor> ClassActor)
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
#if WITH_EDITOR
 bool UHelperBlueprintFunctionLibrary::WorldPositionToLandscapePosition(FVector Position,ALandscape* Landscape, FVector2D& AnswerPosition)
{

	 if (!Landscape)
	 {
		 return false;
	 }
	 FIntRect LandscapeExtent;
	 Landscape->GetLandscapeInfo()->GetLandscapeExtent(LandscapeExtent.Min.X, LandscapeExtent.Min.Y, LandscapeExtent.Max.X, LandscapeExtent.Max.Y);
	 float DistanceToProxy = FLT_MAX;
	 ALandscapeProxy* AnswerProxy = nullptr;
	 for (TActorIterator<ALandscapeProxy> It(Landscape->GetWorld(), ALandscapeProxy::StaticClass()); It; ++It)
	 {
		 ALandscapeProxy* Proxy = *It;
		 if (DistanceToProxy > FVector::Distance(Proxy->GetActorLocation(), Position))
		 {
			 DistanceToProxy = FVector::Distance(Proxy->GetActorLocation(), Position);
			 AnswerProxy = Proxy;
		 }
	 }
	 if (!AnswerProxy)
	 {
		 return false;
	 }
	 AnswerPosition = FVector2D(AnswerProxy->LandscapeActorToWorld().InverseTransformPosition(Position).X, AnswerProxy->LandscapeActorToWorld().InverseTransformPosition(Position).Y);
	 if (AnswerPosition.X > LandscapeExtent.Min.X && AnswerPosition.X < LandscapeExtent.Max.X && AnswerPosition.Y > LandscapeExtent.Min.Y && AnswerPosition.Y < LandscapeExtent.Max.Y)
	 {
		 return true;
	 }
	 return false;
 }

 bool UHelperBlueprintFunctionLibrary::LandscapePositionToWorldPosition(FVector2D LocalPosition, ALandscape* Landscape, FVector& WorldPosition)
 {
	 if (!Landscape)
	 {
		 return false;
	 }

	 ALandscapeProxy* AnswerProxy = nullptr;
	 float DistanceToProxy = FLT_MAX;

	 // Найти ближайший ALandscapeProxy
	 for (TActorIterator<ALandscapeProxy> It(Landscape->GetWorld(), ALandscapeProxy::StaticClass()); It; ++It)
	 {
		 ALandscapeProxy* Proxy = *It;
		 float Distance = FVector::Distance(Proxy->GetActorLocation(), Landscape->GetActorLocation());
		 if (Distance < DistanceToProxy)
		 {
			 DistanceToProxy = Distance;
			 AnswerProxy = Proxy;
		 }
	 }

	 if (!AnswerProxy)
	 {
		 return false;
	 }

	 // Преобразуем локальную позицию обратно в мировую
	 WorldPosition = AnswerProxy->LandscapeActorToWorld().TransformPosition(FVector(LocalPosition.X, LocalPosition.Y, 0.0f));

	 return true;
 }
 void UHelperBlueprintFunctionLibrary::SetEnableNaniteStaticMesh(UStaticMesh* StaticMesh, bool NaniteEnable)
 {
	 StaticMesh->Modify();
	 StaticMesh->NaniteSettings.bEnabled = NaniteEnable;
	 FProperty* ChangedProperty = FindFProperty<FProperty>(UStaticMesh::StaticClass(), GET_MEMBER_NAME_CHECKED(UStaticMesh, NaniteSettings));
	 FPropertyChangedEvent Event(ChangedProperty);
	 StaticMesh->PostEditChangeProperty(Event);
 }
#endif //WITH_EDITOR


 int UHelperBlueprintFunctionLibrary::GetMaxEnumValueSimple(UEnum* Enum)
 {
	 return Enum->GetMaxEnumValue();
 }

 UEnum* UHelperBlueprintFunctionLibrary::GetEnumFieldObject(const FString enumName)
 {
	 return FindObject<UEnum>(ANY_PACKAGE, *enumName); 
 }



 void UHelperBlueprintFunctionLibrary::SetGraphicsRHI(EGraphicsRHI GraphicsRHI)
 {

	 FString TargetPlataform = UGameplayStatics::GetPlatformName();
	 FString DefaultGraphicsRHI;
	 FString RHI_DX11(TEXT("DefaultGraphicsRHI_DX11"));
	 FString RHI_DX12(TEXT("DefaultGraphicsRHI_DX12"));
	 FString RHI_VULKAN(TEXT("DefaultGraphicsRHI_Vulkan"));

	 if (TargetPlataform == "Windows")
	 {
		 GConfig->GetString(TEXT("/Script/WindowsTargetPlatform.WindowsTargetSettings"), TEXT("DefaultGraphicsRHI"), DefaultGraphicsRHI, GEngineIni);
		 switch (GraphicsRHI)
		 {
		 case EGraphicsRHI::RHI_DX11:
			 if (DefaultGraphicsRHI != "DefaultGraphicsRHI_DX11")
			 {
				 GConfig->SetString(TEXT("/Script/WindowsTargetPlatform.WindowsTargetSettings"), TEXT("DefaultGraphicsRHI"), *RHI_DX11, GEngineIni);
				 GConfig->Flush(true, GEngineIni);
				 return;
			 }
			 return;

		 case EGraphicsRHI::RHI_DX12:
			 if (DefaultGraphicsRHI != "DefaultGraphicsRHI_DX12")
			 {
				 GConfig->SetString(TEXT("/Script/WindowsTargetPlatform.WindowsTargetSettings"), TEXT("DefaultGraphicsRHI"), *RHI_DX12, GEngineIni);
				 GConfig->Flush(true, GEngineIni);
				 return;
			 }
			 return;

		 case EGraphicsRHI::RHI_VULKAN:
			 if (DefaultGraphicsRHI != "DefaultGraphicsRHI_Vulkan")
			 {
				 GConfig->SetString(TEXT("/Script/WindowsTargetPlatform.WindowsTargetSettings"), TEXT("DefaultGraphicsRHI"), *RHI_VULKAN, GEngineIni);
				 GConfig->Flush(true, GEngineIni);
				 return;
			 }
			 return;
		 }

	 }
	 return;
 }

 EGraphicsRHI UHelperBlueprintFunctionLibrary::GetCurrentGraphicsRHI()
 {
	 FString TargetPlataform = UGameplayStatics::GetPlatformName();
	 FString DefaultGraphicsRHI;

	 GConfig->GetString(TEXT("/Script/WindowsTargetPlatform.WindowsTargetSettings"), TEXT("DefaultGraphicsRHI"), DefaultGraphicsRHI, GEngineIni);

	 if (DefaultGraphicsRHI == "DefaultGraphicsRHI_DX11")
	 {
		 return EGraphicsRHI::RHI_DX11;
	 }
	 else if (DefaultGraphicsRHI == "DefaultGraphicsRHI_DX12")
	 {
		 return EGraphicsRHI::RHI_DX12;
	 }
	 return EGraphicsRHI::RHI_VULKAN;
 }

 void UHelperBlueprintFunctionLibrary::SetupMaxFPS(int MaxFPS)
 {
	 GEngine->SetMaxFPS(MaxFPS);
 }

 TArray<TSubclassOf<UObject>> UHelperBlueprintFunctionLibrary::GetAllDerivedClasses(TSubclassOf<UObject> BaseClass)
 {
	 TArray<TSubclassOf<UObject>> DerivedClasses;

	 // Проходим через все загруженные классы
	 for (TObjectIterator<UClass> It; It; ++It)
	 {
		 UClass* CurrentClass = *It;

		 // Проверяем, является ли CurrentClass наследником BaseClass
		 if (CurrentClass && CurrentClass->IsChildOf(BaseClass.Get()) && CurrentClass != BaseClass.Get())
		 {
			 DerivedClasses.Add(CurrentClass);
		 }
	 }

	 return DerivedClasses;
 }

 bool UHelperBlueprintFunctionLibrary::IsLastInputIsGamepad()
 {
	 if (GEngine && GEngine->GameViewport)
	 {
		 UUpgradeViewportClient* UpgradeViewport = Cast<UUpgradeViewportClient>(GEngine->GameViewport);
		 if (UpgradeViewport)
		 {
			 return UpgradeViewport->bIsLastInputIsGamepad;
		 }
	 }
	 return false;
 }

 void UHelperBlueprintFunctionLibrary::ForceUpdateWater(AWaterBody* WaterBodyActor)
 {
#if WITH_EDITOR
	 WaterBodyActor->PostEditMove(true);
#endif
 }

 void UHelperBlueprintFunctionLibrary::SetRiverSplinePointParameters(UWaterSplineComponent* WaterSplineComponent, int PointIndex, float Depth, float RiverWidth, float Velocity)
 {
	 UWaterSplineMetadata* Metadata = Cast<UWaterSplineMetadata>(WaterSplineComponent->GetSplinePointsMetadata());
	 if (Metadata)
	 {
		 FWaterSplineCurveDefaults PreviousDefaults;
		 FWaterSplineCurveDefaults NewDefaults;
		 NewDefaults.DefaultWidth = RiverWidth;
		 NewDefaults.DefaultDepth = Depth;
		 NewDefaults.DefaultVelocity = Velocity;
		 Metadata->PropagateDefaultValue(PointIndex, PreviousDefaults, NewDefaults);
	 }
 }
 UActorComponent* UHelperBlueprintFunctionLibrary::AddComponentToActor(AActor* Actor, TSubclassOf<UActorComponent> ActorComponentClass)
 {
	 if (!Actor)
	 {
		 UE_LOG(LogTemp, Warning, TEXT("Actor is null."));
		 return nullptr;
	 }

	 if (!ActorComponentClass)
	 {
		 UE_LOG(LogTemp, Warning, TEXT("ActorComponentClass is null."));
		 return nullptr;
	 }

	 UActorComponent* NewComponent = NewObject<UActorComponent>(Actor, ActorComponentClass);
	 if (!NewComponent)
	 {
		 UE_LOG(LogTemp, Warning, TEXT("Failed to create component."));
		 return nullptr;
	 }

	 NewComponent->SetFlags(RF_Transactional);

	 USceneComponent* SceneComponent = Cast<USceneComponent>(NewComponent);
	 if (SceneComponent)
	 {
		 if (!Actor->GetRootComponent())
		 {
			 Actor->SetRootComponent(SceneComponent);
		 }
		 else
		 {
			 SceneComponent->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		 }
	 }

	 NewComponent->RegisterComponent();
	 Actor->AddInstanceComponent(NewComponent);

	 UE_LOG(LogTemp, Log, TEXT("Component of class %s successfully added to actor %s."),
		 *ActorComponentClass->GetName(), *Actor->GetName());

	 return NewComponent;
 }

 TArray<FKey> UHelperBlueprintFunctionLibrary::GetAllKeys()
 {
	 TArray<FKey> Keys;
	 EKeys::GetAllKeys(Keys);
	 return Keys;
 }

 FKey UHelperBlueprintFunctionLibrary::ConvertNameToKey(FName KeyName)
 {
	 return FKey(KeyName);
 }

 FString UHelperBlueprintFunctionLibrary::SplitByUpperCase(const FString InputString)
 {
	 FString AnswerString = "";
	 TArray<FString> Words;
	 FString CurrentWord;

	 for (int32 Index = 0; Index < InputString.Len(); ++Index)
	 {
		 TCHAR CurrentChar = InputString[Index];

		 if (FChar::IsUpper(CurrentChar) && !CurrentWord.IsEmpty())
		 {
			 Words.Add(CurrentWord);
			 CurrentWord.Empty();
		 }

		 CurrentWord.AppendChar(CurrentChar);
	 }

	 if (!CurrentWord.IsEmpty())
	 {
		 Words.Add(CurrentWord);
	 }

	 AnswerString = Words[0];
	 for (int32 Index = 1; Index < Words.Num(); ++Index)
	 {
		 AnswerString = AnswerString + " " + Words[Index];
	 }
	 return AnswerString;
 }

 TArray<FString> UHelperBlueprintFunctionLibrary::OpenFileDialog(FString FileTypes)
 {
	 IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	 if (DesktopPlatform)
	 {
		 FString OutFileName;
		 TArray<FString> OutFiles;
		 const FString Title = TEXT("ChooseFile");
		 const FString DefaultPath = FPaths::ProjectDir();

		 bool bOpened = DesktopPlatform->OpenFileDialog(
			 FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			 Title,
			 DefaultPath,
			 TEXT(""),
			 FileTypes,
			 EFileDialogFlags::None,
			 OutFiles
		 );

		 if (bOpened && OutFiles.Num() > 0)
		 {
			 OutFileName = OutFiles[0];
			 UE_LOG(LogTemp, Log, TEXT("ChoosenFile: %s"), *OutFileName);
			 return OutFiles;
		 }
	 }
	 return TArray<FString>();
 }

 bool UHelperBlueprintFunctionLibrary::IsPointInsideSpline2D(USplineComponent* Spline, const FVector2D& TestPoint)
 {
	 int32 NumPoints = Spline->GetNumberOfSplinePoints();
	 if (NumPoints < 3) return false;

	 int32 Crossings = 0;

	 for (int32 i = 0; i < NumPoints; ++i)
	 {
		 FVector2D A = FVector2D(Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
		 FVector2D B = FVector2D(Spline->GetLocationAtSplinePoint((i + 1) % NumPoints, ESplineCoordinateSpace::World));

		 if (A.Y > B.Y) Swap(A, B);

		 if (TestPoint.Y > A.Y && TestPoint.Y <= B.Y &&
			 (TestPoint.X < FMath::Lerp(A.X, B.X, (TestPoint.Y - A.Y) / (B.Y - A.Y + KINDA_SMALL_NUMBER))))
		 {
			 Crossings++;
		 }
	 }

	 return Crossings % 2 == 1;
 }

 bool UHelperBlueprintFunctionLibrary::ExecuteProc(FString ExePath, FString Params, FString WorkingDirectory)
 {
	 FProcHandle ProcHandle = FPlatformProcess::CreateProc(
		 *ExePath,
		 *Params,
		 true,
		 false,
		 false,
		 nullptr,
		 0,
		 *WorkingDirectory,
		 nullptr
	 );

	 if (ProcHandle.IsValid())
	 {
		 return true;
	 }
	 else
	 {
		 return false;
	 }
 }


 UProcessReaderProxy* UHelperBlueprintFunctionLibrary::ExecuteProcWithOutput(FString ExePath, FString Params, FString WorkingDirectory)
 {
	 void* ReadPipe = nullptr;
	 void* WritePipe = nullptr;
	 FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

	 FProcHandle ProcHandle = FPlatformProcess::CreateProc(
		 *ExePath,
		 *Params,
		 true,
		 false,
		 false,
		 nullptr,
		 0,
		 *WorkingDirectory,
		 WritePipe
	 );

	 if (!ProcHandle.IsValid())
	 {
		 FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
		 return nullptr;
	 }

	 UProcessReaderProxy* Proxy = NewObject<UProcessReaderProxy>();
	 Proxy->AddToRoot(); 
	 Proxy->Init(ReadPipe, ProcHandle);
	 return Proxy;
 }

 void UProcessReaderProxy::Init(void* InReadPipe, FProcHandle InProcHandle)
 {
	 ReadPipe = InReadPipe;
	 ProcHandle = InProcHandle;

	 TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		 FTickerDelegate::CreateUObject(this, &UProcessReaderProxy::Tick)
	 );
 }

 bool UProcessReaderProxy::Tick(float DeltaTime)
 {
	 if (!ReadPipe)
		 return false;

	 if (!FPlatformProcess::IsProcRunning(ProcHandle))
	 {
		 const FString Remaining = FPlatformProcess::ReadPipe(ReadPipe);
		 if (!Remaining.IsEmpty())
		 {
			 OnOutputReceived.Broadcast(Remaining);
		 }
		 FPlatformProcess::CloseProc(ProcHandle);
		 FPlatformProcess::ClosePipe(ReadPipe, nullptr);
		 ReadPipe = nullptr;
		 return false; 
	 }

	 const FString Output = FPlatformProcess::ReadPipe(ReadPipe);
	 if (!Output.IsEmpty())
	 {
		 OnOutputReceived.Broadcast(Output);
	 }
	 return true; 
 }

 void UProcessReaderProxy::BeginDestroy()
 {
	 if (TickHandle.IsValid())
	 {
		 FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	 }
	 if (ReadPipe)
	 {
		 FPlatformProcess::CloseProc(ProcHandle);
		 FPlatformProcess::ClosePipe(ReadPipe, nullptr);
	 }
	 Super::BeginDestroy();
 }

 ANavMeshBoundsVolume* UHelperBlueprintFunctionLibrary::SpawnNavMeshBoundsVolume(const UObject* WorldContextObject, FVector Position)
 {
	 if (UWorld* World = WorldContextObject->GetWorld())
	 {
		 return World->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(), Position, FRotator());
	 }
	 return nullptr;
 }

 void UHelperBlueprintFunctionLibrary::NavUpdate(USceneComponent* SceneComponent)
 {
	 if (SceneComponent->IsRegistered())
	 {
		 if (SceneComponent->GetWorld() != nullptr)
		 {
			 FNavigationSystem::UpdateComponentData(*SceneComponent);
		 }
	 }
 }



UReciveInputObject::~UReciveInputObject()
{
	if (StartSetup)
	{
		StopSetup();
	}
}

void UReciveInputObject::Setup()
{
	StartSetup = true;
	ReciveInputProcessor = MakeShared<FReciveInputProcessor>();
	ReciveInputProcessor->ReciveInputObject = this;
	FSlateApplication::Get().RegisterInputPreProcessor(ReciveInputProcessor);
}

void UReciveInputObject::StopSetup()
{
	StartSetup = false;
	ReciveInputProcessor->ReciveInputObject = nullptr;
	ReciveInputProcessor = nullptr;
	FSlateApplication::Get().UnregisterInputPreProcessor(ReciveInputProcessor);
	KeyPressDelegate.Clear();
}

void FReciveInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
}

bool FReciveInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	return false;
}

bool FReciveInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (IsValid(ReciveInputObject))
	{
		ReciveInputObject->KeyPressDelegate.Broadcast(InKeyEvent.GetKey());
	}
	return false;
}

bool FReciveInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (IsValid(ReciveInputObject))
	{
		ReciveInputObject->KeyPressDelegate.Broadcast(MouseEvent.GetEffectingButton());
	}
	return false;
}

void UReciveWorldObject::Setup(AActor* NewWorldContextObject)
{
	if (IsValid(NewWorldContextObject) && !SpawnNewActorDelegateHandle.IsValid())
	{
		WorldContextObject = NewWorldContextObject;
		SpawnNewActorDelegateHandle = WorldContextObject->GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UReciveWorldObject::BindCallNewActorDelegate));
		SpawnNewActorWithComponentsDelegateHandle = WorldContextObject->GetWorld()->AddOnPostRegisterAllActorComponentsHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UReciveWorldObject::BindCallNewActorWithComponentsDelegate));
	}
}

void UReciveWorldObject::BindCallNewActorDelegate(AActor* Actor)
{
	NewActorDelegate.Broadcast(Actor);
}

void UReciveWorldObject::BindCallNewActorWithComponentsDelegate(AActor* Actor)
{
	NewActorWithComponentsDelegate.Broadcast(Actor);
}

void UReciveWorldObject::StopSetup()
{
	if (IsValid(WorldContextObject)&& SpawnNewActorDelegateHandle.IsValid())
	{
		WorldContextObject->GetWorld()->RemoveOnActorSpawnedHandler(SpawnNewActorDelegateHandle);
		WorldContextObject->GetWorld()->RemoveOnPostRegisterAllActorComponentsHandler(SpawnNewActorWithComponentsDelegateHandle);
	}
}

bool UUpgradeViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
	bIsLastInputIsGamepad = EventArgs.IsGamepad();

	return Super::InputKey(EventArgs);
}


void UEffectStackFunctionLibrary::AddFloatValueToStack(FFloatEffectStackBlueprint& Stack, FName EffectName, float Value, EEffectApplicationMode Mode, int Priority)
{
	Stack.EffectStack.AddEffect(EffectName, Value, Mode, Priority);
}

void UEffectStackFunctionLibrary::RemoveFloatValueFromStack(FFloatEffectStackBlueprint& Stack, FName EffectName)
{
	Stack.EffectStack.RemoveEffect(EffectName);
}


float UEffectStackFunctionLibrary::CalculateFloatTotalEffect(UPARAM(ref)FFloatEffectStackBlueprint& Stack)
{
	return Stack.EffectStack.CalculateTotalEffect();
}

bool UEffectStackFunctionLibrary::HasFloatEffect(const FFloatEffectStackBlueprint& Stack, FName EffectName)
{
	return Stack.EffectStack.EffectMap.Contains(EffectName);
}

bool UEffectStackFunctionLibrary::GetFloatEffectValue(const FFloatEffectStackBlueprint& Stack, FName EffectName, float& OutValue, EEffectApplicationMode& OutMode)
{
	if (const FEffectData<float>* EffectData = Stack.EffectStack.EffectMap.Find(EffectName))
	{
		OutValue = EffectData->Value;
		OutMode = EffectData->ApplicationMode;
		return true;
	}
	return false;
}

void UEffectStackFunctionLibrary::SetFloatDefaultVariableStack(UPARAM(ref)FFloatEffectStackBlueprint& Stack, float DefaultStackVariable)
{
	Stack.EffectStack.DefaultValue = DefaultStackVariable;
}



void UEffectStackFunctionLibrary::AddIntValueToStack(FIntEffectStackBlueprint& Stack, FName EffectName, int Value, EEffectApplicationMode Mode, int Priority)
{
	Stack.EffectStack.AddEffect(EffectName, Value, Mode, Priority);
}

void UEffectStackFunctionLibrary::RemoveIntValueFromStack(FIntEffectStackBlueprint& Stack, FName EffectName)
{
	Stack.EffectStack.RemoveEffect(EffectName);
}

int UEffectStackFunctionLibrary::CalculateIntTotalEffect(const FIntEffectStackBlueprint& Stack)
{
	return Stack.EffectStack.CalculateTotalEffect();
}

bool UEffectStackFunctionLibrary::HasIntEffect(const FIntEffectStackBlueprint& Stack, FName EffectName)
{
	return Stack.EffectStack.EffectMap.Contains(EffectName);
}

bool UEffectStackFunctionLibrary::GetIntEffectValue(const FIntEffectStackBlueprint& Stack, FName EffectName, int& OutValue, EEffectApplicationMode& OutMode)
{
	if (const FEffectData<int>* EffectData = Stack.EffectStack.EffectMap.Find(EffectName))
	{
		OutValue = EffectData->Value;
		OutMode = EffectData->ApplicationMode;
		return true;
	}
	return false;
}

void UEffectStackFunctionLibrary::SetIntDefaultVariableStack(UPARAM(ref)FIntEffectStackBlueprint& Stack, int DefaultStackVariable)
{
	Stack.EffectStack.DefaultValue = DefaultStackVariable;
}


void UEffectStackFunctionLibrary::AddColorValueToStack(FLinearColorEffectStackBlueprint& Stack, FName EffectName, FLinearColor Value, EEffectApplicationMode Mode, int Priority)
{
	Stack.EffectStack.AddEffect(EffectName, Value, Mode, Priority);
}

void UEffectStackFunctionLibrary::RemoveColorValueFromStack(FLinearColorEffectStackBlueprint& Stack, FName EffectName)
{
	Stack.EffectStack.RemoveEffect(EffectName);
}

FLinearColor UEffectStackFunctionLibrary::CalculateColorTotalEffect(const FLinearColorEffectStackBlueprint& Stack)
{
	return Stack.EffectStack.CalculateTotalEffect();
}

bool UEffectStackFunctionLibrary::HasColorEffect(const FLinearColorEffectStackBlueprint& Stack, FName EffectName)
{
	return Stack.EffectStack.EffectMap.Contains(EffectName);
}

bool UEffectStackFunctionLibrary::GetColorEffectValue(const FLinearColorEffectStackBlueprint& Stack, FName EffectName, FLinearColor& OutValue, EEffectApplicationMode& OutMode)
{
	if (const FEffectData<FLinearColor>* EffectData = Stack.EffectStack.EffectMap.Find(EffectName))
	{
		OutValue = EffectData->Value;
		OutMode = EffectData->ApplicationMode;
		return true;
	}
	return false;
}

void UEffectStackFunctionLibrary::SetColorDefaultVariableStack(UPARAM(ref) FLinearColorEffectStackBlueprint& Stack, FLinearColor DefaultStackVariable)
{
	Stack.EffectStack.DefaultValue = DefaultStackVariable;
}

void UEffectStackFunctionLibrary::AddBoolValueToStack(UPARAM(ref)FBoolEffectStackBlueprint& Stack, FName EffectName, bool Value, EEffectApplicationMode Mode, int Priority)
{
	Stack.EffectStack.AddEffect(EffectName, Value, Mode, Priority);
}

void UEffectStackFunctionLibrary::RemoveBoolValueFromStack(UPARAM(ref)FBoolEffectStackBlueprint& Stack, FName EffectName)
{
	Stack.EffectStack.RemoveEffect(EffectName);
}

bool UEffectStackFunctionLibrary::CalculateBoolTotalEffect(const FBoolEffectStackBlueprint& Stack)
{
	return Stack.EffectStack.CalculateTotalEffect();
}

bool UEffectStackFunctionLibrary::HasBoolEffect(const FBoolEffectStackBlueprint& Stack, FName EffectName)
{
	return Stack.EffectStack.EffectMap.Contains(EffectName);
}

bool UEffectStackFunctionLibrary::GetBoolEffectValue(const FBoolEffectStackBlueprint& Stack, FName EffectName, bool& OutValue, EEffectApplicationMode& OutMode)
{
	if (const FEffectData<bool>* EffectData = Stack.EffectStack.EffectMap.Find(EffectName))
	{
		OutValue = EffectData->Value;
		OutMode = EffectData->ApplicationMode;
		return true;
	}
	return false;
}

void UEffectStackFunctionLibrary::SetBoolDefaultVariableStack(UPARAM(ref)FBoolEffectStackBlueprint& Stack, bool DefaultStackVariable)
{
	Stack.EffectStack.DefaultValue = DefaultStackVariable;
}