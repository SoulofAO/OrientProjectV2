#include "Magic/AbstractClasses/UpgradeObject.h"
#include "Engine/NetDriver.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/ActorChannel.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/NetConnection.h"

UWorld* UUpgradeObject::GetWorld() const
{
	// Возвращаем ссылку на мир из владельца объекта, если не работаем редакторе.
	if (GIsEditor && !GIsPlayInEditorWorld) return nullptr;
	else if (GetOuter()) return GetOuter()->GetWorld();
	else if (GWorld.GetReference()) return GWorld.GetReference();
	else return nullptr;
}

UObject* UUpgradeObject::GetOuterV2() const
{
	if (IsValid(CustomOuter))
	{
		return CustomOuter;
	}
	if (IsValid(GetOuter()))
	{
		return GetOuter();
	}
	return nullptr;
}

void UUpgradeObject::SetCustomOuter(UObject* NewCustomOuter)
{
	CustomOuter = NewCustomOuter;
}

void UUpgradeObject::PostInitProperties()
{
	Super::PostInitProperties();

	if (GetOuter() && GetOuter()->GetWorld())
		BeginPlay();
}


bool UUpgradeObject::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	if (!GetOuterV2()) return false;
	UNetDriver* NetDriver = GetWorld()->GetNetDriver();
	if (!NetDriver) return false;
	AActor* Actor = nullptr;
	if (Cast<AActor>(GetOuterV2()))
	{
		Actor = Cast<AActor>(GetOuterV2());
	}
	if (Cast<UActorComponent>(GetOuterV2()))
	{
		UActorComponent* ActorComponent = Cast<UActorComponent>(GetOuterV2());
		Actor = ActorComponent->GetOwner();
	}
	if (Actor)
	{
		NetDriver->ProcessRemoteFunction(Actor, Function, Parms, OutParms, Stack, this);
	}
	return true;
}

int32 UUpgradeObject::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	AActor* Actor = nullptr;
	if (Cast<AActor>(GetOuterV2()))
	{
		Actor = Cast<AActor>(GetOuterV2());
	}
	if (Cast<UActorComponent>(GetOuterV2()))
	{
		UActorComponent* ActorComponent = Cast<UActorComponent>(GetOuterV2());
		Actor = ActorComponent->GetOwner();
	}
	if (Actor)
	{
		return Actor->GetFunctionCallspace(Function, Stack);
	}
	return FunctionCallspace::Local;
}

void UUpgradeObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass) BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
}

bool UUpgradeComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (UObject* LObject : ReplicatesObjects)
	{
		// Реплицируем наш объект.
		if (LObject) WroteSomething |= Channel->ReplicateSubobject(LObject, *Bunch, *RepFlags);
	}

	return WroteSomething;
}

void UUpgradeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UUpgradeComponent, ReplicatesObjects);

	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass) BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);


}

void UUpgradeComponent::ForceNetUpdate()
{
	if (GetOwner()->GetNetDriver())
	{
		for (UNetConnection* Connection : GetOwner()->GetNetDriver()->ClientConnections)
		{
			UActorChannel* Channel = Connection->ActorChannelMap().FindRef(GetOwner());
			if (Channel != nullptr)
				Channel->ReplicateActor();
		}
	}
	
}
