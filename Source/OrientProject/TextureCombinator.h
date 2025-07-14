// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TextureResource.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureCombinator.generated.h"

/**
 * 
 */

USTRUCT(Blueprintable)
struct FTextureCombinatorStruct
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D Position;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D Size;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int Priority = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTextureRenderTarget2D* TextureRenderTarget;
};


UENUM(BlueprintType)
enum class ETextureFilterMode : uint8
{
	None UMETA(DisplayName = "None"),
	Simple UMETA(DisplayName = "Simple")
};


UCLASS(Blueprintable)
class ORIENTPROJECT_API UTextureCombinator : public UObject
{
	GENERATED_BODY()

public:

	virtual UWorld* GetWorld() const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETextureFilterMode TextureFilterMode;

	UFUNCTION(BlueprintCallable)
	void RenderTargetInitialization(UTextureRenderTarget2D* TextureRenderTarget, FVector2D NewSize);

	UFUNCTION(BlueprintCallable)
	void Texture2DInitialization(UTexture2D* Texture2D, FVector2D NewSize);

	TArray<TSharedPtr<FTextureCombinatorStruct>> TextureRenderTargets;

	UFUNCTION(BlueprintCallable)
	void K2_AddRenderTargetTextureCombinator(UTextureRenderTarget2D* TextureRenderTarget, FVector2D Position, FVector2D Size, int Priority);

	TSharedPtr<FTextureCombinatorStruct> AddTextureCombinator(UTextureRenderTarget2D* TextureRenderTarget, FVector2D Position, FVector2D Size, int Priority);
	
	UFUNCTION(BlueprintCallable)
	void K2_AddTexture2DCombinator(UTexture2D* Texture2DTarget, FVector2D Position, FVector2D Size, int Priority);

	TSharedPtr<FTextureCombinatorStruct> AddTextureCombinator(UTexture2D* Texture2DTarget, FVector2D Position, FVector2D Size, int Priority);

	void RemoveTextureCombinator(TSharedPtr<FTextureCombinatorStruct> TextureRenderTarget);

	UFUNCTION(BlueprintCallable)
	void RemoveTextureCombinatorByIndex(int Index);

	UFUNCTION(BlueprintCallable)
	UTextureRenderTarget2D* BakePart(FVector2D Position, FVector2D Size);

	UFUNCTION(BlueprintCallable)
	UTextureRenderTarget2D* BakeAll();


	TArray<TSharedPtr<FTextureCombinatorStruct>> FindTexturesInRange(FVector2D RangePosition, FVector2D RangeSize);

	FVector2D GlobalSize;
};
