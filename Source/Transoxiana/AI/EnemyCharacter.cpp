#include "AI/EnemyCharacter.h"

#include "AI/EnemyAIController.h"
#include "Combat/ArrowProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/TransoxianaGameMode.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Player/TransoxianaCharacter.h"
#include "World/TransoxianaVisuals.h"

AEnemyCharacter::AEnemyCharacter()
{
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// The AI controller sets a focal point; the body turns toward it smoothly.
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	Move->bUseControllerDesiredRotation = true;
	Move->RotationRate = FRotator(0.f, 300.f, 0.f);
	Move->MaxWalkSpeed = PatrolSpeed;

	// Conical helmet with a red crest: a readable raider silhouette.
	Helmet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Helmet"));
	Helmet->SetupAttachment(GetCapsuleComponent());
	Helmet->SetRelativeLocation(FVector(0.f, 0.f, 96.f));
	Helmet->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.32f));
	Helmet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (EnemyType == EEnemyType::Archer)
	{
		MaxHealth = 70.f;
		Health = MaxHealth;
	}

	SetupAppearance(TransoxianaVisuals::Madder);
	TransoxianaVisuals::SetupShape(Helmet, TransoxianaVisuals::GetCone(), TransoxianaVisuals::Steel, this);
	SwordBlade->SetVisibility(EnemyType == EEnemyType::Swordsman);
	BowStave->SetVisibility(EnemyType == EEnemyType::Archer);

	if (!GetController())
	{
		SpawnDefaultController();
	}
	if (ATransoxianaGameMode* GM = GetWorld()->GetAuthGameMode<ATransoxianaGameMode>())
	{
		GM->RegisterEnemy(this);
	}
}

AEnemyAIController* AEnemyCharacter::GetEnemyController() const
{
	return Cast<AEnemyAIController>(GetController());
}

EEnemyState AEnemyCharacter::GetEnemyState() const
{
	if (bDead)
	{
		return EEnemyState::Dead;
	}
	const AEnemyAIController* AI = GetEnemyController();
	return AI ? AI->GetState() : EEnemyState::Patrol;
}

float AEnemyCharacter::GetAwareness() const
{
	const AEnemyAIController* AI = GetEnemyController();
	return AI ? AI->GetAwareness() : 0.f;
}

bool AEnemyCharacter::IsStaggered() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < StaggeredUntil;
}

bool AEnemyCharacter::CanBeTakenDown() const
{
	if (bDead)
	{
		return false;
	}
	const EEnemyState State = GetEnemyState();
	return State != EEnemyState::Combat || IsStaggered();
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDead)
	{
		return;
	}

	if (bAttacking)
	{
		AttackElapsed += DeltaSeconds;
		// Telegraph: raise the blade during the wind-up, then swing.
		if (AttackElapsed < AttackWindup)
		{
			PoseSword(0.f, false, true);
		}
		else
		{
			PoseSword(FMath::Clamp((AttackElapsed - AttackWindup) / 0.25f, 0.01f, 1.f), false, false);
		}

		if (!bAttackHitApplied && AttackElapsed >= AttackWindup + 0.08f)
		{
			bAttackHitApplied = true;
			if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
			{
				const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
				const bool bInReach = ToPlayer.Size2D() <= AttackReach && FMath::Abs(ToPlayer.Z) < 120.f;
				const bool bInFront = FVector::DotProduct(GetActorForwardVector(), ToPlayer.GetSafeNormal2D()) > 0.4f;
				if (bInReach && bInFront)
				{
					// DamageCauser = this enemy, so the player can block/parry it.
					UGameplayStatics::ApplyDamage(Player, SwordDamage, GetController(), this, UDamageType::StaticClass());
				}
			}
		}

		if (AttackElapsed >= AttackWindup + AttackRecovery)
		{
			bAttacking = false;
			PoseSword(0.f, false, false);
		}
	}
	else if (EnemyType == EEnemyType::Swordsman)
	{
		PoseSword(0.f, false, GetEnemyState() == EEnemyState::Combat && !IsStaggered());
	}

	const bool bChasing = GetEnemyState() == EEnemyState::Combat || GetEnemyState() == EEnemyState::Search;
	GetCharacterMovement()->MaxWalkSpeed = IsStaggered() ? 0.f : (bAttacking ? 60.f : (bChasing ? ChaseSpeed : PatrolSpeed));
}

void AEnemyCharacter::StartMeleeAttack()
{
	if (bDead || bAttacking || IsStaggered())
	{
		return;
	}
	bAttacking = true;
	bAttackHitApplied = false;
	AttackElapsed = 0.f;
}

void AEnemyCharacter::ShootArrowAt(const AActor* Target)
{
	if (bDead || !Target || IsStaggered())
	{
		return;
	}
	const FVector Start = GetActorLocation() + GetActorForwardVector() * 50.f + FVector(0.f, 0.f, 50.f);
	FVector Aim = Target->GetActorLocation() + FVector(0.f, 0.f, 20.f);

	// Compensate for gravity drop and add some human error.
	const float Speed = 3800.f;
	const float Distance = FVector::Dist(Start, Aim);
	const float Gravity = 980.f * 0.6f; // matches arrow gravity scale
	const float FlightTime = Distance / Speed;
	Aim.Z += 0.5f * Gravity * FlightTime * FlightTime;
	Aim += FMath::VRand() * FMath::Min(120.f, Distance * 0.03f);

	const FVector Direction = (Aim - Start).GetSafeNormal();
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AArrowProjectile* Arrow = GetWorld()->SpawnActor<AArrowProjectile>(AArrowProjectile::StaticClass(), Start, Direction.Rotation(), Params))
	{
		Arrow->Launch(Direction * Speed, ArrowDamage, false);
	}
}

void AEnemyCharacter::Stagger(float Duration)
{
	bAttacking = false;
	StaggeredUntil = GetWorld()->GetTimeSeconds() + Duration;
	PoseSword(0.f, false, false);
	GetCharacterMovement()->StopMovementImmediately();
}

float AEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead)
	{
		return 0.f;
	}

	APawn* Attacker = EventInstigator ? EventInstigator->GetPawn() : nullptr;
	const bool bMelee = Cast<ATransoxianaCharacter>(DamageCauser) != nullptr;
	const bool bAware = GetEnemyState() == EEnemyState::Combat;

	// Alert swordsmen may block a melee hit from the front.
	if (bMelee && bAware && EnemyType == EEnemyType::Swordsman && !bAttacking && !IsStaggered() && Attacker)
	{
		const FVector ToAttacker = (Attacker->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (FVector::DotProduct(GetActorForwardVector(), ToAttacker) > 0.5f && FMath::FRand() < BlockChance)
		{
			LastBlockTime = GetWorld()->GetTimeSeconds();
			PoseSword(0.f, false, true);
			return 0.f;
		}
	}

	// Staggered (parried) enemies take double damage; unaware ones take triple.
	float Multiplier = 1.f;
	if (IsStaggered())
	{
		Multiplier = 2.f;
	}
	else if (!bAware)
	{
		Multiplier = 3.f;
	}

	const float Applied = Super::TakeDamage(DamageAmount * Multiplier, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - Applied);

	if (Health <= 0.f)
	{
		Die(EventInstigator, false);
	}
	else if (AEnemyAIController* AI = GetEnemyController())
	{
		AI->OnDamaged(Attacker);
	}
	return Applied;
}

void AEnemyCharacter::KillByTakedown(AController* Killer)
{
	Die(Killer, true);
}

void AEnemyCharacter::Die(AController* Killer, bool bTakedown)
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	bAttacking = false;
	Helmet->SetVisibility(false);

	const bool bWasInCombat = GetEnemyController() && GetEnemyController()->GetState() == EEnemyState::Combat;
	if (AEnemyAIController* AI = GetEnemyController())
	{
		AI->OnPawnDied();
	}
	GetCharacterMovement()->DisableMovement();
	PlayDeathPose();
	DetachFromControllerPendingDestroy();

	if (ATransoxianaGameMode* GM = GetWorld()->GetAuthGameMode<ATransoxianaGameMode>())
	{
		GM->NotifyEnemyKilled(this, bTakedown);
		if (!bTakedown && !bWasInCombat)
		{
			// A guard dying loudly makes noise.
			GM->ReportNoise(GetActorLocation(), 700.f, this);
		}
	}
}
