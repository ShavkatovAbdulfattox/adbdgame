#include "UI/TransoxianaHUD.h"

#include "AI/EnemyCharacter.h"
#include "Core/TransoxianaGameMode.h"
#include "Core/TransoxianaPlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Player/TransoxianaCharacter.h"

namespace
{
	const FLinearColor Ink(0.95f, 0.92f, 0.85f);
	const FLinearColor Dim(0.f, 0.f, 0.f, 0.55f);
	const FLinearColor HealthRed(0.75f, 0.15f, 0.12f);
	const FLinearColor StaminaGold(0.85f, 0.7f, 0.25f);
	const FLinearColor TileBlue(0.18f, 0.61f, 0.6f);
	const FLinearColor Warning(1.f, 0.82f, 0.2f);
	const FLinearColor Danger(1.f, 0.2f, 0.15f);
}

void ATransoxianaHUD::BeginPlay()
{
	Super::BeginPlay();
	StartTime = GetWorld()->GetTimeSeconds();
}

void ATransoxianaHUD::ShowToast(const FString& Message, float Duration)
{
	ToastText = Message;
	ToastUntil = GetWorld()->GetRealTimeSeconds() + Duration;
}

void ATransoxianaHUD::Text(const FString& InText, float X, float Y, const FLinearColor& Color, float Scale, bool bCentered, bool bLarge)
{
	UFont* Font = bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont();
	const float FinalScale = Scale * UIScale;
	if (bCentered)
	{
		float W = 0.f, H = 0.f;
		GetTextSize(InText, W, H, Font, FinalScale);
		X -= W * 0.5f;
	}
	// Drop shadow for readability on bright sand.
	DrawText(InText, FLinearColor(0.f, 0.f, 0.f, 0.8f), X + 2.f, Y + 2.f, Font, FinalScale);
	DrawText(InText, Color, X, Y, Font, FinalScale);
}

void ATransoxianaHUD::Bar(float X, float Y, float W, float H, float Fill, const FLinearColor& Color)
{
	DrawRect(Dim, X - 2.f, Y - 2.f, W + 4.f, H + 4.f);
	DrawRect(Color, X, Y, W * FMath::Clamp(Fill, 0.f, 1.f), H);
}

void ATransoxianaHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	UIScale = FMath::Max(0.6f, Canvas->ClipY / 1080.f);

	const ATransoxianaPlayerController* PC = Cast<ATransoxianaPlayerController>(PlayerOwner);
	const ATransoxianaCharacter* Player = PC ? Cast<ATransoxianaCharacter>(PC->GetPawn()) : nullptr;
	const ATransoxianaGameMode* GM = GetWorld()->GetAuthGameMode<ATransoxianaGameMode>();

	if (GM && GM->IsMissionOver())
	{
		DrawEndScreen(GM);
		return;
	}

	if (Player)
	{
		DrawEnemyAwareness();
		DrawBars(Player);
		DrawCrosshair(Player);
		DrawPrompts(Player);
	}
	if (GM)
	{
		DrawObjective(GM);
	}
	DrawBriefing();

	if (ToastUntil > GetWorld()->GetRealTimeSeconds())
	{
		Text(ToastText, Canvas->ClipX * 0.5f, Canvas->ClipY * 0.30f, Ink, 1.1f, true);
	}

	if (PC && PC->GetMenuPage() != ETransoxianaMenuPage::None)
	{
		DrawMenu(PC);
	}
}

void ATransoxianaHUD::DrawBars(const ATransoxianaCharacter* Player)
{
	const float X = 40.f * UIScale;
	const float Y = Canvas->ClipY - 120.f * UIScale;
	const float W = 320.f * UIScale;
	const float H = 14.f * UIScale;

	Bar(X, Y, W, H, Player->GetHealth() / Player->GetMaxHealth(), HealthRed);
	Bar(X, Y + 24.f * UIScale, W * 0.8f, H * 0.7f, Player->GetStamina() / Player->GetMaxStamina(), StaminaGold);

	FString Weapon = (Player->GetWeaponMode() == EWeaponMode::Sabre)
		? TEXT("SABRE   LMB attack  |  RMB block (tap just before a hit = parry)")
		: TEXT("BOW   hold RMB aim  |  hold & release LMB to shoot");
	Text(Weapon, X, Y + 44.f * UIScale, Ink, 0.8f);

	if (Player->IsInHidingSpot())
	{
		Text(TEXT("HIDDEN"), X, Y - 34.f * UIScale, TileBlue, 1.0f);
	}
	else if (Player->bIsCrouched)
	{
		Text(TEXT("Sneaking"), X, Y - 34.f * UIScale, Ink, 0.9f);
	}
}

void ATransoxianaHUD::DrawObjective(const ATransoxianaGameMode* GM)
{
	const float X = 40.f * UIScale;
	const float Y = 36.f * UIScale;
	Text(TEXT("THE SEALED DISPATCH"), X, Y, TileBlue, 0.8f);
	Text(GM->GetObjectiveText().ToString(), X, Y + 22.f * UIScale, Ink, 1.0f);

	FVector Target;
	if (!GM->GetObjectiveLocation(Target))
	{
		return;
	}

	// Marker: projected onto the screen, clamped to the edges when off-screen.
	FVector Screen = Project(Target + FVector(0.f, 0.f, 150.f), false);
	const float Margin = 40.f * UIScale;
	FVector CamLoc;
	FRotator CamRot;
	PlayerOwner->GetPlayerViewPoint(CamLoc, CamRot);
	const bool bBehind = FVector::DotProduct(CamRot.Vector(), Target - CamLoc) < 0.f;
	if (bBehind)
	{
		Screen.X = Canvas->ClipX - Screen.X;
		Screen.Y = Canvas->ClipY - Margin;
	}
	Screen.X = FMath::Clamp(Screen.X, Margin, Canvas->ClipX - Margin);
	Screen.Y = FMath::Clamp(Screen.Y, Margin, Canvas->ClipY - Margin);

	const float S = 10.f * UIScale;
	DrawRect(TileBlue, Screen.X - S * 0.5f, Screen.Y - S * 0.5f, S, S);
	const float Meters = FVector::Dist(CamLoc, Target) / 100.f;
	Text(FString::Printf(TEXT("%d m"), FMath::RoundToInt(Meters)), Screen.X, Screen.Y + 8.f * UIScale, Ink, 0.75f, true);
}

void ATransoxianaHUD::DrawEnemyAwareness()
{
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const AEnemyCharacter* Enemy = *It;
		if (!Enemy || Enemy->IsDead())
		{
			continue;
		}
		const float Awareness = Enemy->GetAwareness();
		const EEnemyState State = Enemy->GetEnemyState();
		if (Awareness <= 0.01f && State == EEnemyState::Patrol)
		{
			continue;
		}
		const FVector Screen = Project(Enemy->GetHeadLocation() + FVector(0.f, 0.f, 45.f), false);
		if (Screen.Z <= 0.f || Screen.X < 0.f || Screen.X > Canvas->ClipX || Screen.Y < 0.f || Screen.Y > Canvas->ClipY)
		{
			continue;
		}

		const bool bCombat = State == EEnemyState::Combat;
		const FLinearColor Color = bCombat ? Danger : Warning;
		const float W = 44.f * UIScale;
		Bar(Screen.X - W * 0.5f, Screen.Y, W, 5.f * UIScale, Awareness, Color);

		FString Symbol = TEXT("?");
		if (bCombat)
		{
			Symbol = Enemy->IsTelegraphing() ? TEXT("!! ATTACK") : TEXT("!");
		}
		else if (State == EEnemyState::Search)
		{
			Symbol = TEXT("? searching");
		}
		if (Enemy->IsStaggered())
		{
			Symbol = TEXT("STAGGERED");
		}
		if (GetWorld()->GetTimeSeconds() - Enemy->GetLastBlockTime() < 0.6f)
		{
			Symbol = TEXT("blocked");
		}
		Text(Symbol, Screen.X, Screen.Y - 26.f * UIScale, Color, Enemy->IsTelegraphing() ? 1.1f : 0.9f, true);
	}
}

void ATransoxianaHUD::DrawCrosshair(const ATransoxianaCharacter* Player)
{
	if (!Player->IsAiming())
	{
		return;
	}
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;
	const float L = 8.f * UIScale;
	DrawLine(CX - L, CY, CX + L, CY, Ink, 1.5f);
	DrawLine(CX, CY - L, CX, CY + L, Ink, 1.5f);
	const float Draw = Player->GetDrawAlpha();
	if (Draw > 0.f)
	{
		Bar(CX - 30.f * UIScale, CY + 18.f * UIScale, 60.f * UIScale, 4.f * UIScale, Draw, Draw >= 1.f ? TileBlue : Ink);
	}
}

void ATransoxianaHUD::DrawPrompts(const ATransoxianaCharacter* Player)
{
	const FString& Prompt = Player->GetInteractPrompt();
	if (!Prompt.IsEmpty())
	{
		Text(Prompt, Canvas->ClipX * 0.5f, Canvas->ClipY * 0.68f, Ink, 1.0f, true);
	}
}

void ATransoxianaHUD::DrawBriefing()
{
	const float Elapsed = GetWorld()->GetTimeSeconds() - StartTime;
	if (Elapsed > 14.f)
	{
		return;
	}
	const float X = Canvas->ClipX * 0.5f;
	float Y = Canvas->ClipY * 0.14f;
	const float Line = 26.f * UIScale;
	DrawRect(Dim, X - 420.f * UIScale, Y - 12.f * UIScale, 840.f * UIScale, 9.6f * Line);
	Text(TEXT("Autumn 1387 - a rabat on the road from Bukhara to Samarkand"), X, Y, TileBlue, 0.85f, true); Y += Line * 1.3f;
	Text(TEXT("\"The raiders holding this rabat carry a sealed letter naming who in Bukhara"), X, Y, Ink, 0.85f, true); Y += Line;
	Text(TEXT("will open a gate to them. Take it before dawn. Kill only if you must.\""), X, Y, Ink, 0.85f, true); Y += Line * 1.4f;
	Text(TEXT("WASD move | Mouse look | Shift sprint | C / Ctrl crouch | Space jump | Alt dodge"), X, Y, Ink, 0.75f, true); Y += Line;
	Text(TEXT("LMB attack/shoot | RMB block/aim | Q switch sabre/bow | E take/takedown | P pause"), X, Y, Ink, 0.75f, true); Y += Line;
	Text(TEXT("Crouch in hay and shade to hide. Shoot an arrow near a guard to distract him."), X, Y, Ink, 0.75f, true); Y += Line;
	Text(TEXT("Historical setting; the people and this mission are fictional."), X, Y, Ink, 0.65f, true);
}

void ATransoxianaHUD::DrawMenu(const ATransoxianaPlayerController* PC)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	const float X = Canvas->ClipX * 0.5f;
	float Y = Canvas->ClipY * 0.28f;
	const bool bSettings = PC->GetMenuPage() == ETransoxianaMenuPage::Settings;
	Text(bSettings ? TEXT("SETTINGS") : TEXT("PAUSED"), X, Y, TileBlue, 1.0f, true, true);
	Y += 70.f * UIScale;

	const TArray<FString> Lines = PC->GetMenuLines();
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		const bool bSelected = (i == PC->GetMenuSelection());
		Text(bSelected ? FString::Printf(TEXT(">  %s  <"), *Lines[i]) : Lines[i], X, Y, bSelected ? Warning : Ink, 1.1f, true);
		Y += 40.f * UIScale;
	}
	Y += 30.f * UIScale;
	Text(bSettings ? TEXT("Up/Down select  |  Left/Right change  |  Backspace back")
	               : TEXT("Up/Down select  |  Enter confirm  |  P / Esc resume"), X, Y, Ink, 0.75f, true);
}

void ATransoxianaHUD::DrawEndScreen(const ATransoxianaGameMode* GM)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	const float X = Canvas->ClipX * 0.5f;
	float Y = Canvas->ClipY * 0.25f;
	const bool bWon = GM->GetMissionState() == EMissionState::Success;

	Text(bWon ? TEXT("MISSION COMPLETE") : TEXT("BAHRAM HAS FALLEN"), X, Y, bWon ? TileBlue : Danger, 1.4f, true, true);
	Y += 80.f * UIScale;
	if (bWon)
	{
		Text(TEXT("Aybike's falcon carries the dispatch to Samarkand before dawn."), X, Y, Ink, 0.9f, true);
		Y += 50.f * UIScale;
		Text(GM->GetRankText().ToString(), X, Y, Warning, 1.1f, true);
		Y += 50.f * UIScale;
	}
	const int32 Seconds = FMath::RoundToInt(GM->GetMissionTime());
	Text(FString::Printf(TEXT("Time %d:%02d   |   Times detected %d   |   Takedowns %d   |   Guards killed %d"),
		Seconds / 60, Seconds % 60, GM->GetTimesDetected(), GM->GetTakedowns(), GM->GetKills()), X, Y, Ink, 0.85f, true);
	Y += 70.f * UIScale;
	Text(TEXT("Press Enter / (A) to play again"), X, Y, Ink, 0.9f, true);
}
