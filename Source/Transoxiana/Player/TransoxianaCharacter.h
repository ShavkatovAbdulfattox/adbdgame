#pragma once

#include "CoreMinimal.h"
#include "Player/TransoxianaCharacterBase.h"
#include "Core/TransoxianaTypes.h"
#include "TransoxianaCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class AEnemyCharacter;
class ATransoxianaGameMode;
struct FInputActionValue;

/**
 * Bahram, the playable scout: third-person movement, sprint/crouch/dodge, sabre with
 * block and parry, composite bow with aiming, silent takedowns, and stealth visibility.
 */
UCLASS(Config = Game)
class TRANSOXIANA_API ATransoxianaCharacter : public ATransoxianaCharacterBase
{
	GENERATED_BODY()

public:
	ATransoxianaCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Landed(const FHitResult& Hit) override;

	/** 0..~1.5 multiplier used by enemy vision. Lower = harder to see. */
	float GetStealthVisibility() const;

	bool IsInHidingSpot() const { return bIsCrouched && StealthZoneCount > 0; }
	bool IsAiming() const { return bAiming; }
	bool IsBlocking() const { return bBlocking; }
	float GetDrawAlpha() const;
	float GetStamina() const { return Stamina; }
	float GetMaxStamina() const { return MaxStamina; }
	EWeaponMode GetWeaponMode() const { return WeaponMode; }
	const FString& GetInteractPrompt() const { return InteractPrompt; }

	/** Called by stealth zones on overlap. */
	void EnterStealthZone() { ++StealthZoneCount; }
	void LeaveStealthZone() { StealthZoneCount = FMath::Max(0, StealthZoneCount - 1); }

protected:
	virtual void BeginPlay() override;

	// ---- Input handlers ----
	void OnMoveForward(const FInputActionValue& Value);
	void OnMoveRight(const FInputActionValue& Value);
	void OnLookX(const FInputActionValue& Value);
	void OnLookY(const FInputActionValue& Value);
	void OnPadLookX(const FInputActionValue& Value);
	void OnPadLookY(const FInputActionValue& Value);
	void OnJumpPressed();
	void OnJumpReleased();
	void OnSprintPressed();
	void OnSprintReleased();
	void OnCrouchPressed();
	void OnDodgePressed();
	void OnAttackPressed();
	void OnAttackReleased();
	void OnAltPressed();
	void OnAltReleased();
	void OnInteractPressed();
	void OnSwitchWeaponPressed();

	// ---- Actions ----
	void StartSwordAttack();
	void UpdateSwordAttack(float DeltaSeconds);
	void FireArrow();
	void TryTakedown();
	void Die();
	void UpdateMovementSpeed();
	void UpdateCamera(float DeltaSeconds);
	void UpdateStamina(float DeltaSeconds);
	void UpdateNoise(float DeltaSeconds);
	void UpdateInteractPrompt();
	bool SpendStamina(float Amount);
	void ShowToast(const FString& Message) const;

	/** Unaware enemy that can be taken down right now, or null. */
	AEnemyCharacter* FindTakedownTarget() const;
	/** Closest living enemy in front of the player, for attack soft-lock. */
	AEnemyCharacter* FindSoftLockTarget(float MaxDistance) const;
	/** True if the mission is over or the game is paused: ignore gameplay input. */
	bool IsInputBlocked() const;
	ATransoxianaGameMode* GetTransoxianaGameMode() const;
	float GetLookSensitivity() const;
	bool IsLookInverted() const;

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// ---- Tuning (edit in a Blueprint child or here) ----
	UPROPERTY(EditAnywhere, Category = "Movement") float RunSpeed = 450.f;
	UPROPERTY(EditAnywhere, Category = "Movement") float SprintSpeed = 700.f;
	UPROPERTY(EditAnywhere, Category = "Movement") float CrouchSpeed = 200.f;
	UPROPERTY(EditAnywhere, Category = "Movement") float CombatMoveSpeed = 250.f;
	UPROPERTY(EditAnywhere, Category = "Movement") float DodgeImpulse = 1100.f;

	UPROPERTY(EditAnywhere, Category = "Stamina") float MaxStamina = 100.f;
	UPROPERTY(EditAnywhere, Category = "Stamina") float StaminaRegenPerSecond = 22.f;
	UPROPERTY(EditAnywhere, Category = "Stamina") float SprintStaminaPerSecond = 18.f;
	UPROPERTY(EditAnywhere, Category = "Stamina") float AttackStaminaCost = 12.f;
	UPROPERTY(EditAnywhere, Category = "Stamina") float DodgeStaminaCost = 20.f;
	UPROPERTY(EditAnywhere, Category = "Stamina") float BlockStaminaCost = 25.f;

	UPROPERTY(EditAnywhere, Category = "Combat") float SwordDamage = 34.f;
	UPROPERTY(EditAnywhere, Category = "Combat") float SwordReach = 130.f;
	UPROPERTY(EditAnywhere, Category = "Combat") float SwordAttackDuration = 0.55f;
	/** Pressing block this close before a hit lands counts as a parry. */
	UPROPERTY(EditAnywhere, Category = "Combat") float ParryWindow = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Combat") float TakedownRange = 200.f;
	UPROPERTY(EditAnywhere, Category = "Combat") float ArrowMinDamage = 25.f;
	UPROPERTY(EditAnywhere, Category = "Combat") float ArrowMaxDamage = 70.f;
	UPROPERTY(EditAnywhere, Category = "Combat") float FullDrawTime = 1.0f;

private:
	float Stamina = 100.f;
	float TimeSinceStaminaSpent = 10.f;
	EWeaponMode WeaponMode = EWeaponMode::Sabre;

	bool bSprintHeld = false;
	bool bSprintExhausted = false;
	bool bAltHeld = false;
	bool bAiming = false;
	bool bBlocking = false;
	float BlockStartTime = -100.f;
	bool bDrawing = false;
	float DrawStartTime = 0.f;

	bool bAttacking = false;
	bool bAttackQueued = false;
	bool bAttackHitApplied = false;
	bool bMirrorSwing = false;
	float AttackElapsed = 0.f;

	float InvulnerableUntil = -1.f;
	float NoiseTimer = 0.f;
	int32 StealthZoneCount = 0;
	FString InteractPrompt;
};
