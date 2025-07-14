// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/Actor.h"
#include "ActorSpatialSubsystem.generated.h"

/**
 * 
 */

USTRUCT()
struct FActorRefArray
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<AActor*> Array;
};

static TArray<FActorRefArray> FindActorRefByClass(const TMap<TSubclassOf<AActor>, FActorRefArray>& Map, TSubclassOf<AActor> Class)
{
	TArray<FActorRefArray> Result;
	for (const auto& Elem : Map)
	{
		if (Elem.Key->IsChildOf(Class))
		{
			Result.Add(Elem.Value);
		}
	}
	return Result;
}


UCLASS()
class USpatialNode : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FBox2D Bounds;

	UPROPERTY()
	int32 Depth = 0;
	
	UPROPERTY()
	TArray<USpatialNode*> Children;

	UPROPERTY()
	TMap<TSubclassOf<AActor>, FActorRefArray> ActorMap;

	bool IsLeaf() const { return Children.Num() == 0; }

	void Initialize(const FBox2D& InBounds, int32 InDepth)
	{
		Bounds = InBounds;
		Depth = InDepth;
	}

	void Subdivide();
	void ClearInvalidActorMap();

	void AddActor(AActor* Actor);
	void RemoveActor(AActor* Actor);
	void Query(const FBox2D& Area, TSubclassOf<AActor> Class, TArray<AActor*>& OutActors, bool EqualExactly) const;
};

UCLASS()
class UActorSpatialSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Spatial")
	void AddActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Spatial")
	void RemoveActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Spatial")
	void QueryActors(FVector2D Center, FVector2D Extents, TSubclassOf<AActor> Class, TArray<AActor*>& OutActors, bool EqualExactly = false) const;

	virtual void OnWorldBoundsChanged(const FBox2D& OldBounds, const FBox2D& NewBounds);

	UPROPERTY()
	USpatialNode* RootNode;

	UPROPERTY()
	TArray<AActor*> RegisteredActors;

	UPROPERTY()
	FBox2D WorldBounds = FBox2D(FVector2D(0,0), FVector2D(0, 0));

	void RecalculateWorldBoundsFromActors();

	void UpdateWorldBoundsIfNeeded(const FBox2D& ActorBounds);
};
