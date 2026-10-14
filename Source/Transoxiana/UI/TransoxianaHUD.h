#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TransoxianaHUD.generated.h"

class ATransoxianaCharacter;
class ATransoxianaGameMode;
class ATransoxianaPlayerController;

/**
 * Minimal HUD drawn with the canvas (no widget assets needed): health and stamina,
 * weapon, objective and marker, guard awareness meters, crosshair and draw power,
 * prompts, short messages, the opening briefing, pause/settings menus, and the
 * mission end screen.
 */
UCLASS()
class TRANSOXIANA_API ATransoxianaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Short on-screen message, e.g. "Parry!". */
	void ShowToast(const FString& Message, float Duration = 1.6f);

protected:
	virtual void BeginPlay() override;

private:
	void DrawBars(const ATransoxianaCharacter* Player);
	void DrawObjective(const ATransoxianaGameMode* GM);
	void DrawEnemyAwareness();
	void DrawCrosshair(const ATransoxianaCharacter* Player);
	void DrawPrompts(const ATransoxianaCharacter* Player);
	void DrawBriefing();
	void DrawMenu(const ATransoxianaPlayerController* PC);
	void DrawEndScreen(const ATransoxianaGameMode* GM);

	void Text(const FString& InText, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCentered = false, bool bLarge = false);
	void Bar(float X, float Y, float W, float H, float Fill, const FLinearColor& Color);

	float UIScale = 1.f;
	FString ToastText;
	float ToastUntil = -1.f;
	float StartTime = 0.f;
};
