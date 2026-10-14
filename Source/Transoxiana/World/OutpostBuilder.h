#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/TransoxianaTypes.h"
#include "OutpostBuilder.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class APlayerStart;
class ADispatchPickup;
class AExtractionZone;

/**
 * Builds the grey-box "Karmana rabat" for the prototype mission out of engine primitives:
 * mud-brick enclosure, corner towers, gate, west-wall breach, domed hall, rooms, shed,
 * tents, hay (hiding spots), outside poplars and rocks. Geometry is visible in the editor
 * (OnConstruction). At play time it also spawns the guards, hiding zones, objective,
 * extraction point, player start and — if the level has none — sun/sky/fog.
 *
 * Local axes: +X = north, +Y = east. Units are centimetres.
 */
UCLASS()
class TRANSOXIANA_API AOutpostBuilder : public AActor
{
	GENERATED_BODY()

public:
	AOutpostBuilder();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Spawns gameplay actors once. Called by the game mode before the player spawns. */
	void BuildRuntimeActors();

	AActor* GetPlayerStart() const;
	FVector GetDispatchLocation() const;
	FVector GetExtractionLocation() const;

	/** Half the side length of the walled enclosure. */
	UPROPERTY(EditAnywhere, Category = "Outpost")
	float HalfSize = 3500.f;

	UPROPERTY(EditAnywhere, Category = "Outpost")
	float WallHeight = 600.f;

	UPROPERTY(EditAnywhere, Category = "Outpost")
	bool bSpawnGuards = true;

	/** Adds sun, sky atmosphere, sky light and fog if the level has no directional light. */
	UPROPERTY(EditAnywhere, Category = "Outpost")
	bool bSpawnLightingIfMissing = true;

protected:
	virtual void BeginPlay() override;

private:
	void BuildGeometry();
	void SpawnLighting();
	void SpawnGuards();
	void SpawnHidingZones();
	AActor* SpawnEnemy(EEnemyType Type, const FVector& LocalLocation, float Yaw, const TArray<FVector>& LocalPatrol);

	void AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& Size, float Yaw = 0.f);
	FVector ToWorld(const FVector& Local) const;
	UInstancedStaticMeshComponent* MakeISM(const TCHAR* Name);

	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> GroundISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> MudBrickISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> TowerISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> FiredBrickISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> PlasterISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> TileISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> DomeISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> DrumISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> WoodISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> HayISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> TentISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> WellISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> RockISM;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UInstancedStaticMeshComponent> FoliageISM;

	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UPointLightComponent> HallTorch;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UPointLightComponent> GateTorchA;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UPointLightComponent> GateTorchB;
	UPROPERTY(VisibleAnywhere, Category = "Outpost") TObjectPtr<UPointLightComponent> CourtyardBrazier;

	UPROPERTY(Transient) TObjectPtr<APlayerStart> SpawnedPlayerStart;
	UPROPERTY(Transient) TObjectPtr<ADispatchPickup> SpawnedDispatch;
	UPROPERTY(Transient) TObjectPtr<AExtractionZone> SpawnedExtraction;

	bool bRuntimeBuilt = false;
};
