

#include "UpgradePawn.h"

// Sets default values
AUpgradePawn::AUpgradePawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUpgradePawn::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AUpgradePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AUpgradePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AUpgradePawn::EnableInput(APlayerController* PlayerController)
{
	APawn::EnableInput(PlayerController);

	AActor::EnableInput(PlayerController);
}

void AUpgradePawn::DisableInput(APlayerController* PlayerController)
{
	APawn::DisableInput(PlayerController);

	AActor::DisableInput(PlayerController);
}

