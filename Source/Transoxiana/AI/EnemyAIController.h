#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Core/TransoxianaTypes.h"
#include "EnemyAIController.generated.h"

class AEnemyCharacter;
class ATransoxianaCharacter;
class ATransoxianaGameMode;

/**
 * Guard brain as a readable state machine:
 *
 *   Patrol --(awareness >= 35 % or noise)--> Suspicious --(awareness 100 %)--> Combat
 *     ^                                          |                               |
 *     +----------(nothing found)-----------------+          (lost player 5 s)    v
 *     +-------------------------------(search finished)-------------------- Search
 *
 * Vision: cone + line of sight; the awareness meter fills faster when the player is
 * close, centred in view, standing, moving fast, or after the alarm is raised.
 * Movement uses the NavMesh when one exists, otherwise steers directly.
 */
UCLASS()
class TRANSOXIANA_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;

	EEnemyState GetState() const { return State; }
	float GetAwareness() const { return Awareness; }

	/** A noise (footsteps, arrow impact, fighting) happened at Location. */
	void HearNoise(const FVector& Location);
	/** This guard was hurt. */
	void OnDamaged(APawn* Attacker);
	/** Another guard raised the alarm. */
	void AlertTo(const FVector& Location);
	void OnPawnDied();

	// ---- Tuning ----
	UPROPERTY(EditAnywhere, Category = "Perception") float SightRange = 2000.f;
	UPROPERTY(EditAnywhere, Category = "Perception") float ArcherSightRange = 2800.f;
	/** Cosine of the half view angle (0.5 = 60 degrees each side). */
	UPROPERTY(EditAnywhere, Category = "Perception") float ViewConeCos = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Perception") float CloseSenseRadius = 200.f;
	UPROPERTY(EditAnywhere, Category = "Perception") float AwarenessGainRate = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Perception") float AwarenessDecayRate = 0.12f;
	UPROPERTY(EditAnywhere, Category = "Perception") float SuspiciousThreshold = 0.35f;
	/** Crouched in a hiding spot, the player is invisible beyond this distance. */
	UPROPERTY(EditAnywhere, Category = "Perception") float HidingSpotRevealDistance = 300.f;
	UPROPERTY(EditAnywhere, Category = "Behaviour") float AlertShareRadius = 1500.f;
	UPROPERTY(EditAnywhere, Category = "Behaviour") float LoseTargetTime = 5.f;
	UPROPERTY(EditAnywhere, Category = "Behaviour") float SearchDuration = 8.f;
	UPROPERTY(EditAnywhere, Category = "Behaviour") float PatrolWaitTime = 2.f;

private:
	void UpdatePerception(float DeltaSeconds);
	bool CanSeePlayer(const ATransoxianaCharacter* Player, float& OutDistance, float& OutCenterDot) const;
	void UpdatePatrol(float DeltaSeconds);
	void UpdateSuspicious(float DeltaSeconds);
	void UpdateCombat(float DeltaSeconds);
	void UpdateSearch(float DeltaSeconds);
	void EnterCombat(bool bShareAlert);
	void EnterState(EEnemyState NewState);

	/** Moves toward Goal. Returns true when within AcceptanceRadius. */
	bool MoveToward(const FVector& Goal, float AcceptanceRadius);
	void HaltMovement();
	void LookAround(const FVector& Center);

	AEnemyCharacter* GetEnemy() const;
	ATransoxianaCharacter* GetPlayer() const;
	ATransoxianaGameMode* GetGameMode() const;

	EEnemyState State = EEnemyState::Patrol;
	float Awareness = 0.f;
	float StateTimer = 0.f;
	bool bSeesPlayer = false;
	float TimeSinceSeen = 0.f;
	FVector LastKnownPlayerLocation = FVector::ZeroVector;
	FVector InvestigatePoint = FVector::ZeroVector;
	bool bArrivedAtInvestigatePoint = false;
	bool bPersonallyHeightened = false;

	int32 PatrolIndex = 0;
	float PatrolWaitTimer = 0.f;
	FVector HomeLocation = FVector::ZeroVector;
	FRotator HomeRotation = FRotator::ZeroRotator;

	float AttackCooldown = 1.f;
	float ArrowCooldown = 1.5f;

	FVector CurrentMoveGoal = FVector::ZeroVector;
	bool bHasMoveGoal = false;
	float DirectSteeringUntil = -1.f;
};
