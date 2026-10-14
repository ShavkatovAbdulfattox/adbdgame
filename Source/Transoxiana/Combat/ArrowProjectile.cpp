#include "Combat/ArrowProjectile.h"

#include "AI/EnemyCharacter.h"
#include "Components/SphereComponent.h"
#include "Engine/CollisionProfile.h"
#include "Components/StaticMeshComponent.h"
#include "Core/TransoxianaGameMode.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/TransoxianaCharacterBase.h"
#include "World/TransoxianaVisuals.h"

AArrowProjectile::AArrowProjectile()
{
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(4.f);
	Collision->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->SetCanEverAffectNavigation(false);
	RootComponent = Collision;

	Shaft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shaft"));
	Shaft->SetupAttachment(Collision);
	// Cylinder is 100 units tall along Z; lay it along X and make it thin.
	Shaft->SetRelativeLocation(FVector(-35.f, 0.f, 0.f));
	Shaft->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Shaft->SetRelativeScale3D(FVector(0.015f, 0.015f, 0.75f));
	Shaft->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 0.f;
	Movement->MaxSpeed = 8000.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->ProjectileGravityScale = 0.6f;

	InitialLifeSpan = 20.f;
}

void AArrowProjectile::BeginPlay()
{
	Super::BeginPlay();
	TransoxianaVisuals::SetupShape(Shaft, TransoxianaVisuals::GetCylinder(), TransoxianaVisuals::Wood, this);
	Collision->OnComponentHit.AddDynamic(this, &AArrowProjectile::OnArrowHit);
	if (AActor* Shooter = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}
}

void AArrowProjectile::Launch(const FVector& Velocity, float InDamage, bool bInFiredByPlayer)
{
	Damage = InDamage;
	bFiredByPlayer = bInFiredByPlayer;
	Movement->Velocity = Velocity;
	if (AActor* Shooter = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}
}

void AArrowProjectile::OnArrowHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasHit)
	{
		return;
	}
	bHasHit = true;

	if (ATransoxianaCharacterBase* Victim = Cast<ATransoxianaCharacterBase>(OtherActor))
	{
		// Guards' arrows don't hurt other guards.
		const bool bFriendlyFire = !bFiredByPlayer && Cast<AEnemyCharacter>(Victim) != nullptr;
		if (Victim != GetInstigator() && !bFriendlyFire)
		{
			const bool bHeadshot = Hit.ImpactPoint.Z > Victim->GetHeadLocation().Z - 30.f;
			const float FinalDamage = bHeadshot ? Damage * HeadshotMultiplier : Damage;
			AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
			UGameplayStatics::ApplyDamage(Victim, FinalDamage, InstigatorController, this, UDamageType::StaticClass());
		}
		Destroy();
		return;
	}

	// Stick into walls/ground.
	Movement->StopMovementImmediately();
	Movement->ProjectileGravityScale = 0.f;
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLifeSpan(10.f);

	if (bFiredByPlayer)
	{
		if (ATransoxianaGameMode* GM = GetWorld()->GetAuthGameMode<ATransoxianaGameMode>())
		{
			GM->ReportNoise(Hit.ImpactPoint, ImpactNoiseRadius, this);
		}
	}
}
