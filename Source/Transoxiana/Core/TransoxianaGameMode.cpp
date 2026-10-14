#include "Core/TransoxianaGameMode.h"

#include "AI/EnemyAIController.h"
#include "AI/EnemyCharacter.h"
#include "Core/TransoxianaPlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Player/TransoxianaCharacter.h"
#include "Transoxiana.h"
#include "UI/TransoxianaHUD.h"
#include "World/OutpostBuilder.h"

ATransoxianaGameMode::ATransoxianaGameMode()
{
	DefaultPawnClass = ATransoxianaCharacter::StaticClass();
	PlayerControllerClass = ATransoxianaPlayerController::StaticClass();
	HUDClass = ATransoxianaHUD::StaticClass();
}

AOutpostBuilder* ATransoxianaGameMode::EnsureOutpost()
{
	if (!Outpost)
	{
		for (TActorIterator<AOutpostBuilder> It(GetWorld()); It; ++It)
		{
			Outpost = *It;
			break;
		}
	}
	if (!Outpost)
	{
		// Nothing placed in the level: build the outpost at the world origin.
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Outpost = GetWorld()->SpawnActor<AOutpostBuilder>(AOutpostBuilder::StaticClass(), FTransform::Identity, Params);
		UE_LOG(LogTransoxiana, Log, TEXT("No AOutpostBuilder in the level - spawned one at the origin."));
	}
	if (Outpost)
	{
		Outpost->BuildRuntimeActors();
	}
	return Outpost;
}

void ATransoxianaGameMode::StartPlay()
{
	EnsureOutpost();
	Super::StartPlay();
	MissionStartTime = GetWorld()->GetTimeSeconds();
}

AActor* ATransoxianaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (AOutpostBuilder* Builder = EnsureOutpost())
	{
		if (AActor* Start = Builder->GetPlayerStart())
		{
			return Start;
		}
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

// ---------------------------------------------------------------------------
// Mission flow
// ---------------------------------------------------------------------------

void ATransoxianaGameMode::OnDispatchTaken()
{
	if (MissionState != EMissionState::Infiltrate)
	{
		return;
	}
	MissionState = EMissionState::Escape;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (ATransoxianaHUD* HUD = Cast<ATransoxianaHUD>(PC->GetHUD()))
		{
			HUD->ShowToast(TEXT("Dispatch taken. The garrison is uneasy - get out."), 4.f);
		}
	}
}

void ATransoxianaGameMode::OnPlayerExtracted()
{
	if (MissionState != EMissionState::Escape)
	{
		return;
	}
	MissionState = EMissionState::Success;
	MissionEndTime = GetWorld()->GetTimeSeconds();
}

void ATransoxianaGameMode::OnPlayerDied()
{
	if (IsMissionOver())
	{
		return;
	}
	MissionState = EMissionState::Failed;
	MissionEndTime = GetWorld()->GetTimeSeconds();
}

void ATransoxianaGameMode::RestartMission()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

FText ATransoxianaGameMode::GetObjectiveText() const
{
	switch (MissionState)
	{
	case EMissionState::Infiltrate:
		return NSLOCTEXT("Transoxiana", "ObjSteal", "Steal the sealed dispatch from the domed hall");
	case EMissionState::Escape:
		return NSLOCTEXT("Transoxiana", "ObjEscape", "Escape to Aybike the falconer (north-east, outside the walls)");
	case EMissionState::Success:
		return NSLOCTEXT("Transoxiana", "ObjDone", "Mission complete");
	default:
		return NSLOCTEXT("Transoxiana", "ObjFailed", "Mission failed");
	}
}

bool ATransoxianaGameMode::GetObjectiveLocation(FVector& OutLocation) const
{
	if (!Outpost || IsMissionOver())
	{
		return false;
	}
	OutLocation = (MissionState == EMissionState::Infiltrate) ? Outpost->GetDispatchLocation() : Outpost->GetExtractionLocation();
	return true;
}

// ---------------------------------------------------------------------------
// Guards
// ---------------------------------------------------------------------------

void ATransoxianaGameMode::RegisterEnemy(AEnemyCharacter* Enemy)
{
	if (Enemy)
	{
		Enemies.AddUnique(Enemy);
	}
}

void ATransoxianaGameMode::ReportNoise(const FVector& Location, float Radius, AActor* Source)
{
	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (!Enemy || Enemy == Source || Enemy->IsDead())
		{
			continue;
		}
		if (FVector::Dist(Enemy->GetActorLocation(), Location) <= Radius)
		{
			if (AEnemyAIController* AI = Enemy->GetEnemyController())
			{
				AI->HearNoise(Location);
			}
		}
	}
}

void ATransoxianaGameMode::NotifyEnemyEnteredCombat(AEnemyCharacter* Enemy)
{
	// Count a "detection" only when the alarm starts, not for every guard that joins.
	for (AEnemyCharacter* Other : Enemies)
	{
		if (Other && Other != Enemy && !Other->IsDead() && Other->GetEnemyState() == EEnemyState::Combat)
		{
			return;
		}
	}
	++TimesDetected;
}

void ATransoxianaGameMode::NotifyEnemyKilled(AEnemyCharacter* Enemy, bool bTakedown)
{
	++Kills;
	if (bTakedown)
	{
		++Takedowns;
	}
}

bool ATransoxianaGameMode::CanEnemyStartAttack() const
{
	int32 Attacking = 0;
	for (const AEnemyCharacter* Enemy : Enemies)
	{
		if (Enemy && !Enemy->IsDead() && Enemy->IsAttacking())
		{
			++Attacking;
		}
	}
	return Attacking < MaxSimultaneousAttackers;
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

float ATransoxianaGameMode::GetMissionTime() const
{
	const float End = (MissionEndTime >= 0.f) ? MissionEndTime : GetWorld()->GetTimeSeconds();
	return End - MissionStartTime;
}

FText ATransoxianaGameMode::GetRankText() const
{
	if (TimesDetected == 0 && Kills == 0)
	{
		return NSLOCTEXT("Transoxiana", "RankShadow", "SHADOW - never seen, no blood spilled");
	}
	if (TimesDetected == 0)
	{
		return NSLOCTEXT("Transoxiana", "RankFalcon", "FALCON - never seen");
	}
	return NSLOCTEXT("Transoxiana", "RankSword", "SWORD - the hard way");
}
