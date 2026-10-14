#include "Player/TransoxianaCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "World/TransoxianaVisuals.h"

ATransoxianaCharacterBase::ATransoxianaCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.f);

	// Mannequin art (if present) faces +Y in its asset; rotate so it faces +X like the capsule.
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));

	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetRelativeLocation(FVector(0.f, 0.f, -10.f));
	PlaceholderBody->SetRelativeScale3D(FVector(0.6f, 0.5f, 1.6f));
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PlaceholderHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderHead"));
	PlaceholderHead->SetupAttachment(GetCapsuleComponent());
	PlaceholderHead->SetRelativeLocation(FVector(0.f, 0.f, 78.f));
	PlaceholderHead->SetRelativeScale3D(FVector(0.26f));
	PlaceholderHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// The sword hangs at the right hand position and swings procedurally until
	// real attack animations exist (milestone M2).
	SwordPivot = CreateDefaultSubobject<USceneComponent>(TEXT("SwordPivot"));
	SwordPivot->SetupAttachment(GetCapsuleComponent());
	SwordPivot->SetRelativeLocation(FVector(10.f, 38.f, 10.f));

	SwordBlade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordBlade"));
	SwordBlade->SetupAttachment(SwordPivot);
	SwordBlade->SetRelativeLocation(FVector(45.f, 0.f, 0.f));
	SwordBlade->SetRelativeScale3D(FVector(0.9f, 0.04f, 0.015f));
	SwordBlade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BowStave = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BowStave"));
	BowStave->SetupAttachment(GetCapsuleComponent());
	BowStave->SetRelativeLocation(FVector(-18.f, -22.f, 20.f));
	BowStave->SetRelativeRotation(FRotator(0.f, 0.f, 15.f));
	BowStave->SetRelativeScale3D(FVector(0.03f, 0.03f, 1.1f));
	BowStave->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATransoxianaCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
}

void ATransoxianaCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Placeholder body shrinks when crouched so stealth is readable without animations.
	if (!bHasSkeletalMeshArt && !bDead && PlaceholderBody)
	{
		const float TargetScaleZ = bIsCrouched ? PlaceholderStandingScaleZ * 0.55f : PlaceholderStandingScaleZ;
		FVector Scale = PlaceholderBody->GetRelativeScale3D();
		Scale.Z = FMath::FInterpTo(Scale.Z, TargetScaleZ, DeltaSeconds, 12.f);
		PlaceholderBody->SetRelativeScale3D(Scale);
		PlaceholderHead->SetRelativeLocation(FVector(0.f, 0.f, bIsCrouched ? 20.f : 78.f));
	}
}

FVector ATransoxianaCharacterBase::GetHeadLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 10.f);
}

void ATransoxianaCharacterBase::SetupAppearance(const FLinearColor& BodyColor)
{
	USkeletalMesh* MeshAsset = Cast<USkeletalMesh>(TransoxianaVisuals::TryLoadOptional(CharacterMeshPath));
	if (!MeshAsset)
	{
		MeshAsset = Cast<USkeletalMesh>(TransoxianaVisuals::TryLoadOptional(FallbackCharacterMeshPath));
	}

	if (MeshAsset)
	{
		GetMesh()->SetSkeletalMeshAsset(MeshAsset);
		if (UClass* AnimClass = Cast<UClass>(TransoxianaVisuals::TryLoadOptional(AnimClassPath)))
		{
			GetMesh()->SetAnimInstanceClass(AnimClass);
		}
		bHasSkeletalMeshArt = true;
	}

	PlaceholderStandingScaleZ = PlaceholderBody->GetRelativeScale3D().Z;
	TransoxianaVisuals::SetupShape(PlaceholderBody, TransoxianaVisuals::GetCylinder(), BodyColor, this);
	TransoxianaVisuals::SetupShape(PlaceholderHead, TransoxianaVisuals::GetSphere(), TransoxianaVisuals::Skin, this);
	PlaceholderBody->SetVisibility(!bHasSkeletalMeshArt);
	PlaceholderHead->SetVisibility(!bHasSkeletalMeshArt);

	TransoxianaVisuals::SetupShape(SwordBlade, TransoxianaVisuals::GetCube(), TransoxianaVisuals::Steel, this);
	TransoxianaVisuals::SetupShape(BowStave, TransoxianaVisuals::GetCylinder(), TransoxianaVisuals::Wood, this);
	PoseSword(0.f, false, false);
}

void ATransoxianaCharacterBase::PoseSword(float SwingAlpha, bool bMirror, bool bGuard)
{
	if (!SwordPivot)
	{
		return;
	}
	FRotator Rotation;
	if (bGuard)
	{
		// Blade held across the body.
		Rotation = FRotator(20.f, -70.f, 0.f);
	}
	else if (SwingAlpha <= 0.f)
	{
		// Resting: blade pointing forward and down.
		Rotation = FRotator(-35.f, 10.f, 0.f);
	}
	else
	{
		// Horizontal arc in front of the body.
		const float Start = bMirror ? -80.f : 80.f;
		const float End = -Start;
		const float Yaw = FMath::Lerp(Start, End, FMath::Clamp(SwingAlpha, 0.f, 1.f));
		Rotation = FRotator(-5.f, Yaw - 40.f, 0.f);
	}
	SwordPivot->SetRelativeRotation(Rotation);
}

void ATransoxianaCharacterBase::PlayDeathPose()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordBlade->SetVisibility(false);
	BowStave->SetVisibility(false);

	if (bHasSkeletalMeshArt && GetMesh()->GetPhysicsAsset())
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
		return;
	}

	// Placeholder: lie down on the ground.
	PlaceholderBody->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -75.f), FRotator(90.f, 0.f, 0.f));
	PlaceholderHead->SetRelativeLocation(FVector(-75.f, 0.f, -75.f));
}
