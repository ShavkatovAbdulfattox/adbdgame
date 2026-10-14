#include "World/StealthZone.h"

#include "Components/BoxComponent.h"
#include "Player/TransoxianaCharacter.h"

AStealthZone::AStealthZone()
{
	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	Volume->InitBoxExtent(FVector(200.f, 200.f, 100.f));
	Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Volume->SetCollisionResponseToAllChannels(ECR_Ignore);
	Volume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Volume->SetGenerateOverlapEvents(true);
	Volume->SetCanEverAffectNavigation(false);
	Volume->ShapeColor = FColor(40, 200, 120);
	RootComponent = Volume;

	Volume->OnComponentBeginOverlap.AddDynamic(this, &AStealthZone::OnBeginOverlap);
	Volume->OnComponentEndOverlap.AddDynamic(this, &AStealthZone::OnEndOverlap);
}

void AStealthZone::SetExtent(const FVector& HalfExtent)
{
	Volume->SetBoxExtent(HalfExtent, true);
}

void AStealthZone::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ATransoxianaCharacter* Player = Cast<ATransoxianaCharacter>(OtherActor))
	{
		Player->EnterStealthZone();
	}
}

void AStealthZone::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ATransoxianaCharacter* Player = Cast<ATransoxianaCharacter>(OtherActor))
	{
		Player->LeaveStealthZone();
	}
}
