#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TransoxianaPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UWindSynthComponent;
struct FKey;

/** Which page of the in-game menu is open. */
enum class ETransoxianaMenuPage : uint8
{
	None,
	Pause,
	Settings
};

/**
 * Owns input definitions (created in code, no assets needed), the pause and settings
 * menus (keyboard/gamepad driven, drawn by ATransoxianaHUD), saved player settings,
 * and the procedural ambient wind.
 */
UCLASS(Config = Game)
class TRANSOXIANA_API ATransoxianaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATransoxianaPlayerController();

	/** Creates all input actions and key mappings once. Safe to call repeatedly. */
	void EnsureInputCreated();

	// ---- Saved settings (Saved/Config/<Platform>/Game.ini) ----
	UPROPERTY(Config) float MouseSensitivity = 1.0f;
	UPROPERTY(Config) bool bInvertY = false;
	UPROPERTY(Config) float AmbientVolume = 0.8f;

	// ---- Menu (read by the HUD) ----
	ETransoxianaMenuPage GetMenuPage() const { return MenuPage; }
	int32 GetMenuSelection() const { return MenuSelection; }
	/** Lines of the current menu page; the HUD highlights GetMenuSelection(). */
	TArray<FString> GetMenuLines() const;

	// ---- Input actions (bound by ATransoxianaCharacter) ----
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MoveForward;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MoveRight;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_LookX;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_LookY;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_PadLookX;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_PadLookY;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Jump;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Sprint;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Crouch;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Dodge;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Attack;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Alt;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Interact;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_SwitchWeapon;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Pause;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MenuUp;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MenuDown;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MenuLeft;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MenuRight;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MenuConfirm;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MenuBack;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void OnPausePressed();
	void OnMenuUp();
	void OnMenuDown();
	void OnMenuLeft();
	void OnMenuRight();
	void OnMenuConfirm();
	void OnMenuBack();

	void OpenMenu(ETransoxianaMenuPage Page);
	void CloseMenu();
	void ChangeSetting(int32 Direction);
	void ApplyAudioSettings();

	UPROPERTY(VisibleAnywhere, Category = "Audio")
	TObjectPtr<UWindSynthComponent> AmbientWind;

private:
	UInputAction* MakeAction(const TCHAR* Name, bool bAxis, bool bWorksWhenPaused);
	void Map(UInputAction* Action, const FKey& Key, bool bNegate = false, bool bDeadZone = false);
	bool bMappingContextAdded = false;
	void AddMappingContext();

	ETransoxianaMenuPage MenuPage = ETransoxianaMenuPage::None;
	int32 MenuSelection = 0;
};
