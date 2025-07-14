// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGManagedResource.h"
#include "Async/PCGAsyncLoadingContext.h"
#include "MeshSelectors/PCGMeshSelectorBase.h"
#include "InstanceDataPackers/PCGInstanceDataPackerBase.h"
#include "Components/SplineMeshComponent.h"
#include "PCGContext.h"
#include "PCGSplineMeshSpawnerSettings.generated.h"

/**
 * 
 */
struct FPCGContext;

UCLASS()
class UPCGSplineMeshSpawnerSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGSplineMeshSpawnerSettings(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	// ~Begin UPCGSettings interface
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("SplineMeshSpawner")); }
	virtual FText GetDefaultNodeTitle() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spawner; }
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return Super::DefaultPointInputPinProperties(); }
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override { return Super::DefaultPointOutputPinProperties(); }
	virtual FPCGElementPtr CreateElement() const override;
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	UStaticMesh* StaticMesh = nullptr;
};

class FPCGSplineMeshSpawnerElement : public IPCGElement
{
public:
	virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const override;
	virtual bool IsCacheable(const UPCGSettings* InSettings) const override { return false; }

protected:
	virtual FPCGContext* CreateContext() override;
	virtual bool PrepareDataInternal(FPCGContext* Context) const override;
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
	virtual void AbortInternal(FPCGContext* Context) const override;
public:
	void AddNewSplineMesh(FPCGSplineMeshSpawnerContext* Context, UPCGManagedSplineMeshComponentOldVersion * ManagedSplineMeshComponent, UStaticMesh* StaticMesh, FVector StartPosition, FVector EndPosition, FVector StartTangent, FVector EndTangent, FVector UpDir) const;
	static UPCGManagedSplineMeshComponentOldVersion * CreatePCGManagedSplineMeshComponent(FPCGSplineMeshSpawnerContext* Context);
};


USTRUCT(BlueprintType)
struct FPCGSplineMeshSpawnerContext : public FPCGContext, public IPCGAsyncLoadingContext
{
	GENERATED_BODY()
public:
	UPCGManagedSplineMeshComponentOldVersion * ResourceObject;
};

UCLASS(BlueprintType)
class UPCGManagedSplineMeshComponentOldVersion : public UPCGManagedResource
{
	GENERATED_BODY()

public:

	//~Begin UPCGManagedResource interface
	virtual bool Release(bool bHardRelease, TSet<TSoftObjectPtr<AActor>>& OutActorsToDelete);
	/** Releases resource if empty or unused. Returns true if the resource can be removed from the PCG component */
	virtual bool ReleaseIfUnused(TSet<TSoftObjectPtr<AActor>>& OutActorsToDelete);
	//~End UPCGManagedResource interface

	virtual void ResetComponent();
	TArray<TSoftObjectPtr<USplineMeshComponent>> GetComponents() const;
	void AddComponent(USplineMeshComponent* NewComponent);
	void RemoveComponent(USplineMeshComponent* RemoveComponent);
	void RemoveComponentByIndex(int Index);

protected:

	UPROPERTY()
	TArray<TSoftObjectPtr<USplineMeshComponent>> GeneratedComponents;

};


