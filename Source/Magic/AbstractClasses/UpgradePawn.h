

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UpgradePawn.generated.h"

UCLASS()
class AUpgradePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AUpgradePawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void EnableInput(APlayerController* PlayerController) override;
	virtual void DisableInput(APlayerController* PlayerController) override;

};
