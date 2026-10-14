#include "Mission/ExtractionZone.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/TransoxianaGameMode.h"
#include "Engine/World.h"
#include "Player/TransoxianaCharacter.h"
#include "World/TransoxianaVisuals.h"

AExtractionZone::AExtractionZone()
{
	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	Volume->InitBoxExtent(FVector(300.f, 300.f, 200.f));
	Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Volume->SetCollisionResponseToAllChannels(ECR_Ignore);
	Volume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Volume->SetGenerateOverlapEvents(true);
	RootComponent = Volume;
	Volume->OnComponentBeginOverlap.AddDynamic(this, &AExtractionZone::OnBeginOverlap);

	// Placeholder figure of Aybike with a falcon on her arm.
	FalconerBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FalconerBody"));
	FalconerBody->SetupAttachment(Volume);
	FalconerBody->SetRelativeLocation(FVector(0.f, 0.f, -110.f));
	FalconerBody->SetRelativeScale3D(FVector(0.55f, 0.45f, 1.6f));

	FalconerHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FalconerHead"));
	FalconerHead->SetupAttachment(Volume);
	FalconerHead->SetRelativeLocation(FVector(0.f, 0.f, -15.f));
	FalconerHead->SetRelativeScale3D(FVector(0.25f));

	Falcon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Falcon"));
	Falcon->SetupAttachment(Volume);
	Falcon->SetRelativeLocation(FVector(10.f, 45.f, -40.f));
	Falcon->SetRelativeScale3D(FVector(0.18f, 0.12f, 0.22f));

	Lantern = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lantern"));
	Lantern->SetupAttachment(Volume);
	Lantern->SetRelativeLocation(FVector(40.f, 0.f, 0.f));
	Lantern->SetMobility(EComponentMobility::Movable);
	Lantern->SetIntensityUnits(ELightUnits::Candelas);
	Lantern->SetIntensity(20.f);
	Lantern->SetAttenuationRadius(900.f);
	Lantern->SetLightColor(FLinearColor(1.f, 0.6f, 0.3f));
}

void AExtractionZone::BeginPlay()
{
	Super::BeginPlay();
	TransoxianaVisuals::SetupShape(FalconerBody, TransoxianaVisuals::GetCylinder(), TransoxianaVisuals::Cobalt, this);
	TransoxianaVisuals::SetupShape(FalconerHead, TransoxianaVisuals::GetSphere(), TransoxianaVisuals::Skin, this);
	TransoxianaVisuals::SetupShape(Falcon, TransoxianaVisuals::GetSphere(), TransoxianaVisuals::Wood, this);
}

void AExtractionZone::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<ATransoxianaCharacter>(OtherActor))
	{
		if (ATransoxianaGameMode* GM = GetWorld()->GetAuthGameMode<ATransoxianaGameMode>())
		{
			// Arriving without the dispatch does nothing; the HUD objective explains why.
			if (GM->GetMissionState() == EMissionState::Escape)
			{
				GM->OnPlayerExtracted();
			}
		}
	}
}
