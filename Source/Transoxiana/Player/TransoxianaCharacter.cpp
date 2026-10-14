#include "Player/TransoxianaCharacter.h"

#include "AI/EnemyCharacter.h"
#include "Camera/CameraComponent.h"
#include "Combat/ArrowProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Core/TransoxianaGameMode.h"
#include "Core/TransoxianaPlayerController.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Mission/DispatchPickup.h"
#include "Transoxiana.h"
#include "UI/TransoxianaHUD.h"
#include "World/TransoxianaVisuals.h"

ATransoxianaCharacter::ATransoxianaCharacter()
{
	// Camera follows the controller; the body turns toward movement (classic third person).
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 600.f, 0.f);
	Move->MaxWalkSpeed = RunSpeed;
	Move->MaxWalkSpeedCrouched = CrouchSpeed;
	Move->JumpZVelocity = 520.f;
	Move->AirControl = 0.3f;
	Move->BrakingDecelerationWalking = 2000.f;
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->SetCrouchedHalfHeight(55.f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 360.f;
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 14.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ATransoxianaCharacter::BeginPlay()
{
	Super::BeginPlay();
	Stamina = MaxStamina;
	SetupAppearance(TransoxianaVisuals::Cloth);
	BowStave->SetVisibility(true);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void ATransoxianaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	ATransoxianaPlayerController* PC = Cast<ATransoxianaPlayerController>(GetController());
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!PC || !Input)
	{
		UE_LOG(LogTransoxiana, Error, TEXT("Player input not set up: needs ATransoxianaPlayerController and Enhanced Input (see Config/DefaultInput.ini)."));
		return;
	}
	PC->EnsureInputCreated();

	Input->BindAction(PC->IA_MoveForward, ETriggerEvent::Triggered, this, &ThisClass::OnMoveForward);
	Input->BindAction(PC->IA_MoveRight, ETriggerEvent::Triggered, this, &ThisClass::OnMoveRight);
	Input->BindAction(PC->IA_LookX, ETriggerEvent::Triggered, this, &ThisClass::OnLookX);
	Input->BindAction(PC->IA_LookY, ETriggerEvent::Triggered, this, &ThisClass::OnLookY);
	Input->BindAction(PC->IA_PadLookX, ETriggerEvent::Triggered, this, &ThisClass::OnPadLookX);
	Input->BindAction(PC->IA_PadLookY, ETriggerEvent::Triggered, this, &ThisClass::OnPadLookY);
	Input->BindAction(PC->IA_Jump, ETriggerEvent::Started, this, &ThisClass::OnJumpPressed);
	Input->BindAction(PC->IA_Jump, ETriggerEvent::Completed, this, &ThisClass::OnJumpReleased);
	Input->BindAction(PC->IA_Sprint, ETriggerEvent::Started, this, &ThisClass::OnSprintPressed);
	Input->BindAction(PC->IA_Sprint, ETriggerEvent::Completed, this, &ThisClass::OnSprintReleased);
	Input->BindAction(PC->IA_Crouch, ETriggerEvent::Started, this, &ThisClass::OnCrouchPressed);
	Input->BindAction(PC->IA_Dodge, ETriggerEvent::Started, this, &ThisClass::OnDodgePressed);
	Input->BindAction(PC->IA_Attack, ETriggerEvent::Started, this, &ThisClass::OnAttackPressed);
	Input->BindAction(PC->IA_Attack, ETriggerEvent::Completed, this, &ThisClass::OnAttackReleased);
	Input->BindAction(PC->IA_Alt, ETriggerEvent::Started, this, &ThisClass::OnAltPressed);
	Input->BindAction(PC->IA_Alt, ETriggerEvent::Completed, this, &ThisClass::OnAltReleased);
	Input->BindAction(PC->IA_Interact, ETriggerEvent::Started, this, &ThisClass::OnInteractPressed);
	Input->BindAction(PC->IA_SwitchWeapon, ETriggerEvent::Started, this, &ThisClass::OnSwitchWeaponPressed);
}

bool ATransoxianaCharacter::IsInputBlocked() const
{
	if (bDead)
	{
		return true;
	}
	const ATransoxianaGameMode* GM = GetTransoxianaGameMode();
	return GM && GM->IsMissionOver();
}

ATransoxianaGameMode* ATransoxianaCharacter::GetTransoxianaGameMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<ATransoxianaGameMode>() : nullptr;
}

float ATransoxianaCharacter::GetLookSensitivity() const
{
	const ATransoxianaPlayerController* PC = Cast<ATransoxianaPlayerController>(GetController());
	return PC ? PC->MouseSensitivity : 1.f;
}

bool ATransoxianaCharacter::IsLookInverted() const
{
	const ATransoxianaPlayerController* PC = Cast<ATransoxianaPlayerController>(GetController());
	return PC && PC->bInvertY;
}

void ATransoxianaCharacter::OnMoveForward(const FInputActionValue& Value)
{
	const float Amount = Value.Get<float>();
	if (IsInputBlocked() || !Controller || FMath::IsNearlyZero(Amount))
	{
		return;
	}
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Amount);
}

void ATransoxianaCharacter::OnMoveRight(const FInputActionValue& Value)
{
	const float Amount = Value.Get<float>();
	if (IsInputBlocked() || !Controller || FMath::IsNearlyZero(Amount))
	{
		return;
	}
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Amount);
}

void ATransoxianaCharacter::OnLookX(const FInputActionValue& Value)
{
	AddControllerYawInput(Value.Get<float>() * GetLookSensitivity());
}

void ATransoxianaCharacter::OnLookY(const FInputActionValue& Value)
{
	// Same convention as Epic's Third Person template (mouse Y negated);
	// "Invert Y" in the settings menu flips it.
	const float Sign = IsLookInverted() ? 1.f : -1.f;
	AddControllerPitchInput(Sign * Value.Get<float>() * GetLookSensitivity());
}

void ATransoxianaCharacter::OnPadLookX(const FInputActionValue& Value)
{
	const float Rate = 70.f * GetWorld()->GetDeltaSeconds();
	AddControllerYawInput(Value.Get<float>() * Rate * GetLookSensitivity());
}

void ATransoxianaCharacter::OnPadLookY(const FInputActionValue& Value)
{
	const float Rate = 50.f * GetWorld()->GetDeltaSeconds();
	const float Sign = IsLookInverted() ? 1.f : -1.f;
	AddControllerPitchInput(Sign * Value.Get<float>() * Rate * GetLookSensitivity());
}

void ATransoxianaCharacter::OnJumpPressed()
{
	if (IsInputBlocked())
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
		return;
	}
	Jump();
}

void ATransoxianaCharacter::OnJumpReleased()
{
	StopJumping();
}

void ATransoxianaCharacter::OnSprintPressed()
{
	bSprintHeld = true;
	if (bIsCrouched)
	{
		UnCrouch();
	}
}

void ATransoxianaCharacter::OnSprintReleased()
{
	bSprintHeld = false;
	bSprintExhausted = false;
}

void ATransoxianaCharacter::OnCrouchPressed()
{
	if (IsInputBlocked())
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else if (!GetCharacterMovement()->IsFalling())
	{
		Crouch();
	}
}

void ATransoxianaCharacter::OnDodgePressed()
{
	if (IsInputBlocked() || bAttacking || GetCharacterMovement()->IsFalling())
	{
		return;
	}
	if (!SpendStamina(DodgeStaminaCost))
	{
		ShowToast(TEXT("Too tired to dodge"));
		return;
	}
	FVector Direction = GetLastMovementInputVector().GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		Direction = -GetActorForwardVector(); // back-step
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	LaunchCharacter(Direction * DodgeImpulse + FVector(0.f, 0.f, 120.f), true, true);
	InvulnerableUntil = GetWorld()->GetTimeSeconds() + 0.35f;
}

void ATransoxianaCharacter::OnAttackPressed()
{
	if (IsInputBlocked())
	{
		return;
	}
	if (WeaponMode == EWeaponMode::Sabre)
	{
		if (bBlocking)
		{
			return;
		}
		if (bAttacking)
		{
			// Queue the second hit of the combo if pressed late in the swing.
			if (AttackElapsed > SwordAttackDuration * 0.45f)
			{
				bAttackQueued = true;
			}
			return;
		}
		StartSwordAttack();
	}
	else
	{
		if (!bAiming)
		{
			ShowToast(TEXT("Hold Right Mouse / Left Trigger to aim the bow"));
			return;
		}
		bDrawing = true;
		DrawStartTime = GetWorld()->GetTimeSeconds();
	}
}

void ATransoxianaCharacter::OnAttackReleased()
{
	if (WeaponMode == EWeaponMode::Bow && bDrawing)
	{
		bDrawing = false;
		if (bAiming && !IsInputBlocked())
		{
			FireArrow();
		}
	}
}

void ATransoxianaCharacter::OnAltPressed()
{
	bAltHeld = true;
	if (IsInputBlocked())
	{
		return;
	}
	if (WeaponMode == EWeaponMode::Sabre)
	{
		bBlocking = true;
		BlockStartTime = GetWorld()->GetTimeSeconds();
	}
	else
	{
		bAiming = true;
	}
}

void ATransoxianaCharacter::OnAltReleased()
{
	bAltHeld = false;
	bBlocking = false;
	bAiming = false;
	bDrawing = false;
}

void ATransoxianaCharacter::OnInteractPressed()
{
	if (IsInputBlocked())
	{
		return;
	}

	// 1) Pick up the mission objective if standing next to it.
	TArray<AActor*> Overlapping;
	GetOverlappingActors(Overlapping, ADispatchPickup::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		if (ADispatchPickup* Pickup = Cast<ADispatchPickup>(Actor))
		{
			Pickup->Collect(this);
			return;
		}
	}

	// 2) Otherwise, try a silent takedown.
	TryTakedown();
}

void ATransoxianaCharacter::OnSwitchWeaponPressed()
{
	if (IsInputBlocked() || bAttacking)
	{
		return;
	}
	WeaponMode = (WeaponMode == EWeaponMode::Sabre) ? EWeaponMode::Bow : EWeaponMode::Sabre;
	bBlocking = false;
	bAiming = false;
	bDrawing = false;
	// Holding the alt button while switching keeps the new weapon's alt action active.
	if (bAltHeld)
	{
		OnAltPressed();
	}
	ShowToast(WeaponMode == EWeaponMode::Sabre ? TEXT("Sabre drawn") : TEXT("Bow ready"));
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void ATransoxianaCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDead)
	{
		return;
	}

	UpdateSwordAttack(DeltaSeconds);
	UpdateStamina(DeltaSeconds);
	UpdateMovementSpeed();
	UpdateCamera(DeltaSeconds);
	UpdateNoise(DeltaSeconds);
	UpdateInteractPrompt();

	// Weapon visuals
	SwordBlade->SetVisibility(WeaponMode == EWeaponMode::Sabre);
	if (!bAttacking)
	{
		PoseSword(0.f, false, bBlocking);
	}
}

void ATransoxianaCharacter::UpdateMovementSpeed()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bMoving = GetVelocity().SizeSquared2D() > 100.f;
	const bool bWantsSprint = bSprintHeld && !bSprintExhausted && !bAiming && !bBlocking && !bIsCrouched && bMoving;

	float Speed = RunSpeed;
	if (bAiming || bBlocking || bAttacking)
	{
		Speed = CombatMoveSpeed;
	}
	else if (bWantsSprint)
	{
		Speed = SprintSpeed;
	}
	Move->MaxWalkSpeed = Speed;

	// Strafe while aiming or blocking so the player faces the camera direction.
	const bool bStrafe = bAiming || bBlocking;
	bUseControllerRotationYaw = bStrafe;
	Move->bOrientRotationToMovement = !bStrafe;
}

void ATransoxianaCharacter::UpdateCamera(float DeltaSeconds)
{
	const float TargetLength = bAiming ? 170.f : 360.f;
	const FVector TargetOffset = bAiming ? FVector(0.f, 70.f, 55.f) : FVector(0.f, 50.f, 60.f);
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetLength, DeltaSeconds, 10.f);
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetOffset, DeltaSeconds, 10.f);
}

void ATransoxianaCharacter::UpdateStamina(float DeltaSeconds)
{
	TimeSinceStaminaSpent += DeltaSeconds;
	const bool bSprinting = GetCharacterMovement()->MaxWalkSpeed >= SprintSpeed - 1.f && GetVelocity().SizeSquared2D() > 100.f;
	if (bSprinting)
	{
		Stamina = FMath::Max(0.f, Stamina - SprintStaminaPerSecond * DeltaSeconds);
		TimeSinceStaminaSpent = 0.f;
		if (Stamina <= 0.f)
		{
			bSprintExhausted = true;
		}
	}
	else if (!bBlocking && TimeSinceStaminaSpent > 0.8f)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond * DeltaSeconds);
	}
}

bool ATransoxianaCharacter::SpendStamina(float Amount)
{
	if (Stamina < Amount)
	{
		return false;
	}
	Stamina -= Amount;
	TimeSinceStaminaSpent = 0.f;
	return true;
}

void ATransoxianaCharacter::UpdateNoise(float DeltaSeconds)
{
	ATransoxianaGameMode* GM = GetTransoxianaGameMode();
	if (!GM || GetCharacterMovement()->IsFalling())
	{
		return;
	}
	const float Speed = GetVelocity().Size2D();
	NoiseTimer -= DeltaSeconds;
	if (NoiseTimer > 0.f || Speed < 50.f || bIsCrouched)
	{
		return;
	}
	// Footsteps: sprinting is loud, running is quiet, sneaking is silent.
	const float Radius = (Speed > RunSpeed + 50.f) ? 900.f : 300.f;
	GM->ReportNoise(GetActorLocation(), Radius, this);
	NoiseTimer = 0.5f;
}

void ATransoxianaCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (ATransoxianaGameMode* GM = GetTransoxianaGameMode())
	{
		GM->ReportNoise(GetActorLocation(), 600.f, this);
	}
}

float ATransoxianaCharacter::GetStealthVisibility() const
{
	float Visibility = 1.f;
	if (bIsCrouched)
	{
		Visibility *= 0.5f;
		if (StealthZoneCount > 0)
		{
			Visibility *= 0.5f; // crouched in shade/hay: 25 %
		}
	}
	const float Speed = GetVelocity().Size2D();
	if (Speed > RunSpeed + 50.f)
	{
		Visibility *= 1.5f; // sprinting draws the eye
	}
	else if (Speed < 10.f)
	{
		Visibility *= 0.8f; // standing still
	}
	return Visibility;
}

float ATransoxianaCharacter::GetDrawAlpha() const
{
	if (!bDrawing)
	{
		return 0.f;
	}
	return FMath::Clamp((GetWorld()->GetTimeSeconds() - DrawStartTime) / FullDrawTime, 0.f, 1.f);
}

void ATransoxianaCharacter::UpdateInteractPrompt()
{
	InteractPrompt.Reset();
	TArray<AActor*> Overlapping;
	GetOverlappingActors(Overlapping, ADispatchPickup::StaticClass());
	if (Overlapping.Num() > 0)
	{
		InteractPrompt = TEXT("[E / X]  Take the sealed dispatch");
		return;
	}
	if (FindTakedownTarget())
	{
		InteractPrompt = TEXT("[E / X]  Silent takedown");
	}
}

void ATransoxianaCharacter::ShowToast(const FString& Message) const
{
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ATransoxianaHUD* HUD = Cast<ATransoxianaHUD>(PC->GetHUD()))
		{
			HUD->ShowToast(Message);
		}
	}
}

// ---------------------------------------------------------------------------
// Combat
// ---------------------------------------------------------------------------

AEnemyCharacter* ATransoxianaCharacter::FindSoftLockTarget(float MaxDistance) const
{
	AEnemyCharacter* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	const FVector Forward = GetControlRotation().Vector().GetSafeNormal2D();
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		AEnemyCharacter* Enemy = *It;
		if (!Enemy || Enemy->IsDead())
		{
			continue;
		}
		const FVector ToEnemy = Enemy->GetActorLocation() - GetActorLocation();
		const float Distance = ToEnemy.Size2D();
		if (Distance > MaxDistance)
		{
			continue;
		}
		const float Facing = FVector::DotProduct(Forward, ToEnemy.GetSafeNormal2D());
		if (Facing < 0.2f)
		{
			continue;
		}
		const float Score = Distance * (2.f - Facing);
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Enemy;
		}
	}
	return Best;
}

void ATransoxianaCharacter::StartSwordAttack()
{
	if (!SpendStamina(AttackStaminaCost))
	{
		ShowToast(TEXT("Too tired to attack"));
		return;
	}
	bAttacking = true;
	bAttackQueued = false;
	bAttackHitApplied = false;
	AttackElapsed = 0.f;
	bMirrorSwing = !bMirrorSwing;

	if (bIsCrouched)
	{
		UnCrouch();
	}

	// Soft lock: turn toward the closest enemy in front, otherwise toward the camera.
	FRotator Facing(0.f, GetControlRotation().Yaw, 0.f);
	if (AEnemyCharacter* Target = FindSoftLockTarget(400.f))
	{
		Facing = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();
	}
	SetActorRotation(Facing);
}

void ATransoxianaCharacter::UpdateSwordAttack(float DeltaSeconds)
{
	if (!bAttacking)
	{
		return;
	}
	AttackElapsed += DeltaSeconds;
	const float Alpha = AttackElapsed / SwordAttackDuration;
	PoseSword(FMath::Clamp((Alpha - 0.1f) / 0.5f, 0.01f, 1.f), bMirrorSwing, false);

	// Hit window: 25 %..55 % of the swing.
	if (!bAttackHitApplied && Alpha >= 0.25f)
	{
		bAttackHitApplied = true;
		const FVector Center = GetActorLocation() + GetActorForwardVector() * (SwordReach * 0.6f);
		bool bHitSomething = false;
		for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
		{
			AEnemyCharacter* Enemy = *It;
			if (!Enemy || Enemy->IsDead())
			{
				continue;
			}
			const FVector ToEnemy = Enemy->GetActorLocation() - GetActorLocation();
			const bool bInReach = ToEnemy.Size2D() <= SwordReach + 40.f;
			const bool bInFront = FVector::DotProduct(GetActorForwardVector(), ToEnemy.GetSafeNormal2D()) > 0.3f;
			const bool bNearCenter = FVector::Dist2D(Center, Enemy->GetActorLocation()) < SwordReach;
			if (bInReach && (bInFront || bNearCenter))
			{
				UGameplayStatics::ApplyDamage(Enemy, SwordDamage, GetController(), this, UDamageType::StaticClass());
				bHitSomething = true;
			}
		}
		if (bHitSomething)
		{
			if (ATransoxianaGameMode* GM = GetTransoxianaGameMode())
			{
				GM->ReportNoise(GetActorLocation(), 1200.f, this);
			}
		}
	}

	if (AttackElapsed >= SwordAttackDuration)
	{
		bAttacking = false;
		PoseSword(0.f, false, false);
		if (bAttackQueued)
		{
			StartSwordAttack();
		}
	}
}

void ATransoxianaCharacter::FireArrow()
{
	// bDrawing is already cleared by the release handler, so compute power from the start time.
	const float ClampedPower = FMath::Clamp((GetWorld()->GetTimeSeconds() - DrawStartTime) / FullDrawTime, 0.25f, 1.f);

	// Aim where the camera crosshair points.
	FVector CamLocation;
	FRotator CamRotation;
	GetController()->GetPlayerViewPoint(CamLocation, CamRotation);
	const FVector TraceEnd = CamLocation + CamRotation.Vector() * 8000.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ArrowAim), false, this);
	const FVector AimPoint = GetWorld()->LineTraceSingleByChannel(Hit, CamLocation, TraceEnd, ECC_Visibility, Params) ? Hit.ImpactPoint : TraceEnd;

	const FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 60.f + GetActorRightVector() * 15.f + FVector(0.f, 0.f, 50.f);
	const FVector Direction = (AimPoint - SpawnLocation).GetSafeNormal();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AArrowProjectile* Arrow = GetWorld()->SpawnActor<AArrowProjectile>(AArrowProjectile::StaticClass(), SpawnLocation, Direction.Rotation(), SpawnParams))
	{
		const float Damage = FMath::Lerp(ArrowMinDamage, ArrowMaxDamage, ClampedPower);
		const float Speed = FMath::Lerp(2500.f, 6000.f, ClampedPower);
		Arrow->Launch(Direction * Speed, Damage, true);
	}
}

AEnemyCharacter* ATransoxianaCharacter::FindTakedownTarget() const
{
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		AEnemyCharacter* Enemy = *It;
		if (!Enemy || !Enemy->CanBeTakenDown())
		{
			continue;
		}
		const FVector EnemyToPlayer = GetActorLocation() - Enemy->GetActorLocation();
		if (EnemyToPlayer.Size2D() > TakedownRange || FMath::Abs(EnemyToPlayer.Z) > 100.f)
		{
			continue;
		}
		// Player must be behind the enemy.
		const float Behind = FVector::DotProduct(Enemy->GetActorForwardVector(), EnemyToPlayer.GetSafeNormal2D());
		if (Behind < -0.3f)
		{
			return Enemy;
		}
	}
	return nullptr;
}

void ATransoxianaCharacter::TryTakedown()
{
	AEnemyCharacter* Target = FindTakedownTarget();
	if (!Target)
	{
		return;
	}
	SetActorRotation((Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D().Rotation());
	PoseSword(1.f, false, false);
	Target->KillByTakedown(GetController());
	ShowToast(TEXT("Silent takedown"));
}

float ATransoxianaCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead || GetWorld()->GetTimeSeconds() < InvulnerableUntil)
	{
		return 0.f;
	}

	// Melee hits from the front can be blocked or parried.
	AEnemyCharacter* MeleeAttacker = Cast<AEnemyCharacter>(DamageCauser);
	if (MeleeAttacker && bBlocking)
	{
		const FVector ToAttacker = (MeleeAttacker->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (FVector::DotProduct(GetActorForwardVector(), ToAttacker) > 0.3f)
		{
			const float Now = GetWorld()->GetTimeSeconds();
			if (Now - BlockStartTime <= ParryWindow)
			{
				MeleeAttacker->Stagger(1.6f);
				ShowToast(TEXT("PARRY! Strike now"));
				return 0.f;
			}
			if (SpendStamina(BlockStaminaCost))
			{
				ShowToast(TEXT("Blocked"));
				return 0.f;
			}
			// Guard broken: not enough stamina.
			ShowToast(TEXT("Guard broken!"));
			bBlocking = false;
			DamageAmount *= 0.5f;
		}
	}

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - Applied);
	if (Health <= 0.f)
	{
		Die();
	}
	return Applied;
}

void ATransoxianaCharacter::Die()
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	bAttacking = false;
	bBlocking = false;
	bAiming = false;
	GetCharacterMovement()->DisableMovement();
	PlayDeathPose();
	if (ATransoxianaGameMode* GM = GetTransoxianaGameMode())
	{
		GM->OnPlayerDied();
	}
}
