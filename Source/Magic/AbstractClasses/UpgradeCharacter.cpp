


#include "UpgradeCharacter.h"

// Sets default values
AUpgradeCharacter::AUpgradeCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUpgradeCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUpgradeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AUpgradeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AUpgradeCharacter::Destroyed()
{
	AActor::Destroyed();
	DetachFromControllerPendingDestroy();
}

