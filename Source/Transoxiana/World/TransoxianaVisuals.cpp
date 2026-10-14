#include "World/TransoxianaVisuals.h"

#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPath.h"

namespace TransoxianaVisuals
{
	static UStaticMesh* LoadEngineMesh(const TCHAR* Path)
	{
		return LoadObject<UStaticMesh>(nullptr, Path);
	}

	UStaticMesh* GetCube()     { return LoadEngineMesh(TEXT("/Engine/BasicShapes/Cube.Cube")); }
	UStaticMesh* GetCylinder() { return LoadEngineMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); }
	UStaticMesh* GetSphere()   { return LoadEngineMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere")); }
	UStaticMesh* GetCone()     { return LoadEngineMesh(TEXT("/Engine/BasicShapes/Cone.Cone")); }

	UMaterialInstanceDynamic* MakeColorMaterial(UObject* Outer, const FLinearColor& Color)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!Base)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Outer);
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
		return MID;
	}

	void SetupShape(UStaticMeshComponent* Component, UStaticMesh* Mesh, const FLinearColor& Color, UObject* MaterialOuter)
	{
		if (!Component)
		{
			return;
		}
		Component->SetStaticMesh(Mesh);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCanEverAffectNavigation(false);
		if (UMaterialInstanceDynamic* MID = MakeColorMaterial(MaterialOuter, Color))
		{
			Component->SetMaterial(0, MID);
		}
	}

	UObject* TryLoadOptional(const FString& Path)
	{
		if (Path.IsEmpty())
		{
			return nullptr;
		}
		const FSoftObjectPath SoftPath(Path);
		if (!SoftPath.IsValid() || !FPackageName::DoesPackageExist(SoftPath.GetLongPackageName()))
		{
			return nullptr;
		}
		return SoftPath.TryLoad();
	}
}
