


#include "ReplicatedFloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"

void UReplicatedFloatingPawnMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (!GetWorld())
	{
		return;
	}
	if (ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	UPawnMovementComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PawnOwner || UpdatedComponent )
	{
		ApplyControlInputToVelocity(DeltaTime);
	}
	Velocity = Velocity + LaunchVector;
	LaunchVector = FVector(0, 0, 0);

	if (IsExceedingMaxSpeed(GlobalMaxSpeed) == true)
	{
		Velocity = Velocity.GetUnsafeNormal() * GlobalMaxSpeed;
	}

	LimitWorldBounds();

	// Move actor

	FVector Delta = Velocity * DeltaTime;

	if (!Delta.IsNearlyZero(1e-6f))
	{
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FQuat Rotation = UpdatedComponent->GetComponentQuat();

		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(Delta, Rotation, true, Hit);

		if (Hit.IsValidBlockingHit())
		{
			HandleImpact(Hit, DeltaTime, Delta);
			// Try to slide the remaining distance along the surface.
			SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		}

		const FVector NewLocation = UpdatedComponent->GetComponentLocation();
		Velocity = ((NewLocation - OldLocation) / DeltaTime);
	}
	
	if ((GetWorld()->GetNetMode() == NM_ListenServer || GetWorld()->GetNetMode() == NM_DedicatedServer))
	{
		if (!(GetOwnerController() && Cast<APlayerController>(GetOwnerController())))
		{
			MulticastSetLocationAndVelocity(UpdatedComponent->GetComponentLocation(), Velocity, UpdatedComponent->GetComponentRotation(), FRotator(0, 0, 0), GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
			return;
		}
	}
	if ((GetWorld()->GetNetMode() == NM_Client && UGameplayStatics::GetPlayerController(GetWorld(), 0) == GetOwnerController()))
	{
		if (!GetWorld() || !GetWorld()->GetGameState())
		{
			return;
		}
		LastReciveClientTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		ServerSetLocationAndVelocity(UpdatedComponent->GetComponentLocation(), Velocity, UpdatedComponent->GetComponentRotation(), FRotator(0,0,0), GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
		return;
	}
	
}

void UReplicatedFloatingPawnMovement::ServerSetLocationAndVelocity_Implementation(FVector ClientPosition, FVector ClientVeloctity, FRotator ClientRotation, FRotator ClientAngularVelocity, float ClientTime)
{
	MulticastSetLocationAndVelocity(ClientPosition, ClientVeloctity, ClientRotation, ClientAngularVelocity, ClientTime);
}

void UReplicatedFloatingPawnMovement::MulticastSetLocationAndVelocity_Implementation(FVector ClientPosition, FVector ClientVeloctity, FRotator ClientRotation, FRotator ClientAngularVelocity, float ClientTime)
{
	if (!GetWorld() || !GetWorld()->GetGameState())
	{
		return;
	}
	
	
	if (!(GetWorld()->GetNetMode() == NM_Client&&UGameplayStatics::GetPlayerController(GetWorld(), 0) == GetOwnerController()) && ClientTime >= LastReciveClientTime)
	{
		LastReciveClientTime = ClientTime;

		float LDeltaTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - ClientTime;
		ClientRotation = ClientRotation + ClientAngularVelocity * LDeltaTime;
		ClientPosition = ClientPosition + ClientVeloctity * LDeltaTime;
		UpdatedComponent->SetWorldLocation(ClientPosition);
		UpdatedComponent->SetWorldRotation(ClientRotation);
		Velocity = ClientVeloctity + (ClientVeloctity - Velocity)*LDeltaTime;
		UpdateComponentVelocity();
	}
}

void UReplicatedFloatingPawnMovement::BeginPlay()
{
	Super::BeginPlay();
	PawnOwner = Cast<APawn>(GetOwner());
}

void UReplicatedFloatingPawnMovement::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
	UMovementComponent::SetUpdatedComponent(NewUpdatedComponent);
}

void UReplicatedFloatingPawnMovement::ApplyControlInputToVelocity(float DeltaTime)
{
	FVector ControlAcceleration = FVector(0,0,0);
	if (GetOwnerController())
	{
		ControlAcceleration = GetPendingInputVector().GetClampedToMaxSize(1.f);
	}
	else
	{
		ControlAcceleration = CachInputVector;
	}

	const float AnalogInputModifier = (ControlAcceleration.SizeSquared() > 0.f ? ControlAcceleration.Size() : 0.f);
	const float MaxPawnSpeed = GetMaxSpeed() * AnalogInputModifier;
	const bool bExceedingMaxSpeed = IsExceedingMaxSpeed(MaxPawnSpeed);

	if (AnalogInputModifier > 0.f && !bExceedingMaxSpeed)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Blue, (Velocity.Normalize() * MovementDecelerationVector * Deceleration * DeltaTime * -1).ToString());
		FVector NormalizeVelocity = Velocity;
		NormalizeVelocity.Normalize();
		float Power = 1+ (1 - (NormalizeVelocity.Dot(ControlAcceleration) + 1)/2) * TurningBoost;
		
		Velocity = Velocity + ControlAcceleration * Acceleration * DeltaTime * Power;
	}
	else
	{
		if (Velocity.SizeSquared() > 0.1f)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Red, (Velocity.Normalize() * MovementDecelerationVector * Deceleration * DeltaTime * -1).ToString());
			FVector NormalizeVelocity = Velocity;
			NormalizeVelocity.Normalize();
			Velocity = Velocity - (NormalizeVelocity * MovementDecelerationVector * Deceleration * DeltaTime)*(FMath::Clamp(Velocity.Length()/ (NormalizeVelocity * MovementDecelerationVector * Deceleration * DeltaTime).Length(),0.0,1.0));
			
		}
	}
	ConsumeInputVector();
}

AController* UReplicatedFloatingPawnMovement::GetOwnerController()
{
	AController* Controller = nullptr;
	if (IsValid(CustomController))
	{
		Controller = CustomController;
	}
	else
	{
		if(PawnOwner)
		{ 
			Controller = PawnOwner->GetController();
		}
	}
	return Controller;
}

void UReplicatedFloatingPawnMovement::LaunchPawn(FVector Vector)
{
	LaunchVector = LaunchVector + Vector;
}

void UReplicatedFloatingPawnMovement::AddInputVector(FVector WorldVector, bool bForce)
{
	if (!PawnOwner)
	{
		CachInputVector = WorldVector;
	}
	else
	{
		Super::AddInputVector(WorldVector, bForce);
	}
}

FVector UReplicatedFloatingPawnMovement::ConsumeInputVector()
{
	if (!PawnOwner)
	{
		FVector LReturnCachInputVector = CachInputVector;
		CachInputVector = FVector(0,0,0);
		return LReturnCachInputVector;
	}
	else
	{
		return Super::ConsumeInputVector();
	}

}
