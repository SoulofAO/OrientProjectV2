

#pragma once

#include "CoreMinimal.h"
#include "UpgradeObject.h"
#include "GlobalScenarioUpgradeObject.generated.h"

/**
 * 
 */

UENUM(Blueprintable)
enum class EScenaioConditionMode  : uint8
{
	Add,
	Multiply
};

USTRUCT(Blueprintable)
struct MAGIC_API FGlobalScenarioObjectStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	UGlobalScenarioObject* EditInLineGlobalScenarioObject;
};

UCLASS(Abstract)
class MAGIC_API UGlobalScenarioObject : public UEditInLineUpgradeObject
{
	GENERATED_BODY()

};


USTRUCT(Blueprintable)
struct MAGIC_API FScenarioConditionObjectStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EScenaioConditionMode ScenarioConditionMode = EScenaioConditionMode::Add;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	UScenarioCondition* EditInLineScenarioConditionObject;
};

UCLASS(Abstract)
class MAGIC_API UScenarioCondition : public UEditInLineUpgradeObject
{
	GENERATED_BODY()

};


USTRUCT(Blueprintable)
struct MAGIC_API FScenarioTargetsObjectStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	UScenarioTargetsObject* EditInLineScenarioTargetsObject;
};

UCLASS(Abstract)
class MAGIC_API UScenarioTargetsObject : public UEditInLineUpgradeObject
{
	GENERATED_BODY()

};