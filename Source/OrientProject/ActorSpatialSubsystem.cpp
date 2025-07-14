// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorSpatialSubsystem.h"
#include "GameFramework/Actor.h"

constexpr int32 MaxActorsPerNode = 10;
constexpr int32 MaxDepth = 6;

void USpatialNode::Subdivide()
{
	FVector2D Center = Bounds.GetCenter();
	FVector2D Min = Bounds.Min;
	FVector2D Max = Bounds.Max;

	Children.SetNum(4);
	Children[0] = NewObject<USpatialNode>(this);      
	Children[0]->Initialize(FBox2D(Min, Center), Depth + 1);
	Children[1] = NewObject<USpatialNode>(this); // BR
	Children[1]->Initialize(FBox2D(FVector2D(Center.X, Min.Y), FVector2D(Max.X, Center.Y)), Depth + 1);
	Children[2] = NewObject<USpatialNode>(this); // TL
	Children[2]->Initialize(FBox2D(FVector2D(Min.X, Center.Y), FVector2D(Center.X, Max.Y)), Depth + 1);
	Children[3] = NewObject<USpatialNode>(this);     
	Children[3]->Initialize(FBox2D(Center, Max), Depth + 1);

	for (auto& Pair : ActorMap)
	{
		for (AActor* Actor : Pair.Value.Array)
		{
			for (auto& Child : Children)
			{
				Child->AddActor(Actor);
			}
		}
	}
	ActorMap.Empty();
}

void USpatialNode::ClearInvalidActorMap()
{
	TMap<TSubclassOf<AActor>, FActorRefArray> CleanMap;

	if (ActorMap.IsEmpty())
	{
		return;
	}
	for (TPair<TSubclassOf<AActor>, FActorRefArray> Pair : ActorMap)
	{
		TSubclassOf<AActor> ActorClass = Pair.Key;

		// ѕропускаем невалидные классы
		if (!ActorClass || !IsValid(ActorClass.Get()))
		{
			continue;
		}

		FActorRefArray CleanArray;
		for (AActor* Actor : Pair.Value.Array)
		{
			if (IsValid(Actor))
			{
				CleanArray.Array.Add(Actor);
			}
		}

		// ƒобавл€ем только если после чистки что-то осталось
		if (CleanArray.Array.Num() > 0)
		{
			CleanMap.Add(ActorClass, CleanArray);
		}
	}

	ActorMap = MoveTemp(CleanMap);
}

void USpatialNode::AddActor(AActor* Actor)
{
	if (!Actor) return;

	FVector Origin, Extent3D;
	Actor->GetActorBounds(false, Origin, Extent3D);
	FBox2D ActorBox(FVector2D(Origin) - FVector2D(Extent3D), FVector2D(Origin) + FVector2D(Extent3D));
	ClearInvalidActorMap();

	if (!Bounds.Intersect(ActorBox)) return;

	if (IsLeaf())
	{
		ActorMap.FindOrAdd(Actor->GetClass()).Array.Add(Actor);

		int32 Total = 0;
		for (auto& Entry : ActorMap)
			Total += Entry.Value.Array.Num();

		if (Total > MaxActorsPerNode && Depth < MaxDepth)
			Subdivide();
	}
	else
	{
		for (auto& Child : Children)
		{
			Child->AddActor(Actor);
		}
	}
}

void USpatialNode::RemoveActor(AActor* Actor)
{
	if (!Actor) return;

	if (!Bounds.bIsValid) return;

	if (IsLeaf())
	{
		if (ActorMap.Contains(Actor->GetClass()))
		{
			ActorMap.Find(Actor->GetClass())->Array.Remove(Actor);
		}
	}
	else
	{
		for (auto& Child : Children)
		{
			Child->RemoveActor(Actor);
		}
	}
}

void USpatialNode::Query(const FBox2D& Area, TSubclassOf<AActor> Class, TArray<AActor*>& OutActors, bool EqualExactly) const
{
	if (!Bounds.bIsValid) return;
	if (!Bounds.Intersect(Area)) return;

	if (IsLeaf())
	{
		TArray<FActorRefArray> FoundRefs;
		if (EqualExactly)
		{
			if (ActorMap.Contains(Class))
			{
				FoundRefs.Add(*ActorMap.Find(Class));
			}
		}
		else
		{
			FoundRefs = FindActorRefByClass(ActorMap, Class);
		}

		for (FActorRefArray ActorRefArray : FoundRefs)
		{
			for (AActor* Actor : ActorRefArray.Array)
			{
				if (!Actor)
				{
					continue;
				}
				FVector Origin, Extent3D;
				Actor->GetActorBounds(false, Origin, Extent3D);
				FBox2D ActorBox(FVector2D(Origin) - FVector2D(Extent3D), FVector2D(Origin) + FVector2D(Extent3D));

				if (Area.Intersect(ActorBox))
				{
					OutActors.AddUnique(Actor);
				}
			}
		}
	}
	else
	{
		for (const auto& Child : Children)
		{
			Child->Query(Area, Class, OutActors, EqualExactly);
		}
	}
}

// -------- Subsystem

void UActorSpatialSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	if (!RootNode && WorldBounds!= FBox2D(FVector2D(0, 0), FVector2D(0, 0)))
	{
		RootNode = NewObject<USpatialNode>(this);
		RootNode->Initialize(WorldBounds, 0);
	}
}

void UActorSpatialSubsystem::Deinitialize()
{
}

void UActorSpatialSubsystem::AddActor(AActor* Actor)
{
	if (!Actor) return;
	if (RegisteredActors.Contains(Actor)) return;

	FBox2D ActorBounds = FBox2D(EForceInit::ForceInit);
	FBox ActorBox = Actor->GetComponentsBoundingBox(true);
	if (ActorBox.IsValid)
	{
		ActorBounds = FBox2D(FVector2D(ActorBox.Min), FVector2D(ActorBox.Max));
		UpdateWorldBoundsIfNeeded(ActorBounds);
	}
	if (!RootNode) return;
	RegisteredActors.Add(Actor);
	RootNode->AddActor(Actor);
}

void UActorSpatialSubsystem::RemoveActor(AActor* Actor)
{
	if (!Actor) return;
	if (!RegisteredActors.Contains(Actor)) return;
	if (!RootNode) return;

	RootNode->RemoveActor(Actor);
	RegisteredActors.Remove(Actor);
}
void UActorSpatialSubsystem::QueryActors(FVector2D Center, FVector2D Extents, TSubclassOf<AActor> Class, TArray<AActor*>& OutActors, bool EqualExactly) const
{
	FBox2D QueryBox(Center - Extents, Center + Extents);
	if (RootNode)
	{
		RootNode->Query(QueryBox, Class, OutActors, EqualExactly);
	}
}

void UActorSpatialSubsystem::OnWorldBoundsChanged(const FBox2D& OldBounds, const FBox2D& NewBounds)
{
	if (NewBounds != WorldBounds && NewBounds != FBox2D(FVector2D(0, 0), FVector2D(0, 0)))
	{
		WorldBounds = NewBounds;
		RootNode = NewObject<USpatialNode>(this);
		RootNode->Initialize(WorldBounds, 0);

		TArray<AActor*> UpdateRegisteredActors = RegisteredActors;

		for (AActor* Actor : UpdateRegisteredActors)
		{
			RemoveActor(Actor);
			AddActor(Actor);
		}
	}
}

void UActorSpatialSubsystem::UpdateWorldBoundsIfNeeded(const FBox2D& ActorBounds)
{
	if (!ActorBounds.bIsValid) return;

	if (!WorldBounds.IsInside(ActorBounds) && ActorBounds!= WorldBounds)
	{
		FBox2D OldBounds = WorldBounds;
		FBox2D NewBounds = WorldBounds + ActorBounds;
		OnWorldBoundsChanged(OldBounds, NewBounds);
	}
}

void UActorSpatialSubsystem::RecalculateWorldBoundsFromActors()
{
	FBox2D NewBounds(EForceInit::ForceInit);

	for (AActor* Actor : RegisteredActors)
	{
		if (!Actor) continue;

		FBox Box = Actor->GetComponentsBoundingBox(true);
		if (Box.IsValid)
		{
			FBox2D Box2D(FVector2D(Box.Min), FVector2D(Box.Max));
			NewBounds += Box2D;
		}
	}

	if (NewBounds.bIsValid && NewBounds != WorldBounds)
	{
		FBox2D OldBounds = WorldBounds;
		WorldBounds = NewBounds;
		OnWorldBoundsChanged(OldBounds, WorldBounds);
	}
}
