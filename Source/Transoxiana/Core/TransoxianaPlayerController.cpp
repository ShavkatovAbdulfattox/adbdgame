#include "Core/TransoxianaPlayerController.h"

#include "Core/TransoxianaGameMode.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "World/WindSynthComponent.h"

namespace
{
	// Pause page
	enum EPauseItem { Pause_Resume, Pause_Settings, Pause_Restart, Pause_Quit, Pause_Count };
	// Settings page
	enum ESettingsItem { Set_Sensitivity, Set_InvertY, Set_Volume, Set_Quality, Set_Back, Set_Count };

	const TCHAR* QualityNames[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") };
}

ATransoxianaPlayerController::ATransoxianaPlayerController()
{
	AmbientWind = CreateDefaultSubobject<UWindSynthComponent>(TEXT("AmbientWind"));
	AmbientWind->SetupAttachment(RootComponent);
}

void ATransoxianaPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		EnsureInputCreated();
		AddMappingContext();
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
		ApplyAudioSettings();
		AmbientWind->Start();
	}
}

// ---------------------------------------------------------------------------
// Input creation
// ---------------------------------------------------------------------------

UInputAction* ATransoxianaPlayerController::MakeAction(const TCHAR* Name, bool bAxis, bool bWorksWhenPaused)
{
	UInputAction* Action = NewObject<UInputAction>(this, FName(Name));
	Action->ValueType = bAxis ? EInputActionValueType::Axis1D : EInputActionValueType::Boolean;
	Action->bTriggerWhenPaused = bWorksWhenPaused;
	return Action;
}

void ATransoxianaPlayerController::Map(UInputAction* Action, const FKey& Key, bool bNegate, bool bDeadZone)
{
	FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(Action, Key);
	if (bDeadZone)
	{
		Mapping.Modifiers.Add(NewObject<UInputModifierDeadZone>(MappingContext));
	}
	if (bNegate)
	{
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
	}
}

void ATransoxianaPlayerController::EnsureInputCreated()
{
	if (MappingContext)
	{
		return;
	}
	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Transoxiana"));

	IA_MoveForward  = MakeAction(TEXT("IA_MoveForward"), true, false);
	IA_MoveRight    = MakeAction(TEXT("IA_MoveRight"), true, false);
	IA_LookX        = MakeAction(TEXT("IA_LookX"), true, false);
	IA_LookY        = MakeAction(TEXT("IA_LookY"), true, false);
	IA_PadLookX     = MakeAction(TEXT("IA_PadLookX"), true, false);
	IA_PadLookY     = MakeAction(TEXT("IA_PadLookY"), true, false);
	IA_Jump         = MakeAction(TEXT("IA_Jump"), false, false);
	IA_Sprint       = MakeAction(TEXT("IA_Sprint"), false, false);
	IA_Crouch       = MakeAction(TEXT("IA_Crouch"), false, false);
	IA_Dodge        = MakeAction(TEXT("IA_Dodge"), false, false);
	IA_Attack       = MakeAction(TEXT("IA_Attack"), false, false);
	IA_Alt          = MakeAction(TEXT("IA_Alt"), false, false);
	IA_Interact     = MakeAction(TEXT("IA_Interact"), false, false);
	IA_SwitchWeapon = MakeAction(TEXT("IA_SwitchWeapon"), false, false);
	IA_Pause        = MakeAction(TEXT("IA_Pause"), false, true);
	IA_MenuUp       = MakeAction(TEXT("IA_MenuUp"), false, true);
	IA_MenuDown     = MakeAction(TEXT("IA_MenuDown"), false, true);
	IA_MenuLeft     = MakeAction(TEXT("IA_MenuLeft"), false, true);
	IA_MenuRight    = MakeAction(TEXT("IA_MenuRight"), false, true);
	IA_MenuConfirm  = MakeAction(TEXT("IA_MenuConfirm"), false, true);
	IA_MenuBack     = MakeAction(TEXT("IA_MenuBack"), false, true);

	// Keyboard + mouse
	Map(IA_MoveForward, EKeys::W);
	Map(IA_MoveForward, EKeys::S, true);
	Map(IA_MoveRight, EKeys::D);
	Map(IA_MoveRight, EKeys::A, true);
	Map(IA_LookX, EKeys::MouseX);
	Map(IA_LookY, EKeys::MouseY);
	Map(IA_Jump, EKeys::SpaceBar);
	Map(IA_Sprint, EKeys::LeftShift);
	Map(IA_Crouch, EKeys::C);
	Map(IA_Crouch, EKeys::LeftControl);
	Map(IA_Dodge, EKeys::LeftAlt);
	Map(IA_Attack, EKeys::LeftMouseButton);
	Map(IA_Alt, EKeys::RightMouseButton);
	Map(IA_Interact, EKeys::E);
	Map(IA_Interact, EKeys::F);
	Map(IA_SwitchWeapon, EKeys::Q);
	Map(IA_SwitchWeapon, EKeys::Tab);
	// Escape stops Play-In-Editor, so P also opens the pause menu.
	Map(IA_Pause, EKeys::Escape);
	Map(IA_Pause, EKeys::P);
	Map(IA_MenuUp, EKeys::Up);
	Map(IA_MenuDown, EKeys::Down);
	Map(IA_MenuLeft, EKeys::Left);
	Map(IA_MenuRight, EKeys::Right);
	Map(IA_MenuConfirm, EKeys::Enter);
	Map(IA_MenuBack, EKeys::BackSpace);

	// Gamepad
	Map(IA_MoveForward, EKeys::Gamepad_LeftY, false, true);
	Map(IA_MoveRight, EKeys::Gamepad_LeftX, false, true);
	Map(IA_PadLookX, EKeys::Gamepad_RightX, false, true);
	Map(IA_PadLookY, EKeys::Gamepad_RightY, false, true);
	Map(IA_Jump, EKeys::Gamepad_FaceButton_Bottom);
	Map(IA_Sprint, EKeys::Gamepad_LeftThumbstick);
	Map(IA_Crouch, EKeys::Gamepad_RightThumbstick);
	Map(IA_Dodge, EKeys::Gamepad_FaceButton_Right);
	Map(IA_Attack, EKeys::Gamepad_RightTrigger);
	Map(IA_Alt, EKeys::Gamepad_LeftTrigger);
	Map(IA_Interact, EKeys::Gamepad_FaceButton_Left);
	Map(IA_SwitchWeapon, EKeys::Gamepad_FaceButton_Top);
	Map(IA_Pause, EKeys::Gamepad_Special_Right);
	Map(IA_MenuUp, EKeys::Gamepad_DPad_Up);
	Map(IA_MenuDown, EKeys::Gamepad_DPad_Down);
	Map(IA_MenuLeft, EKeys::Gamepad_DPad_Left);
	Map(IA_MenuRight, EKeys::Gamepad_DPad_Right);
	Map(IA_MenuConfirm, EKeys::Gamepad_FaceButton_Bottom);
	Map(IA_MenuBack, EKeys::Gamepad_FaceButton_Right);
}

void ATransoxianaPlayerController::AddMappingContext()
{
	if (bMappingContextAdded || !MappingContext)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MappingContext, 0);
		bMappingContextAdded = true;
	}
}

void ATransoxianaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	EnsureInputCreated();
	AddMappingContext();

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		Input->BindAction(IA_Pause, ETriggerEvent::Started, this, &ThisClass::OnPausePressed);
		Input->BindAction(IA_MenuUp, ETriggerEvent::Started, this, &ThisClass::OnMenuUp);
		Input->BindAction(IA_MenuDown, ETriggerEvent::Started, this, &ThisClass::OnMenuDown);
		Input->BindAction(IA_MenuLeft, ETriggerEvent::Started, this, &ThisClass::OnMenuLeft);
		Input->BindAction(IA_MenuRight, ETriggerEvent::Started, this, &ThisClass::OnMenuRight);
		Input->BindAction(IA_MenuConfirm, ETriggerEvent::Started, this, &ThisClass::OnMenuConfirm);
		Input->BindAction(IA_MenuBack, ETriggerEvent::Started, this, &ThisClass::OnMenuBack);
	}
}

// ---------------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------------

TArray<FString> ATransoxianaPlayerController::GetMenuLines() const
{
	TArray<FString> Lines;
	if (MenuPage == ETransoxianaMenuPage::Pause)
	{
		Lines = { TEXT("Resume"), TEXT("Settings"), TEXT("Restart mission"), TEXT("Quit game") };
	}
	else if (MenuPage == ETransoxianaMenuPage::Settings)
	{
		const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
		const int32 Quality = Settings ? FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, 3) : 2;
		Lines.Add(FString::Printf(TEXT("Mouse sensitivity   < %.1f >"), MouseSensitivity));
		Lines.Add(FString::Printf(TEXT("Invert look Y       < %s >"), bInvertY ? TEXT("On") : TEXT("Off")));
		Lines.Add(FString::Printf(TEXT("Ambient volume      < %d%% >"), FMath::RoundToInt(AmbientVolume * 100.f)));
		Lines.Add(FString::Printf(TEXT("Graphics quality    < %s >"), QualityNames[Quality]));
		Lines.Add(TEXT("Back"));
	}
	return Lines;
}

void ATransoxianaPlayerController::OpenMenu(ETransoxianaMenuPage Page)
{
	MenuPage = Page;
	MenuSelection = 0;
	SetPause(true);
}

void ATransoxianaPlayerController::CloseMenu()
{
	MenuPage = ETransoxianaMenuPage::None;
	SaveConfig();
	SetPause(false);
}

void ATransoxianaPlayerController::OnPausePressed()
{
	const ATransoxianaGameMode* GM = GetWorld()->GetAuthGameMode<ATransoxianaGameMode>();
	if (GM && GM->IsMissionOver())
	{
		return;
	}
	if (MenuPage == ETransoxianaMenuPage::None)
	{
		OpenMenu(ETransoxianaMenuPage::Pause);
	}
	else
	{
		CloseMenu();
	}
}

void ATransoxianaPlayerController::OnMenuUp()
{
	const int32 Count = GetMenuLines().Num();
	if (Count > 0)
	{
		MenuSelection = (MenuSelection + Count - 1) % Count;
	}
}

void ATransoxianaPlayerController::OnMenuDown()
{
	const int32 Count = GetMenuLines().Num();
	if (Count > 0)
	{
		MenuSelection = (MenuSelection + 1) % Count;
	}
}

void ATransoxianaPlayerController::OnMenuLeft()
{
	if (MenuPage == ETransoxianaMenuPage::Settings)
	{
		ChangeSetting(-1);
	}
}

void ATransoxianaPlayerController::OnMenuRight()
{
	if (MenuPage == ETransoxianaMenuPage::Settings)
	{
		ChangeSetting(+1);
	}
}

void ATransoxianaPlayerController::OnMenuConfirm()
{
	ATransoxianaGameMode* GM = GetWorld()->GetAuthGameMode<ATransoxianaGameMode>();

	// On the mission end screen, Enter / A replays the mission.
	if (GM && GM->IsMissionOver() && MenuPage == ETransoxianaMenuPage::None)
	{
		GM->RestartMission();
		return;
	}

	if (MenuPage == ETransoxianaMenuPage::Pause)
	{
		switch (MenuSelection)
		{
		case Pause_Resume:   CloseMenu(); break;
		case Pause_Settings: MenuPage = ETransoxianaMenuPage::Settings; MenuSelection = 0; break;
		case Pause_Restart:  SaveConfig(); if (GM) { GM->RestartMission(); } break;
		case Pause_Quit:     SaveConfig(); UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false); break;
		default: break;
		}
	}
	else if (MenuPage == ETransoxianaMenuPage::Settings)
	{
		if (MenuSelection == Set_Back)
		{
			OnMenuBack();
		}
		else
		{
			ChangeSetting(+1);
		}
	}
}

void ATransoxianaPlayerController::OnMenuBack()
{
	if (MenuPage == ETransoxianaMenuPage::Settings)
	{
		SaveConfig();
		MenuPage = ETransoxianaMenuPage::Pause;
		MenuSelection = Pause_Settings;
	}
	else if (MenuPage == ETransoxianaMenuPage::Pause)
	{
		CloseMenu();
	}
}

void ATransoxianaPlayerController::ChangeSetting(int32 Direction)
{
	switch (MenuSelection)
	{
	case Set_Sensitivity:
		MouseSensitivity = FMath::Clamp(MouseSensitivity + 0.1f * Direction, 0.2f, 3.0f);
		break;
	case Set_InvertY:
		bInvertY = !bInvertY;
		break;
	case Set_Volume:
		AmbientVolume = FMath::Clamp(AmbientVolume + 0.1f * Direction, 0.f, 1.f);
		ApplyAudioSettings();
		break;
	case Set_Quality:
		if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
		{
			const int32 Current = FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, 3);
			Settings->SetOverallScalabilityLevel(FMath::Clamp(Current + Direction, 0, 3));
			Settings->ApplySettings(false);
			Settings->SaveSettings();
		}
		break;
	default:
		break;
	}
}

void ATransoxianaPlayerController::ApplyAudioSettings()
{
	if (AmbientWind)
	{
		AmbientWind->SetVolumeMultiplier(AmbientVolume);
	}
}
