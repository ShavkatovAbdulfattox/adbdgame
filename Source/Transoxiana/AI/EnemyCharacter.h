#pragma once

#include "CoreMinimal.h"
#include "Player/TransoxianaCharacterBase.h"
#include "Core/TransoxianaTypes.h"
#include "EnemyCharacter.generated.h"

class UStaticMeshComponent;
class AEnemyAIController;

/**
 * A raider guard. The "brain" (perception, states, decisions) lives in
 * AEnemyAIController; this class holds the body: health, telegraphed sword attacks,
 * blocking, stagger, arrows for archers, and death.
 */
UCLASS(Config = Game)
class TRANSOXIANA_API AEnemyCharacter : public ATransoxianaCharacterBase
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Set before FinishSpawning (see AOutpostBuilder). */
	UPROPERTY(EditAnywhere, Category = "Enemy")
	EEnemyType EnemyType = EEnemyType::Swordsman;

	/** World-space patrol route. Empty = stand guard at the spawn point. */
	UPROPERTY(EditAnywhere, Category = "Enemy")
	TArray<FVector> PatrolPoints;

	UPROPERTY(EditAnywhere, Category = "Enemy")
	float PatrolSpeed = 170.f;

	UPROPERTY(EditAnywhere, Category = "Enemy")
	float ChaseSpeed = 430.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
	float SwordDamage = 22.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
	float ArrowDamage = 16.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
	float AttackWindup = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
	float AttackRecovery = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
	float AttackReach = 190.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
	float BlockChance = 0.35f;

	// ---- Combat actions (called by the AI controller) ----
	void StartMeleeAttack();
	void ShootArrowAt(const AActor* Target);
	void Stagger(float Duration);
	void KillByTakedown(AController* Killer);

	// ---- Queries ----
	bool IsAttacking() const { return bAttacking; }
	bool IsTelegraphing() const { return bAttacking && AttackElapsed < AttackWindup; }
	bool IsStaggered() const;
	bool CanBeTakenDown() const;
	EEnemyState GetEnemyState() const;
	float GetAwareness() const;
	AEnemyAIController* GetEnemyController() const;
	float GetLastBlockTime() const { return LastBlockTime; }

protected:
	virtual void BeginPlay() override;

	void Die(AController* Killer, bool bTakedown);

	UPROPERTY(VisibleAnywhere, Category = "Appearance")
	TObjectPtr<UStaticMeshComponent> Helmet;

private:
	bool bAttacking = false;
	bool bAttackHitApplied = false;
	float AttackElapsed = 0.f;
	float StaggeredUntil = -1.f;
	float LastBlockTime = -100.f;
};
