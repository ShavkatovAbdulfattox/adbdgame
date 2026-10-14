#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/TransoxianaTypes.h"
#include "TransoxianaGameMode.generated.h"

class AEnemyCharacter;
class AOutpostBuilder;

/**
 * Rules of the "Sealed Dispatch" mission: objective flow, alarm state, noise
 * propagation to guards, and end-of-mission statistics.
 */
UCLASS()
class TRANSOXIANA_API ATransoxianaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATransoxianaGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	// ---- Mission flow ----
	void OnDispatchTaken();
	void OnPlayerExtracted();
	void OnPlayerDied();
	void RestartMission();

	EMissionState GetMissionState() const { return MissionState; }
	bool IsMissionOver() const { return MissionState == EMissionState::Success || MissionState == EMissionState::Failed; }
	FText GetObjectiveText() const;
	/** Where the HUD marker should point. Returns false if there is no marker. */
	bool GetObjectiveLocation(FVector& OutLocation) const;
	/** After the theft every guard is on heightened alert (faster detection). */
	bool IsHeightenedAlert() const { return MissionState != EMissionState::Infiltrate; }

	// ---- Guards ----
	void RegisterEnemy(AEnemyCharacter* Enemy);
	void ReportNoise(const FVector& Location, float Radius, AActor* Source);
	void NotifyEnemyEnteredCombat(AEnemyCharacter* Enemy);
	void NotifyEnemyKilled(AEnemyCharacter* Enemy, bool bTakedown);
	/** Limits how many guards swing at the player at the same time. */
	bool CanEnemyStartAttack() const;

	// ---- Stats ----
	int32 GetTimesDetected() const { return TimesDetected; }
	int32 GetKills() const { return Kills; }
	int32 GetTakedowns() const { return Takedowns; }
	float GetMissionTime() const;
	FText GetRankText() const;

	UPROPERTY(EditAnywhere, Category = "Mission")
	int32 MaxSimultaneousAttackers = 2;

private:
	AOutpostBuilder* EnsureOutpost();

	UPROPERTY()
	TObjectPtr<AOutpostBuilder> Outpost;

	UPROPERTY()
	TArray<TObjectPtr<AEnemyCharacter>> Enemies;

	EMissionState MissionState = EMissionState::Infiltrate;
	float MissionStartTime = 0.f;
	float MissionEndTime = -1.f;
	int32 TimesDetected = 0;
	int32 Kills = 0;
	int32 Takedowns = 0;
};
