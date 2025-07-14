#include "HideableSplineComponent.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "UnrealClient.h"
#include "ViewportClient.h"
#include "EditorViewportClient.h"
#endif
UHideableSplineComponent::UHideableSplineComponent()
{
    // Включаем тик только в редакторе
    //PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bCanEverTick = true;
    bTickInEditor = true;
    bAutoActivate = true;
    //PrimaryComponentTick.bTickEvenWhenPaused = true;
    //PrimaryComponentTick.bStartWithTickEnabled = true; // Начинает тик сразу
    bHiddenInGame = false;
}

void UHideableSplineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
#if WITH_EDITOR
    // Обновляем видимость только в редакторе
    if (GetWorld() && GetWorld()->WorldType == EWorldType::Editor)
    {
        UpdateVisibility();
    }
#endif
}
#if WITH_EDITOR
void UHideableSplineComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (GetWorld() && GetWorld()->WorldType == EWorldType::Editor)
    {
        UpdateVisibility();
    }

}
#endif
void UHideableSplineComponent::UpdateVisibility()
{
#if WITH_EDITOR
    if (!EditorEngine.IsValid())
    {
        EditorEngine = GEditor;
    }

    if (EditorEngine.IsValid())
    {
        FVector CameraLocation;
        FRotator CameraRotation;
        auto Client = GEditor->GetActiveViewport()->GetClient();
        FViewport* activeViewport = GEditor->GetActiveViewport();
        FEditorViewportClient* editorViewClient = (activeViewport != nullptr) ? (FEditorViewportClient*)activeViewport->GetClient() : nullptr;
        if (editorViewClient)
        {
            CameraLocation = editorViewClient->GetViewLocation();
            CameraRotation = editorViewClient->GetViewRotation();

            float ClosestDistance = FLT_MAX;
            const int32 NumPoints = GetNumberOfSplinePoints();

            for (int32 i = 0; i < NumPoints; i++)
            {
                FVector SplinePoint = GetWorldLocationAtSplinePoint(i);
                float Distance = FVector::Dist(CameraLocation, SplinePoint);
                ClosestDistance = FMath::Min(ClosestDistance, Distance);
            }
            SetVisibility(ClosestDistance <= MaxVisibilityDistance);
        }
    }
#endif
}