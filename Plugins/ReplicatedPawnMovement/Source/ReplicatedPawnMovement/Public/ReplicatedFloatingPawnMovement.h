

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "ReplicatedFloatingPawnMovement.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent))
class UReplicatedFloatingPawnMovement : public UFloatingPawnMovement
{
	GENERATED_BODY()
public:
	virtual void BeginPlay();

	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void ApplyControlInputToVelocity(float DeltaTime) override;

	UPROPERTY(BlueprintReadWrite)
	AController* CustomController;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float GlobalMaxSpeed = 10000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector MovementDecelerationVector = FVector(1, 1, 1);

	UPROPERTY(BlueprintReadWrite)
	float LastReciveClientTime;

	UFUNCTION(Server, Reliable)
	virtual void ServerSetLocationAndVelocity(FVector ClientPosition, FVector ClientVeloctity, FRotator ClientRotation, FRotator ClientAngularVelocity, float ClientTime);

	UFUNCTION(NetMulticast, Reliable)
	virtual void MulticastSetLocationAndVelocity(FVector ClientPosition, FVector ClientVeloctity, FRotator ClientRotation, FRotator ClientAngularVelocity, float ClientTime);

	UFUNCTION(BlueprintCallable)
	AController* GetOwnerController();

	UFUNCTION(BlueprintCallable)
	void LaunchPawn(FVector Vector);

	UPROPERTY()
	FVector LaunchVector;

	UPROPERTY()
	FRotator LastRotate;

	UPROPERTY()
	FVector CachInputVector;

	virtual void AddInputVector(FVector WorldVector, bool bForce = false);

	virtual FVector ConsumeInputVector();

};
