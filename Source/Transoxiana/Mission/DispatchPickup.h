#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DispatchPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class ATransoxianaCharacter;

/** The sealed dispatch: the mission's objective item. Press Interact next to it. */
UCLASS()
class TRANSOXIANA_API ADispatchPickup : public AActor
{
	GENERATED_BODY()

public:
	ADispatchPickup();

	virtual void Tick(float DeltaSeconds) override;

	void Collect(ATransoxianaCharacter* Collector);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Dispatch")
	TObjectPtr<USphereComponent> PickupRadius;

	UPROPERTY(VisibleAnywhere, Category = "Dispatch")
	TObjectPtr<UStaticMeshComponent> Scroll;

	UPROPERTY(VisibleAnywhere, Category = "Dispatch")
	TObjectPtr<UStaticMeshComponent> Seal;

	UPROPERTY(VisibleAnywhere, Category = "Dispatch")
	TObjectPtr<UPointLightComponent> Glow;

private:
	float Time = 0.f;
};
