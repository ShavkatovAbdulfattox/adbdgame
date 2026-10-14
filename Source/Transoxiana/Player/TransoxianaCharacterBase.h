#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TransoxianaCharacterBase.generated.h"

class UStaticMeshComponent;
class USceneComponent;

/**
 * Shared base for the player and enemies: health, optional mannequin art with a
 * primitive placeholder fallback, and simple procedural weapon visuals.
 */
UCLASS(Abstract, Config = Game)
class TRANSOXIANA_API ATransoxianaCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ATransoxianaCharacterBase();

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bDead; }

	/** World position roughly at the top of the head. */
	FVector GetHeadLocation() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Loads the configured skeletal mesh/anim class, or shows the placeholder body. */
	void SetupAppearance(const FLinearColor& BodyColor);

	/** Turns the body into a ragdoll (if possible) or tips the placeholder over. */
	void PlayDeathPose();

	/** Swings the sword visual. Alpha 0..1 across the swing; bMirror alternates direction. */
	void PoseSword(float SwingAlpha, bool bMirror, bool bGuard);

	bool HasSkeletalMeshArt() const { return bHasSkeletalMeshArt; }

	// ---- Config (Config/DefaultGame.ini) ----
	UPROPERTY(Config, EditDefaultsOnly, Category = "Appearance")
	FString CharacterMeshPath;

	UPROPERTY(Config, EditDefaultsOnly, Category = "Appearance")
	FString FallbackCharacterMeshPath;

	UPROPERTY(Config, EditDefaultsOnly, Category = "Appearance")
	FString AnimClassPath;

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, Category = "Appearance")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(VisibleAnywhere, Category = "Appearance")
	TObjectPtr<UStaticMeshComponent> PlaceholderHead;

	UPROPERTY(VisibleAnywhere, Category = "Weapons")
	TObjectPtr<USceneComponent> SwordPivot;

	UPROPERTY(VisibleAnywhere, Category = "Weapons")
	TObjectPtr<UStaticMeshComponent> SwordBlade;

	UPROPERTY(VisibleAnywhere, Category = "Weapons")
	TObjectPtr<UStaticMeshComponent> BowStave;

	// ---- Health ----
	UPROPERTY(EditAnywhere, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Health")
	float Health = 100.f;

	bool bDead = false;

private:
	bool bHasSkeletalMeshArt = false;
	float PlaceholderStandingScaleZ = 1.f;
};
