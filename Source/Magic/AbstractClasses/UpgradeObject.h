

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/ActorComponent.h"
#include "UpgradeObject.generated.h"

/**
 * 
 */

UCLASS(Blueprintable)
class UUpgradeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadWrite)
	TArray<UUpgradeObject*> ReplicatesObjects;

	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void ForceNetUpdate();

};

UCLASS(Blueprintable, meta = (ShowWorldContextPin))
class MAGIC_API UUpgradeObject : public UObject
{
	GENERATED_BODY()
public:

	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintPure)
	UObject* GetOwner() const { return GetOuter(); };

	UObject* GetOuterV2() const;

	UPROPERTY(BlueprintReadOnly)
	UObject* CustomOuter;

	UFUNCTION(BlueprintCallable)
	void SetCustomOuter(UObject* NewCustomOuter);

	virtual void PostInitProperties() override;

	UFUNCTION(BlueprintImplementableEvent)
	void BeginPlay();

	bool IsSupportedForNetworking() const override {
		return true;
	}

	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;

	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};


USTRUCT(Blueprintable)
struct FEditInLineUpgradeObjectStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	UEditInLineUpgradeObject* EditInLineUpgradeObject;
};

UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class MAGIC_API UEditInLineUpgradeObject : public UUpgradeObject
{
	GENERATED_BODY()

};



