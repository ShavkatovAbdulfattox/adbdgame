#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExtractionZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/** Where Aybike the falconer waits. Reaching it with the dispatch ends the mission. */
UCLASS()
class TRANSOXIANA_API AExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	AExtractionZone();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Extraction")
	TObjectPtr<UBoxComponent> Volume;

	UPROPERTY(VisibleAnywhere, Category = "Extraction")
	TObjectPtr<UStaticMeshComponent> FalconerBody;

	UPROPERTY(VisibleAnywhere, Category = "Extraction")
	TObjectPtr<UStaticMeshComponent> FalconerHead;

	UPROPERTY(VisibleAnywhere, Category = "Extraction")
	TObjectPtr<UStaticMeshComponent> Falcon;

	UPROPERTY(VisibleAnywhere, Category = "Extraction")
	TObjectPtr<UPointLightComponent> Lantern;
};
