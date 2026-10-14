#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * Helpers for the grey-box look: engine basic shapes (always available, no downloads)
 * tinted with a dynamic instance of the engine's BasicShapeMaterial.
 */
namespace TransoxianaVisuals
{
	UStaticMesh* GetCube();
	UStaticMesh* GetCylinder();
	UStaticMesh* GetSphere();
	UStaticMesh* GetCone();

	/** Creates a tinted material. Returns null if the engine material is missing. */
	UMaterialInstanceDynamic* MakeColorMaterial(UObject* Outer, const FLinearColor& Color);

	/** Sets mesh + tint + no collision on a decorative component. */
	void SetupShape(UStaticMeshComponent* Component, UStaticMesh* Mesh, const FLinearColor& Color, UObject* MaterialOuter);

	/** Loads an optional asset by path without warnings when it does not exist. */
	UObject* TryLoadOptional(const FString& Path);

	// Palette (see docs/VISUAL_DIRECTION.md section 4.1), converted to linear colour.
	const FLinearColor MudBrick = FLinearColor::FromSRGBColor(FColor(0xB8, 0x9A, 0x72));
	const FLinearColor MudBrickDark = FLinearColor::FromSRGBColor(FColor(0xA5, 0x83, 0x5A));
	const FLinearColor FiredBrick = FLinearColor::FromSRGBColor(FColor(0x9C, 0x6B, 0x48));
	const FLinearColor Plaster = FLinearColor::FromSRGBColor(FColor(0xD9, 0xCD, 0xB4));
	const FLinearColor Turquoise = FLinearColor::FromSRGBColor(FColor(0x2E, 0x9C, 0x9A));
	const FLinearColor Cobalt = FLinearColor::FromSRGBColor(FColor(0x1F, 0x3F, 0x8F));
	const FLinearColor Madder = FLinearColor::FromSRGBColor(FColor(0x8E, 0x2F, 0x25));
	const FLinearColor Hay = FLinearColor::FromSRGBColor(FColor(0xC9, 0xB0, 0x5E));
	const FLinearColor Wood = FLinearColor::FromSRGBColor(FColor(0x5A, 0x3E, 0x28));
	const FLinearColor Ground = FLinearColor::FromSRGBColor(FColor(0xA8, 0x93, 0x6E));
	const FLinearColor Steel = FLinearColor::FromSRGBColor(FColor(0xB0, 0xB4, 0xB8));
	const FLinearColor Cloth = FLinearColor::FromSRGBColor(FColor(0x3A, 0x34, 0x30));
	const FLinearColor Skin = FLinearColor::FromSRGBColor(FColor(0xB5, 0x8A, 0x6A));
	const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(0xE0, 0xB0, 0x40));
}
