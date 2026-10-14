#pragma once

#include "CoreMinimal.h"
#include "TransoxianaTypes.generated.h"

/** Progress of the "Sealed Dispatch" mission. */
UENUM(BlueprintType)
enum class EMissionState : uint8
{
	Infiltrate	UMETA(DisplayName = "Infiltrate - steal the dispatch"),
	Escape		UMETA(DisplayName = "Escape - reach the falconer"),
	Success,
	Failed
};

/** What an enemy guard is currently doing. */
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Patrol,
	Suspicious,	// Saw or heard something, investigating
	Combat,		// Fully alerted, attacking the player
	Search,		// Lost the player, searching the last known position
	Dead
};

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Swordsman,
	Archer
};

UENUM(BlueprintType)
enum class EWeaponMode : uint8
{
	Sabre,
	Bow
};
