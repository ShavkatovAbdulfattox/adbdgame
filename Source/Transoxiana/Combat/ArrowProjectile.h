#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArrowProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/**
 * An arrow with gravity drop. Damages characters (head hits deal extra damage) and
 * sticks into the world. Player arrows that land make noise, which guards investigate —
 * the prototype's "distraction" tool.
 */
UCLASS()
class TRANSOXIANA_API AArrowProjectile : public AActor
{
	GENERATED_BODY()

public:
	AArrowProjectile();

	/** Call right after spawning. */
	void Launch(const FVector& Velocity, float InDamage, bool bInFiredByPlayer);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnArrowHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, Category = "Arrow")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Arrow")
	TObjectPtr<UStaticMeshComponent> Shaft;

	UPROPERTY(VisibleAnywhere, Category = "Arrow")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(EditAnywhere, Category = "Arrow")
	float HeadshotMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Arrow")
	float ImpactNoiseRadius = 900.f;

private:
	float Damage = 30.f;
	bool bFiredByPlayer = false;
	bool bHasHit = false;
};
