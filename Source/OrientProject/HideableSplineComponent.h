#pragma once

#include "Components/SplineComponent.h"
#include "HideableSplineComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UHideableSplineComponent : public USplineComponent
{
    GENERATED_BODY()

public:
    UHideableSplineComponent();

    // Максимальная дистанция видимости сплайна в редакторе
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
    float MaxVisibilityDistance = 5000.0f;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void UpdateVisibility();
#if WITH_EDITOR
    // Кэшированная ссылка на камеру редактора
    TWeakObjectPtr<class UEditorEngine> EditorEngine;
#endif
};