#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StealthZone.generated.h"

class UBoxComponent;

/**
 * A hiding spot (deep shade, hay, cloth awning). While the player is crouched inside,
 * guards can only spot them at very close range.
 */
UCLASS()
class TRANSOXIANA_API AStealthZone : public AActor
{
	GENERATED_BODY()

public:
	AStealthZone();

	void SetExtent(const FVector& HalfExtent);

protected:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, Category = "Stealth")
	TObjectPtr<UBoxComponent> Volume;
};
