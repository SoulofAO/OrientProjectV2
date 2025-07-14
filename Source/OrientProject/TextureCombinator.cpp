// Fill out your copyright notice in the Description page of Project Settings.


#include "TextureCombinator.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Algo/Reverse.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include "Stats/Stats.h"
UWorld* UTextureCombinator::GetWorld() const
{
    UWorld* SuperWorld = Super::GetWorld();
    if (SuperWorld)
    {
        return SuperWorld;
    }
#if WITH_EDITOR
    if (GEngine)
    {
        return GEngine->GetWorld(); // Только если GEngine существует
    }
#endif
    return nullptr;
}

void UTextureCombinator::RenderTargetInitialization(UTextureRenderTarget2D* TextureRenderTarget, FVector2D NewSize)
{
    GlobalSize = NewSize;
    FTextureCombinatorStruct TextureCombinatorStruct = FTextureCombinatorStruct();
    TextureCombinatorStruct.Size = NewSize;
    AddTextureCombinator(TextureRenderTarget, FVector2D(0, 0), NewSize, 0);
}

void UTextureCombinator::Texture2DInitialization(UTexture2D* Texture2D, FVector2D NewSize)
{
    GlobalSize = NewSize;
    UTextureRenderTarget2D* TextureRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, GlobalSize.X, GlobalSize.Y);
    UCanvas* Canvans;
    FDrawToRenderTargetContext DrawToRenderTargetContext;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, TextureRenderTarget, Canvans, GlobalSize, DrawToRenderTargetContext);
    Canvans->K2_DrawTexture(Cast<UTexture>(Texture2D), FVector2D(0, 0), NewSize, FVector2D(0, 0));
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, DrawToRenderTargetContext);
    AddTextureCombinator(TextureRenderTarget, FVector2D(0, 0), NewSize, 0);
}

void UTextureCombinator::K2_AddRenderTargetTextureCombinator(UTextureRenderTarget2D* TextureRenderTarget, FVector2D Position, FVector2D Size, int Priority)
{
    AddTextureCombinator(TextureRenderTarget, Position, Size, Priority);
}

TSharedPtr<FTextureCombinatorStruct> UTextureCombinator::AddTextureCombinator(UTextureRenderTarget2D* TextureRenderTarget, FVector2D Position, FVector2D Size, int Priority)
{
    TSharedPtr<FTextureCombinatorStruct> NewStruct = MakeShared<FTextureCombinatorStruct>();
    NewStruct->Position = Position;
    NewStruct->Size = Size;
    NewStruct->Priority = Priority;
    NewStruct->TextureRenderTarget = TextureRenderTarget;
    TextureRenderTargets.Add(NewStruct);
    return NewStruct;
}

void UTextureCombinator::K2_AddTexture2DCombinator(UTexture2D* Texture2D, FVector2D Position, FVector2D Size, int Priority)
{
    AddTextureCombinator(Texture2D, Position, Size, Priority);
}

TSharedPtr<FTextureCombinatorStruct> UTextureCombinator::AddTextureCombinator(UTexture2D* Texture2DTarget, FVector2D Position, FVector2D Size, int Priority)
{
    UTextureRenderTarget2D* TextureRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, Size.X, Size.Y);
    UCanvas* Canvans;
    FDrawToRenderTargetContext DrawToRenderTargetContext;

    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, TextureRenderTarget, Canvans, Size, DrawToRenderTargetContext);
    Canvans->K2_DrawTexture(Cast<UTexture>(Texture2DTarget), FVector2D(0, 0), Size, FVector2D(0, 0));
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, DrawToRenderTargetContext);

    return AddTextureCombinator(TextureRenderTarget, Position, Size, Priority);
}

void UTextureCombinator::RemoveTextureCombinator(TSharedPtr<FTextureCombinatorStruct> TextureRenderTarget)
{
    TextureRenderTargets.Remove(TextureRenderTarget);
}

void UTextureCombinator::RemoveTextureCombinatorByIndex(int Index)
{
    RemoveTextureCombinator(TextureRenderTargets[Index]);
}

UTextureRenderTarget2D* UTextureCombinator::BakePart(FVector2D Position, FVector2D Size)
{
    Size.X = FMath::Floor(Size.X);
    Size.Y = FMath::Floor(Size.Y);

    if (Size.X == 0 || Size.Y == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Try Bake when Size equal Zero Vector!"));
        return nullptr;
    }
    TArray<TSharedPtr<FTextureCombinatorStruct>> TargetTextures;

    FVector2D AreaEnd = Position + Size;

    for (const TSharedPtr<FTextureCombinatorStruct>& TextureStruct : TextureRenderTargets)
    {
        if (!TextureStruct.IsValid())
        {
            continue;
        }

        FVector2D TextureEnd = TextureStruct->Position + TextureStruct->Size;

        if (TextureStruct->Position.X < AreaEnd.X &&
            TextureStruct->Position.Y < AreaEnd.Y &&
            TextureEnd.X > Position.X &&
            TextureEnd.Y > Position.Y)
        {
            TargetTextures.Add(TextureStruct);
        }
    }

    TargetTextures.Sort([](const TSharedPtr<FTextureCombinatorStruct>& A, const TSharedPtr<FTextureCombinatorStruct>& B)
        {
            if (A->Priority != B->Priority)
            {
                return A->Priority < B->Priority;
            }
            return A->Size.X * A->Size.Y < B->Size.X * B->Size.Y;
        });

    if (TextureFilterMode == ETextureFilterMode::Simple)
    {
        for (int32 i = TargetTextures.Num() - 1; i >= 0; --i)
        {
            const TSharedPtr<FTextureCombinatorStruct>& Texture = TargetTextures[i];
            bool bFullyCovered = false;

            for (int32 j = 0; j < i; ++j)
            {
                const TSharedPtr<FTextureCombinatorStruct>& HigherPriorityTexture = TargetTextures[j];

                if (HigherPriorityTexture->Position.X <= Texture->Position.X &&
                    HigherPriorityTexture->Position.Y <= Texture->Position.Y &&
                    HigherPriorityTexture->Position.X + HigherPriorityTexture->Size.X >= Texture->Position.X + Texture->Size.X &&
                    HigherPriorityTexture->Position.Y + HigherPriorityTexture->Size.Y >= Texture->Position.Y + Texture->Size.Y)
                {
                    bFullyCovered = true;
                    break;
                }
            }

            if (bFullyCovered)
            {
                TargetTextures.RemoveAt(i);
            }
        }
    }
    if (!GetWorld())
    {
        return nullptr;
    }

    UTextureRenderTarget2D* TextureRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, Size.X, Size.Y, RTF_RGBA16f, FLinearColor::Black);

    TextureRenderTarget->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
#if WITH_EDITOR
    TextureRenderTarget->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
#endif
    TextureRenderTarget->SRGB = false;
    TextureRenderTarget->LODGroup = TEXTUREGROUP_Pixels2D;

    UCanvas* Canvans;
    FDrawToRenderTargetContext DrawToRenderTargetContext;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, TextureRenderTarget, Canvans, Size, DrawToRenderTargetContext);

    for (const TSharedPtr<FTextureCombinatorStruct>& TextureStruct : TargetTextures)
    {
        if (!TextureStruct->TextureRenderTarget || !TextureStruct->TextureRenderTarget->GetResource())
        {
            continue;
        }

        if (TextureStruct->Size.X > 0 && TextureStruct->Size.Y > 0)
        {
            FCanvasTileItem TileItem(TextureStruct->Position - Position ,
                TextureStruct->TextureRenderTarget->GetResource(),
                TextureStruct->Size,
                FVector2D(0,0),
                FVector2D(1,1),
                FColor::White);
            TileItem.BlendMode = SE_BLEND_AlphaBlend;
            TileItem.Rotation = FRotator(0, 0, 0);
            TileItem.PivotPoint = FVector2D(0.5,0.5);
            Canvans->DrawItem(TileItem);
        }
    }

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, DrawToRenderTargetContext);

    /*
    for (int32 i = TextureRenderTargets.Num() - 1; i >= 0; --i)
    {
        const TSharedPtr<FTextureCombinatorStruct>& TextureStruct = TextureRenderTargets[i];

        if (!TextureStruct.IsValid())
        {
            continue;
        }

        FVector2D TextureEnd = TextureStruct->Position + TextureStruct->Size;

        if (TextureStruct->Position.X >= Position.X &&
            TextureStruct->Position.Y >= Position.Y &&
            TextureEnd.X <= AreaEnd.X &&
            TextureEnd.Y <= AreaEnd.Y)
        {
            TextureRenderTargets.RemoveAt(i);
        }
    }
    */
    return TextureRenderTarget;
}

UTextureRenderTarget2D* UTextureCombinator::BakeAll()
{
    return BakePart(FVector2D(0, 0), GlobalSize);
}

TArray<TSharedPtr<FTextureCombinatorStruct>> UTextureCombinator::FindTexturesInRange(FVector2D RangePosition, FVector2D RangeSize)
{
        TArray<TSharedPtr<FTextureCombinatorStruct>> FoundTextures;

        FVector2D RangeEnd = RangePosition + RangeSize;

        for (const TSharedPtr<FTextureCombinatorStruct>& TextureStruct : TextureRenderTargets)
        {
            if (!TextureStruct.IsValid())
            {
                continue;
            }

            FVector2D TextureEnd = TextureStruct->Position + TextureStruct->Size;

            if (TextureStruct->Position.X >= RangePosition.X &&
                TextureStruct->Position.Y >= RangePosition.Y &&
                TextureEnd.X <= RangeEnd.X &&
                TextureEnd.Y <= RangeEnd.Y)
            {
                FoundTextures.Add(TextureStruct);
            }
        }

        return FoundTextures;
}
