#include "Mission/DispatchPickup.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/TransoxianaGameMode.h"
#include "Engine/World.h"
#include "World/TransoxianaVisuals.h"

ADispatchPickup::ADispatchPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupRadius = CreateDefaultSubobject<USphereComponent>(TEXT("PickupRadius"));
	PickupRadius->InitSphereRadius(160.f);
	PickupRadius->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupRadius->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupRadius->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupRadius->SetGenerateOverlapEvents(true);
	RootComponent = PickupRadius;

	// A rolled letter (cylinder) with a wax seal (sphere).
	Scroll = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Scroll"));
	Scroll->SetupAttachment(PickupRadius);
	Scroll->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
	Scroll->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.3f));

	Seal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Seal"));
	Seal->SetupAttachment(Scroll);
	Seal->SetRelativeLocation(FVector(0.f, 50.f, 0.f));
	Seal->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.15f));

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(PickupRadius);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Glow->SetMobility(EComponentMobility::Movable);
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(6.f);
	Glow->SetAttenuationRadius(300.f);
	Glow->SetLightColor(FLinearColor(1.f, 0.75f, 0.4f));
	Glow->SetCastShadows(false);
}

void ADispatchPickup::BeginPlay()
{
	Super::BeginPlay();
	TransoxianaVisuals::SetupShape(Scroll, TransoxianaVisuals::GetCylinder(), TransoxianaVisuals::Plaster, this);
	TransoxianaVisuals::SetupShape(Seal, TransoxianaVisuals::GetSphere(), TransoxianaVisuals::Madder, this);
}

void ADispatchPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	Scroll->SetRelativeLocation(FVector(0.f, 0.f, 8.f * FMath::Sin(Time * 2.f)));
	Glow->SetIntensity(5.f + 2.f * FMath::Sin(Time * 3.f));
}

void ADispatchPickup::Collect(ATransoxianaCharacter* Collector)
{
	if (ATransoxianaGameMode* GM = GetWorld()->GetAuthGameMode<ATransoxianaGameMode>())
	{
		GM->OnDispatchTaken();
	}
	Destroy();
}
