#include "World/OutpostBuilder.h"

#include "AI/EnemyCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Mission/DispatchPickup.h"
#include "Mission/ExtractionZone.h"
#include "Transoxiana.h"
#include "World/StealthZone.h"
#include "World/TransoxianaVisuals.h"

namespace
{
	// Key positions (local space), shared by geometry and gameplay so they stay in sync.
	const FVector DispatchLocal(3200.f, 0.f, 110.f);
	const FVector PlayerStartLocal(-500.f, -5700.f, 100.f);
	const float HallMinX = 1900.f;
	const float HallHalfWidth = 800.f;

	UPointLightComponent* MakeTorch(AActor* Owner, USceneComponent* Parent, const TCHAR* Name)
	{
		UPointLightComponent* Light = Owner->CreateDefaultSubobject<UPointLightComponent>(Name);
		Light->SetupAttachment(Parent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(30.f);
		Light->SetAttenuationRadius(1200.f);
		Light->SetLightColor(FLinearColor(1.f, 0.55f, 0.25f));
		return Light;
	}
}

AOutpostBuilder::AOutpostBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	GroundISM     = MakeISM(TEXT("Ground"));
	MudBrickISM   = MakeISM(TEXT("MudBrick"));
	TowerISM      = MakeISM(TEXT("Towers"));
	FiredBrickISM = MakeISM(TEXT("FiredBrick"));
	PlasterISM    = MakeISM(TEXT("Plaster"));
	TileISM       = MakeISM(TEXT("GlazedTile"));
	DomeISM       = MakeISM(TEXT("Dome"));
	DrumISM       = MakeISM(TEXT("DomeDrum"));
	WoodISM       = MakeISM(TEXT("Wood"));
	HayISM        = MakeISM(TEXT("Hay"));
	TentISM       = MakeISM(TEXT("Tents"));
	WellISM       = MakeISM(TEXT("Well"));
	RockISM       = MakeISM(TEXT("Rocks"));
	FoliageISM    = MakeISM(TEXT("Poplars"));

	HallTorch        = MakeTorch(this, RootComponent, TEXT("HallTorch"));
	GateTorchA       = MakeTorch(this, RootComponent, TEXT("GateTorchA"));
	GateTorchB       = MakeTorch(this, RootComponent, TEXT("GateTorchB"));
	CourtyardBrazier = MakeTorch(this, RootComponent, TEXT("CourtyardBrazier"));
}

UInstancedStaticMeshComponent* AOutpostBuilder::MakeISM(const TCHAR* Name)
{
	UInstancedStaticMeshComponent* ISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
	ISM->SetupAttachment(RootComponent);
	ISM->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	return ISM;
}

void AOutpostBuilder::AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& Size, float Yaw)
{
	// Engine basic shapes are 100 units across with the pivot at the centre.
	ISM->AddInstance(FTransform(FRotator(0.f, Yaw, 0.f), Center, Size / 100.f));
}

FVector AOutpostBuilder::ToWorld(const FVector& Local) const
{
	return GetActorTransform().TransformPosition(Local);
}

void AOutpostBuilder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildGeometry();
}

void AOutpostBuilder::BeginPlay()
{
	Super::BeginPlay();
	BuildRuntimeActors();
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

void AOutpostBuilder::BuildGeometry()
{
	using namespace TransoxianaVisuals;

	struct FIsmSetup { UInstancedStaticMeshComponent* ISM; UStaticMesh* Mesh; FLinearColor Color; };
	const FLinearColor PoplarGreen = FLinearColor::FromSRGBColor(FColor(0x5C, 0x7A, 0x3A));
	const FIsmSetup Setups[] = {
		{ GroundISM, GetCube(), Ground },
		{ MudBrickISM, GetCube(), MudBrick },
		{ TowerISM, GetCylinder(), MudBrickDark },
		{ FiredBrickISM, GetCube(), FiredBrick },
		{ PlasterISM, GetCube(), Plaster },
		{ TileISM, GetCube(), Turquoise },
		{ DomeISM, GetSphere(), Turquoise },
		{ DrumISM, GetCylinder(), FiredBrick },
		{ WoodISM, GetCube(), Wood },
		{ HayISM, GetCube(), Hay },
		{ TentISM, GetCone(), Madder },
		{ WellISM, GetCylinder(), MudBrickDark },
		{ RockISM, GetCube(), MudBrickDark },
		{ FoliageISM, GetSphere(), PoplarGreen },
	};
	for (const FIsmSetup& Setup : Setups)
	{
		Setup.ISM->ClearInstances();
		Setup.ISM->SetStaticMesh(Setup.Mesh);
		if (UMaterialInstanceDynamic* MID = MakeColorMaterial(this, Setup.Color))
		{
			Setup.ISM->SetMaterial(0, MID);
		}
	}

	const float H = HalfSize;
	const float T = 120.f;			// wall thickness
	const float WH = WallHeight;
	const float Z = WH * 0.5f;

	// Ground (top surface at Z = 0)
	AddBox(GroundISM, FVector(0.f, 0.f, -50.f), FVector(40000.f, 40000.f, 100.f));

	// --- Enclosure walls ---
	// North and east: solid
	AddBox(MudBrickISM, FVector(H, 0.f, Z), FVector(T, 2.f * H, WH));
	AddBox(MudBrickISM, FVector(0.f, H, Z), FVector(2.f * H, T, WH));
	// South: gate gap between Y = -300 and +300
	AddBox(MudBrickISM, FVector(-H, (-H - 300.f) * 0.5f, Z), FVector(T, H - 300.f, WH));
	AddBox(MudBrickISM, FVector(-H, (H + 300.f) * 0.5f, Z), FVector(T, H - 300.f, WH));
	// West: collapsed breach between X = -900 and -200 (the player's way in)
	AddBox(MudBrickISM, FVector((-H - 900.f) * 0.5f, -H, Z), FVector(H - 900.f, T, WH));
	AddBox(MudBrickISM, FVector((H - 200.f) * 0.5f, -H, Z), FVector(H + 200.f, T, WH));
	AddBox(MudBrickISM, FVector(-850.f, -H, 90.f), FVector(100.f, 160.f, 180.f), 8.f);
	AddBox(MudBrickISM, FVector(-250.f, -H, 70.f), FVector(100.f, 160.f, 140.f), -6.f);
	AddBox(RockISM, FVector(-600.f, -H + 160.f, 15.f), FVector(90.f, 60.f, 30.f), 20.f);
	AddBox(RockISM, FVector(-480.f, -H - 170.f, 15.f), FVector(70.f, 80.f, 30.f), -15.f);

	// Gate portal (fired brick) on the south wall
	AddBox(FiredBrickISM, FVector(-H, -420.f, 400.f), FVector(260.f, 240.f, 800.f));
	AddBox(FiredBrickISM, FVector(-H, 420.f, 400.f), FVector(260.f, 240.f, 800.f));
	AddBox(FiredBrickISM, FVector(-H, 0.f, 725.f), FVector(260.f, 600.f, 150.f));
	AddBox(TileISM, FVector(-H - 135.f, 0.f, 760.f), FVector(20.f, 560.f, 50.f));

	// Corner towers (archers stand on top at Z = 800)
	for (const FVector2D& Corner : { FVector2D(H, H), FVector2D(H, -H), FVector2D(-H, H), FVector2D(-H, -H) })
	{
		AddBox(TowerISM, FVector(Corner.X, Corner.Y, 400.f), FVector(700.f, 700.f, 800.f));
	}

	// --- Domed hall (commander's quarters), against the north wall ---
	const float HallMaxX = H - T * 0.5f;
	const float HallCenterX = (HallMinX + HallMaxX) * 0.5f;
	const float HallLength = HallMaxX - HallMinX;
	const float W = HallHalfWidth;
	AddBox(PlasterISM, FVector(HallCenterX, -W, 350.f), FVector(HallLength, 80.f, 700.f));
	AddBox(PlasterISM, FVector(HallCenterX, W, 350.f), FVector(HallLength, 80.f, 700.f));
	AddBox(PlasterISM, FVector(HallMinX, -475.f, 350.f), FVector(80.f, 650.f, 700.f));	// south facade, door gap |Y| < 150
	AddBox(PlasterISM, FVector(HallMinX, 475.f, 350.f), FVector(80.f, 650.f, 700.f));
	AddBox(PlasterISM, FVector(HallMinX, 0.f, 500.f), FVector(80.f, 300.f, 400.f));		// over the door (opening 3 m high)
	AddBox(PlasterISM, FVector(HallCenterX, 0.f, 720.f), FVector(HallLength, 2.f * W + 80.f, 40.f)); // roof
	AddBox(DrumISM, FVector(HallCenterX, 0.f, 965.f), FVector(1040.f, 1040.f, 450.f));
	AddBox(DomeISM, FVector(HallCenterX, 0.f, 1190.f), FVector(1000.f, 1000.f, 900.f));
	// Portal (pishtaq) frame and a glazed tile band
	AddBox(FiredBrickISM, FVector(HallMinX - 60.f, -300.f, 450.f), FVector(120.f, 160.f, 900.f));
	AddBox(FiredBrickISM, FVector(HallMinX - 60.f, 300.f, 450.f), FVector(120.f, 160.f, 900.f));
	AddBox(FiredBrickISM, FVector(HallMinX - 60.f, 0.f, 870.f), FVector(120.f, 760.f, 120.f));
	AddBox(TileISM, FVector(HallMinX - 125.f, 0.f, 940.f), FVector(20.f, 760.f, 60.f));
	// Chest holding the dispatch
	AddBox(WoodISM, FVector(DispatchLocal.X, 0.f, 40.f), FVector(100.f, 160.f, 80.f));

	// --- Rooms along the east wall, with shady alleys between them ---
	const float RoomY = H - T * 0.5f - 300.f;
	for (const float X : { -2800.f, -1800.f, -800.f, 200.f, 1200.f })
	{
		AddBox(PlasterISM, FVector(X, RoomY, 175.f), FVector(800.f, 600.f, 350.f));
	}

	// --- Open shed against the west wall (shade = hiding spot) ---
	for (const float X : { 400.f, 1300.f, 2200.f })
	{
		AddBox(WoodISM, FVector(X, -2700.f, 150.f), FVector(30.f, 30.f, 300.f));
	}
	AddBox(WoodISM, FVector(1300.f, -3070.f, 310.f), FVector(2000.f, 800.f, 20.f));
	AddBox(HayISM, FVector(800.f, -3200.f, 90.f), FVector(300.f, 200.f, 180.f));
	AddBox(HayISM, FVector(1800.f, -3250.f, 90.f), FVector(300.f, 200.f, 180.f), 10.f);

	// --- Cloth awning by the gate (inside, east side) ---
	for (const float Y : { 900.f, 1800.f })
	{
		AddBox(WoodISM, FVector(-2980.f, Y, 140.f), FVector(30.f, 30.f, 280.f));
	}
	AddBox(TentISM, FVector(-3200.f, 1350.f, 290.f), FVector(500.f, 1200.f, 20.f)); // flat red cloth

	// --- Courtyard: hay stacks (hiding spots), tents, well, crates ---
	for (const FVector& Hay : { FVector(-2600.f, -2400.f, 0.f), FVector(-1200.f, -400.f, 0.f), FVector(300.f, 1600.f, 0.f), FVector(1200.f, -600.f, 0.f) })
	{
		AddBox(HayISM, FVector(Hay.X, Hay.Y, 90.f), FVector(320.f, 220.f, 180.f), 15.f);
	}
	for (const FVector2D& Tent : { FVector2D(-1800.f, 1600.f), FVector2D(-600.f, 2000.f), FVector2D(-2650.f, 700.f) })
	{
		AddBox(TentISM, FVector(Tent.X, Tent.Y, 225.f), FVector(600.f, 600.f, 450.f));
	}
	AddBox(WellISM, FVector(0.f, -1200.f, 50.f), FVector(260.f, 260.f, 100.f));
	AddBox(WoodISM, FVector(-1300.f, 750.f, 50.f), FVector(100.f, 100.f, 100.f));
	AddBox(WoodISM, FVector(-1200.f, 840.f, 50.f), FVector(100.f, 100.f, 100.f), 20.f);
	AddBox(WoodISM, FVector(-1250.f, 790.f, 150.f), FVector(100.f, 100.f, 100.f), 40.f);

	// --- Outside: approach rocks and a row of poplars along an irrigation ditch ---
	AddBox(RockISM, FVector(-700.f, -H - 1200.f, 60.f), FVector(300.f, 200.f, 120.f), 25.f);
	AddBox(RockISM, FVector(-200.f, -H - 900.f, 50.f), FVector(200.f, 250.f, 100.f), -10.f);
	AddBox(RockISM, FVector(-900.f, -H - 2600.f, 80.f), FVector(400.f, 300.f, 160.f), 5.f);
	for (float Y = -3000.f; Y <= 3000.f; Y += 600.f)
	{
		const FVector Base(-H - 1500.f, Y, 0.f);
		AddBox(WoodISM, Base + FVector(0.f, 0.f, 200.f), FVector(40.f, 40.f, 400.f));
		AddBox(FoliageISM, Base + FVector(0.f, 0.f, 750.f), FVector(220.f, 220.f, 900.f));
	}

	// --- Torches ---
	HallTorch->SetRelativeLocation(FVector(HallCenterX - 300.f, 0.f, 400.f));
	GateTorchA->SetRelativeLocation(FVector(-H + 200.f, -420.f, 350.f));
	GateTorchB->SetRelativeLocation(FVector(-H + 200.f, 420.f, 350.f));
	CourtyardBrazier->SetRelativeLocation(FVector(-300.f, 300.f, 150.f));
	AddBox(FiredBrickISM, FVector(-300.f, 300.f, 50.f), FVector(80.f, 80.f, 100.f));
}

// ---------------------------------------------------------------------------
// Runtime actors
// ---------------------------------------------------------------------------

void AOutpostBuilder::BuildRuntimeActors()
{
	UWorld* World = GetWorld();
	if (bRuntimeBuilt || !World || !World->IsGameWorld())
	{
		return;
	}
	bRuntimeBuilt = true;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Player starts on the ridge west of the breach, facing east toward the outpost.
	SpawnedPlayerStart = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), ToWorld(PlayerStartLocal),
		FRotator(0.f, GetActorRotation().Yaw + 90.f, 0.f), Params);

	SpawnedDispatch = World->SpawnActor<ADispatchPickup>(ADispatchPickup::StaticClass(), ToWorld(DispatchLocal), GetActorRotation(), Params);
	SpawnedExtraction = World->SpawnActor<AExtractionZone>(AExtractionZone::StaticClass(),
		ToWorld(FVector(HalfSize + 1500.f, HalfSize + 1500.f, 190.f)), GetActorRotation(), Params);

	SpawnHidingZones();
	if (bSpawnLightingIfMissing)
	{
		SpawnLighting();
	}
	if (bSpawnGuards)
	{
		SpawnGuards();
	}
}

AActor* AOutpostBuilder::GetPlayerStart() const
{
	return SpawnedPlayerStart;
}

FVector AOutpostBuilder::GetDispatchLocation() const
{
	return ToWorld(DispatchLocal);
}

FVector AOutpostBuilder::GetExtractionLocation() const
{
	return ToWorld(FVector(HalfSize + 1500.f, HalfSize + 1500.f, 190.f));
}

void AOutpostBuilder::SpawnHidingZones()
{
	struct FZone { FVector Center; FVector Extent; };
	const float H = HalfSize;
	const FZone Zones[] = {
		// Hay stacks in the courtyard
		{ FVector(-2600.f, -2400.f, 100.f), FVector(260.f, 200.f, 100.f) },
		{ FVector(-1200.f, -400.f, 100.f), FVector(260.f, 200.f, 100.f) },
		{ FVector(300.f, 1600.f, 100.f), FVector(260.f, 200.f, 100.f) },
		{ FVector(1200.f, -600.f, 100.f), FVector(260.f, 200.f, 100.f) },
		// Under the west shed roof
		{ FVector(1300.f, -3070.f, 100.f), FVector(950.f, 350.f, 100.f) },
		// Under the awning by the gate
		{ FVector(-3200.f, 1350.f, 100.f), FVector(230.f, 580.f, 100.f) },
		// Shady alleys between the east rooms
		{ FVector(-2300.f, H - 360.f, 100.f), FVector(100.f, 300.f, 100.f) },
		{ FVector(-1300.f, H - 360.f, 100.f), FVector(100.f, 300.f, 100.f) },
		{ FVector(-300.f, H - 360.f, 100.f), FVector(100.f, 300.f, 100.f) },
		{ FVector(700.f, H - 360.f, 100.f), FVector(100.f, 300.f, 100.f) },
		// Rocks outside the breach
		{ FVector(-700.f, -H - 1200.f, 100.f), FVector(260.f, 220.f, 100.f) },
	};

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FZone& Zone : Zones)
	{
		if (AStealthZone* Spawned = GetWorld()->SpawnActor<AStealthZone>(AStealthZone::StaticClass(), ToWorld(Zone.Center), GetActorRotation(), Params))
		{
			Spawned->SetExtent(Zone.Extent);
		}
	}
}

AActor* AOutpostBuilder::SpawnEnemy(EEnemyType Type, const FVector& LocalLocation, float Yaw, const TArray<FVector>& LocalPatrol)
{
	const FTransform SpawnTransform(FRotator(0.f, GetActorRotation().Yaw + Yaw, 0.f), ToWorld(LocalLocation));
	AEnemyCharacter* Enemy = GetWorld()->SpawnActorDeferred<AEnemyCharacter>(AEnemyCharacter::StaticClass(), SpawnTransform, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Enemy)
	{
		return nullptr;
	}
	Enemy->EnemyType = Type;
	for (const FVector& Point : LocalPatrol)
	{
		Enemy->PatrolPoints.Add(ToWorld(Point));
	}
	Enemy->FinishSpawning(SpawnTransform);
	return Enemy;
}

void AOutpostBuilder::SpawnGuards()
{
	const float H = HalfSize;
	const float GroundZ = 100.f;
	const float TowerTopZ = 800.f + 100.f;

	// Courtyard loop
	SpawnEnemy(EEnemyType::Swordsman, FVector(-2000.f, -1800.f, GroundZ), 0.f,
		{ FVector(-2000.f, -1800.f, 0.f), FVector(-2000.f, 1200.f, 0.f), FVector(800.f, 1200.f, 0.f), FVector(800.f, -1800.f, 0.f) });
	// Along the west side, past the shed
	SpawnEnemy(EEnemyType::Swordsman, FVector(2600.f, -2300.f, GroundZ), 180.f,
		{ FVector(2600.f, -2300.f, 0.f), FVector(-1500.f, -2300.f, 0.f) });
	// In front of the domed hall
	SpawnEnemy(EEnemyType::Swordsman, FVector(1500.f, -1300.f, GroundZ), 90.f,
		{ FVector(1500.f, -1300.f, 0.f), FVector(1500.f, 1300.f, 0.f) });
	// Outside the walls: south-west corner, sweeping past the breach approach
	SpawnEnemy(EEnemyType::Swordsman, FVector(-H - 700.f, H - 500.f, GroundZ), -90.f,
		{ FVector(-H - 700.f, H - 500.f, 0.f), FVector(-H - 700.f, -H - 700.f, 0.f), FVector(2500.f, -H - 700.f, 0.f), FVector(-H - 700.f, -H - 700.f, 0.f) });
	// Static posts: inside the gate, and at the hall door
	SpawnEnemy(EEnemyType::Swordsman, FVector(-H + 400.f, 0.f, GroundZ), 180.f, {});
	SpawnEnemy(EEnemyType::Swordsman, FVector(HallMinX - 250.f, 250.f, GroundZ), 180.f, {});
	// Archers on the north-west and south-east towers, looking into the courtyard
	SpawnEnemy(EEnemyType::Archer, FVector(H, -H, TowerTopZ), 135.f, {});
	SpawnEnemy(EEnemyType::Archer, FVector(-H, H, TowerTopZ), -45.f, {});
}

void AOutpostBuilder::SpawnLighting()
{
	UWorld* World = GetWorld();
	if (TActorIterator<ADirectionalLight>(World))
	{
		return; // The level already has a sun: keep the artist's lighting.
	}

	FActorSpawnParameters Params;
	Params.Owner = this;

	// Low dusk sun in the west, shining east: long shadows across the courtyard.
	if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0.f, 0.f, 2000.f), FRotator(-9.f, 80.f, 0.f), Params))
	{
		if (UDirectionalLightComponent* SunLight = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			SunLight->SetMobility(EComponentMobility::Movable);
			SunLight->SetIntensity(5.f);
			SunLight->SetLightColor(FLinearColor(1.f, 0.72f, 0.5f));
			SunLight->SetAtmosphereSunLight(true);
		}
	}

	World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FTransform::Identity, Params);

	if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FTransform::Identity, Params))
	{
		if (USkyLightComponent* SkyLight = Sky->GetLightComponent())
		{
			SkyLight->SetMobility(EComponentMobility::Movable);
			SkyLight->bRealTimeCapture = true;
			SkyLight->MarkRenderStateDirty();
			SkyLight->RecaptureSky();
		}
	}

	if (AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FTransform::Identity, Params))
	{
		if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
		{
			FogComponent->SetFogDensity(0.03f);
			FogComponent->SetFogHeightFalloff(0.4f);
			FogComponent->SetVolumetricFog(true);
		}
	}
	UE_LOG(LogTransoxiana, Log, TEXT("Level had no sun - spawned dusk lighting (sun, sky atmosphere, sky light, fog)."));
}
