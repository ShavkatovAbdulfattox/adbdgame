#include "AI/EnemyAIController.h"

#include "AI/EnemyCharacter.h"
#include "Core/TransoxianaGameMode.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Player/TransoxianaCharacter.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bSetControlRotationFromPawnOrientation = true;
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (InPawn)
	{
		HomeLocation = InPawn->GetActorLocation();
		HomeRotation = InPawn->GetActorRotation();
	}
	// Spread out the first attacks so guards don't act in perfect sync.
	AttackCooldown = FMath::FRandRange(0.5f, 1.5f);
	ArrowCooldown = FMath::FRandRange(1.0f, 2.0f);
}

AEnemyCharacter* AEnemyAIController::GetEnemy() const
{
	return Cast<AEnemyCharacter>(GetPawn());
}

ATransoxianaCharacter* AEnemyAIController::GetPlayer() const
{
	return Cast<ATransoxianaCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

ATransoxianaGameMode* AEnemyAIController::GetGameMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<ATransoxianaGameMode>() : nullptr;
}

void AEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AEnemyCharacter* Enemy = GetEnemy();
	if (!Enemy || Enemy->IsDead() || State == EEnemyState::Dead)
	{
		return;
	}
	const ATransoxianaGameMode* GM = GetGameMode();
	if (GM && GM->IsMissionOver())
	{
		HaltMovement();
		return;
	}

	StateTimer += DeltaSeconds;
	UpdatePerception(DeltaSeconds);

	if (Enemy->IsStaggered())
	{
		HaltMovement();
		return;
	}

	switch (State)
	{
	case EEnemyState::Patrol:     UpdatePatrol(DeltaSeconds); break;
	case EEnemyState::Suspicious: UpdateSuspicious(DeltaSeconds); break;
	case EEnemyState::Combat:     UpdateCombat(DeltaSeconds); break;
	case EEnemyState::Search:     UpdateSearch(DeltaSeconds); break;
	default: break;
	}

	// Face where we're walking unless a state set a more specific focus this frame.
	if (State != EEnemyState::Combat && GetFocalPointForPriority(EAIFocusPriority::Gameplay) == FAISystem::InvalidLocation)
	{
		const FVector Velocity = Enemy->GetVelocity();
		if (Velocity.SizeSquared2D() > 400.f)
		{
			SetFocalPoint(Enemy->GetActorLocation() + Velocity.GetSafeNormal2D() * 300.f, EAIFocusPriority::Move);
		}
	}
}

// ---------------------------------------------------------------------------
// Perception
// ---------------------------------------------------------------------------

bool AEnemyAIController::CanSeePlayer(const ATransoxianaCharacter* Player, float& OutDistance, float& OutCenterDot) const
{
	const AEnemyCharacter* Enemy = GetEnemy();
	if (!Player || Player->IsDead() || !Enemy)
	{
		return false;
	}

	const FVector Eye = Enemy->GetHeadLocation();
	const FVector Target = Player->GetActorLocation();
	const FVector ToPlayer = Target - Eye;
	OutDistance = ToPlayer.Size();
	OutCenterDot = FVector::DotProduct(Enemy->GetActorForwardVector(), ToPlayer.GetSafeNormal());

	const float Range = (Enemy->EnemyType == EEnemyType::Archer) ? ArcherSightRange : SightRange;
	const bool bClose = OutDistance <= CloseSenseRadius && !Player->IsCrouched();
	if (!bClose)
	{
		if (OutDistance > Range || OutCenterDot < ViewConeCos)
		{
			return false;
		}
		if (Player->IsInHidingSpot() && OutDistance > HidingSpotRevealDistance)
		{
			return false;
		}
	}

	// Line of sight to the player's head or body.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GuardSight), false, Enemy);
	const FVector Points[] = { Player->GetHeadLocation(), Target + FVector(0.f, 0.f, 10.f) };
	for (const FVector& Point : Points)
	{
		FHitResult Hit;
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Eye, Point, ECC_Visibility, Params);
		if (!bBlocked || Hit.GetActor() == Player || Cast<APawn>(Hit.GetActor()) != nullptr)
		{
			return true;
		}
	}
	return false;
}

void AEnemyAIController::UpdatePerception(float DeltaSeconds)
{
	ATransoxianaCharacter* Player = GetPlayer();
	float Distance = 0.f;
	float CenterDot = 0.f;
	bSeesPlayer = CanSeePlayer(Player, Distance, CenterDot);

	if (bSeesPlayer)
	{
		LastKnownPlayerLocation = Player->GetActorLocation();
		TimeSinceSeen = 0.f;

		if (State != EEnemyState::Combat)
		{
			const AEnemyCharacter* Enemy = GetEnemy();
			const float Range = (Enemy && Enemy->EnemyType == EEnemyType::Archer) ? ArcherSightRange : SightRange;
			const float DistanceFactor = FMath::Lerp(3.0f, 0.4f, FMath::Clamp(Distance / Range, 0.f, 1.f));
			const float AngleFactor = FMath::Lerp(0.5f, 1.0f, FMath::Clamp((CenterDot - ViewConeCos) / (1.f - ViewConeCos), 0.f, 1.f));
			const ATransoxianaGameMode* GM = GetGameMode();
			const bool bHeightened = bPersonallyHeightened || (GM && GM->IsHeightenedAlert());
			const float AlertFactor = (bHeightened ? 1.5f : 1.f) * (State != EEnemyState::Patrol ? 1.3f : 1.f);
			Awareness += AwarenessGainRate * DistanceFactor * AngleFactor * AlertFactor * Player->GetStealthVisibility() * DeltaSeconds;
			Awareness = FMath::Min(Awareness, 1.f);
		}
	}
	else
	{
		TimeSinceSeen += DeltaSeconds;
		if (State == EEnemyState::Patrol || State == EEnemyState::Suspicious)
		{
			Awareness = FMath::Max(0.f, Awareness - AwarenessDecayRate * DeltaSeconds);
		}
	}

	if (State != EEnemyState::Combat && Awareness >= 1.f)
	{
		EnterCombat(true);
	}
	else if (State == EEnemyState::Patrol && Awareness >= SuspiciousThreshold)
	{
		InvestigatePoint = LastKnownPlayerLocation;
		EnterState(EEnemyState::Suspicious);
	}
}

void AEnemyAIController::HearNoise(const FVector& Location)
{
	if (State == EEnemyState::Combat || State == EEnemyState::Dead)
	{
		return;
	}
	Awareness = FMath::Max(Awareness, 0.5f);
	InvestigatePoint = Location;
	if (State == EEnemyState::Search)
	{
		bArrivedAtInvestigatePoint = false;
		StateTimer = 0.f;
		return;
	}
	EnterState(EEnemyState::Suspicious);
}

void AEnemyAIController::OnDamaged(APawn* Attacker)
{
	if (Attacker)
	{
		LastKnownPlayerLocation = Attacker->GetActorLocation();
	}
	if (State != EEnemyState::Combat)
	{
		EnterCombat(true);
	}
}

void AEnemyAIController::AlertTo(const FVector& Location)
{
	if (State == EEnemyState::Combat || State == EEnemyState::Dead)
	{
		return;
	}
	LastKnownPlayerLocation = Location;
	EnterCombat(false);
}

void AEnemyAIController::OnPawnDied()
{
	State = EEnemyState::Dead;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}

// ---------------------------------------------------------------------------
// States
// ---------------------------------------------------------------------------

void AEnemyAIController::EnterState(EEnemyState NewState)
{
	State = NewState;
	StateTimer = 0.f;
	bArrivedAtInvestigatePoint = false;
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AEnemyAIController::EnterCombat(bool bShareAlert)
{
	const bool bWasInCombat = (State == EEnemyState::Combat);
	EnterState(EEnemyState::Combat);
	Awareness = 1.f;
	bPersonallyHeightened = true;
	TimeSinceSeen = 0.f;

	if (bWasInCombat)
	{
		return;
	}
	if (ATransoxianaGameMode* GM = GetGameMode())
	{
		GM->NotifyEnemyEnteredCombat(GetEnemy());
	}
	if (bShareAlert && GetPawn())
	{
		// Shout: nearby guards join the fight.
		for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
		{
			AEnemyCharacter* Other = *It;
			if (Other && Other != GetPawn() && !Other->IsDead() &&
				FVector::Dist(Other->GetActorLocation(), GetPawn()->GetActorLocation()) <= AlertShareRadius)
			{
				if (AEnemyAIController* OtherAI = Other->GetEnemyController())
				{
					OtherAI->AlertTo(LastKnownPlayerLocation);
				}
			}
		}
	}
}

void AEnemyAIController::UpdatePatrol(float DeltaSeconds)
{
	AEnemyCharacter* Enemy = GetEnemy();
	if (Enemy->PatrolPoints.Num() == 0)
	{
		// Stationary guard: return to post and face the original direction.
		if (MoveToward(HomeLocation, 60.f))
		{
			SetFocalPoint(HomeLocation + HomeRotation.Vector() * 500.f, EAIFocusPriority::Gameplay);
		}
		return;
	}

	PatrolIndex = PatrolIndex % Enemy->PatrolPoints.Num();
	const FVector Goal = Enemy->PatrolPoints[PatrolIndex];
	if (MoveToward(Goal, 80.f))
	{
		PatrolWaitTimer += DeltaSeconds;
		if (PatrolWaitTimer >= PatrolWaitTime)
		{
			PatrolWaitTimer = 0.f;
			PatrolIndex = (PatrolIndex + 1) % Enemy->PatrolPoints.Num();
		}
	}
}

void AEnemyAIController::UpdateSuspicious(float DeltaSeconds)
{
	SetFocalPoint(InvestigatePoint, EAIFocusPriority::Gameplay);

	// Pause and stare first ("What was that?"), then walk over to look.
	if (StateTimer < 1.2f)
	{
		HaltMovement();
		return;
	}
	if (!bArrivedAtInvestigatePoint)
	{
		if (MoveToward(InvestigatePoint, 120.f) || StateTimer > 15.f)
		{
			bArrivedAtInvestigatePoint = true;
			StateTimer = 1.2f;
		}
		return;
	}

	LookAround(InvestigatePoint);
	if (StateTimer > 5.f && Awareness < SuspiciousThreshold)
	{
		Awareness = FMath::Min(Awareness, 0.2f);
		EnterState(EEnemyState::Patrol);
	}
}

void AEnemyAIController::UpdateCombat(float DeltaSeconds)
{
	AEnemyCharacter* Enemy = GetEnemy();
	ATransoxianaCharacter* Player = GetPlayer();
	if (!Player || Player->IsDead())
	{
		HaltMovement();
		return;
	}

	if (TimeSinceSeen > LoseTargetTime)
	{
		InvestigatePoint = LastKnownPlayerLocation;
		EnterState(EEnemyState::Search);
		return;
	}

	SetFocalPoint(bSeesPlayer ? Player->GetActorLocation() : LastKnownPlayerLocation, EAIFocusPriority::Gameplay);
	const float Distance = FVector::Dist2D(Enemy->GetActorLocation(), Player->GetActorLocation());

	if (Enemy->EnemyType == EEnemyType::Archer)
	{
		if (bSeesPlayer)
		{
			HaltMovement();
			ArrowCooldown -= DeltaSeconds;
			if (ArrowCooldown <= 0.f)
			{
				Enemy->ShootArrowAt(Player);
				ArrowCooldown = FMath::FRandRange(2.0f, 3.0f);
			}
		}
		else if (Enemy->PatrolPoints.Num() > 0)
		{
			MoveToward(LastKnownPlayerLocation, 400.f); // ground archers reposition; tower archers hold
		}
		return;
	}

	// Swordsman
	const FVector Goal = bSeesPlayer ? Player->GetActorLocation() : LastKnownPlayerLocation;
	if (Distance > Enemy->AttackReach * 0.75f || !bSeesPlayer)
	{
		MoveToward(Goal, 110.f);
	}
	else
	{
		HaltMovement();
	}

	AttackCooldown -= DeltaSeconds;
	const ATransoxianaGameMode* GM = GetGameMode();
	const FVector ToPlayer = (Player->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	const bool bFacing = FVector::DotProduct(Enemy->GetActorForwardVector(), ToPlayer) > 0.7f;
	if (bSeesPlayer && Distance <= Enemy->AttackReach && bFacing && AttackCooldown <= 0.f && !Enemy->IsAttacking() &&
		(!GM || GM->CanEnemyStartAttack()))
	{
		Enemy->StartMeleeAttack();
		AttackCooldown = FMath::FRandRange(1.4f, 2.4f);
	}
}

void AEnemyAIController::UpdateSearch(float DeltaSeconds)
{
	if (bSeesPlayer)
	{
		// Seen again while searching: straight back to combat.
		Awareness = FMath::Max(Awareness, 0.8f);
	}
	if (!bArrivedAtInvestigatePoint)
	{
		SetFocalPoint(InvestigatePoint, EAIFocusPriority::Gameplay);
		if (MoveToward(InvestigatePoint, 120.f) || StateTimer > 15.f)
		{
			bArrivedAtInvestigatePoint = true;
			StateTimer = 0.f;
		}
		return;
	}
	LookAround(InvestigatePoint);
	if (StateTimer > SearchDuration)
	{
		Awareness = 0.25f;
		EnterState(EEnemyState::Patrol);
	}
}

// ---------------------------------------------------------------------------
// Movement helpers
// ---------------------------------------------------------------------------

bool AEnemyAIController::MoveToward(const FVector& Goal, float AcceptanceRadius)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return false;
	}
	if (FVector::Dist2D(MyPawn->GetActorLocation(), Goal) <= AcceptanceRadius)
	{
		HaltMovement();
		return true;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now >= DirectSteeringUntil)
	{
		const bool bGoalChanged = !bHasMoveGoal || FVector::Dist2D(Goal, CurrentMoveGoal) > 100.f;
		if (bGoalChanged || GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			const EPathFollowingRequestResult::Type Result =
				MoveToLocation(Goal, AcceptanceRadius * 0.8f, /*bStopOnOverlap*/ true, /*bUsePathfinding*/ true,
					/*bProjectDestinationToNavigation*/ true, /*bCanStrafe*/ true);
			if (Result == EPathFollowingRequestResult::Failed)
			{
				// No NavMesh (or unreachable): steer directly for a while, then retry.
				DirectSteeringUntil = Now + 2.f;
				bHasMoveGoal = false;
			}
			else
			{
				CurrentMoveGoal = Goal;
				bHasMoveGoal = true;
			}
		}
	}

	if (Now < DirectSteeringUntil)
	{
		MyPawn->AddMovementInput((Goal - MyPawn->GetActorLocation()).GetSafeNormal2D(), 1.f);
	}
	return false;
}

void AEnemyAIController::HaltMovement()
{
	if (bHasMoveGoal)
	{
		StopMovement();
		bHasMoveGoal = false;
	}
}

void AEnemyAIController::LookAround(const FVector& Center)
{
	HaltMovement();
	if (const APawn* MyPawn = GetPawn())
	{
		const float Angle = StateTimer * 70.f;
		const FVector Dir = FRotator(0.f, HomeRotation.Yaw + Angle, 0.f).Vector();
		SetFocalPoint(MyPawn->GetActorLocation() + Dir * 500.f, EAIFocusPriority::Gameplay);
	}
}
