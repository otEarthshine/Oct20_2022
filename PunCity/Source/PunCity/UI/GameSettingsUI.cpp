// Pun Dumnernchanvanit's


#include "GameSettingsUI.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/UserInterfaceSettings.h"

#include "GenericPlatform/GenericPlatformSurvey.h"
#include "SynthBenchmark.h"

static const TArray<FString> GraphicsOptions =
{
	"Low",
	"Medium",
	"High",
	"Ultra",
};

static int32 GetGraphicsOptionIndex(FString optionIn, bool isShiftedUp = false)
{
	for (int32 i = 0; i < GraphicsOptions.Num(); i++) {
		if (GraphicsOptions[i] == optionIn) {
			return isShiftedUp? (i + 1) : i;
		}
	}
	UE_DEBUG_BREAK();
	return -1;
}

void UGameSettingsUI::PunInit(UPunWidget* callbackParent)
{
	_callbackParent = callbackParent;

	bool hasExistingSettings = gameInstance()->LoadSoundAndOtherSettingsFromFile();

	if (!hasExistingSettings ||
		gameInstance()->loadedVersion != GAME_VERSION)
	{
		RestoreDefault();
		gameInstance()->RestoreDefaults();
		gameInstance()->SaveSettingsToFile();
	}

	
	{
		// Get Settings from benchmark
		
		Scalability::FQualityLevels qualityLevels = Scalability::BenchmarkQualityLevels();
		
		const float CPUPerfIndex = qualityLevels.CPUBenchmarkResults;
		const float GPUPerfIndex = qualityLevels.GPUBenchmarkResults;

		PUN_LOG("Scalability CPUPerfIndex:%f GPUPerfIndex:%f", CPUPerfIndex, GPUPerfIndex);

		// Oh's Laptop CPU 166.5, 110.5 ... Savanna  
		// Pun's Desktop CPU 161, GPU 250 .... 

		auto computeQualityIndex = [&](const FString& GroupName, float CPUPerfIndex, float GPUPerfIndex)
		{
			//// Some code defaults in case the ini file can not be read or has dirty data
			//float PerfIndex = FMath::Min(CPUPerfIndex, GPUPerfIndex);

			//TArray<float, TInlineAllocator<4>> Thresholds;
			//Thresholds.Add(20.0f);
			//Thresholds.Add(50.0f);
			//Thresholds.Add(70.0f);

		};

		// decide on the actual quality needed
		//qualityLevels.ResolutionQuality = 
		//qualityLevels.AntiAliasingQuality = ComputeOptionFromPerfIndex(TEXT("AntiAliasingQuality"), CPUPerfIndex, GPUPerfIndex);
		//qualityLevels.PostProcessQuality = ComputeOptionFromPerfIndex(TEXT("PostProcessQuality"), CPUPerfIndex, GPUPerfIndex);
		//qualityLevels.ShadowQuality = ComputeOptionFromPerfIndex(TEXT("ShadowQuality"), CPUPerfIndex, GPUPerfIndex);
		//qualityLevels.TextureQuality = ComputeOptionFromPerfIndex(TEXT("TextureQuality"), CPUPerfIndex, GPUPerfIndex);
		//qualityLevels.EffectsQuality = ComputeOptionFromPerfIndex(TEXT("EffectsQuality"), CPUPerfIndex, GPUPerfIndex);

		

		//auto settings = GetGameUserSettings();
		//
		//settings->SetAntiAliasingQuality(qualityLevels.AntiAliasingQuality);
		//AntiAliasingDropdown->SetSelectedIndex(qualityLevels.AntiAliasingQuality);

		//settings->SetPostProcessingQuality(qualityLevels.PostProcessQuality);
		//PostProcessingDropdown->SetSelectedIndex(qualityLevels.PostProcessQuality);

		//// 0 shadow is way too ugly
		//int32 shadowQuality = std::max(1, qualityLevels.ShadowQuality);
		//settings->SetShadowQuality(shadowQuality);
		//ShadowsDropdown->SetSelectedIndex(shadowQuality);

		//settings->SetTextureQuality(qualityLevels.TextureQuality);
		//TexturesDropdown->SetSelectedIndex(qualityLevels.TextureQuality);

		//settings->SetVisualEffectQuality(qualityLevels.EffectsQuality);
		//EffectsDropdown->SetSelectedIndex(qualityLevels.EffectsQuality);

		//settings->ApplyNonResolutionSettings();
		//settings->RequestUIUpdate();
		//settings->SaveSettings();

		//gameInstance()->SaveSettingsToFile(); // Save settings to file so next time the game doesn't run benchmark calculation again.
	}

	/*
	 * Special cases
	 */
	//GetGameUserSettings()->SetPostProcessingQuality(2); // 4.25 postprocessing broken...
	
	/*
	 * 
	 */

	MasterSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnMasterVolumeChanged);
	MusicSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnMusicVolumeChanged);
	SoundEffectsSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnSoundEffectsVolumeChanged);
	AmbientSoundsSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnAmbientSoundsVolumeChanged);


	SettingsApplyButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickSettingsApply);
	SettingsConfirmButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickSettingsConfirm);
	SettingsCancelButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickSettingsCancel);
	BackButton->OnClicked.AddDynamic(this , &UGameSettingsUI::OnClickSettingsCancel);

	GraphicsSettingsButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickGraphicsSettings);
	AudioSettingsButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickAudioSettings);
	InputSettingsButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickInputSettings);

	MouseWheelSpeedSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnMouseWheelSpeedChanged);

	RestoreDefaultsButton->OnClicked.AddDynamic(this, &UGameSettingsUI::RestoreDefault);


	/*
	 * Video settings
	 */

	ResolutionDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnResolutionDropdownChanged);
	WindowModeDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnWindowModeDropdownChanged);
	
	UIScalingDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnUIScalingDropdownChanged);
	UIScalingDropdown->SetVisibility(ESlateVisibility::Collapsed);
	
	ResolutionDropdown->ClearOptions();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(resolutions);
	for (FIntPoint point : resolutions) {
		ResolutionDropdown->AddOption(FString::FromInt(point.X) + "x" + FString::FromInt(point.Y));
	}

	WindowModeDropdown->ClearOptions();
	WindowModeDropdown->AddOption("Fullscreen"); // TODO: Fullscreen causing crash...
	WindowModeDropdown->AddOption("WindowedFullscreen");
	WindowModeDropdown->AddOption("Windowed");

	
	UIScalingDropdown->ClearOptions();
	UIScalingDropdown->AddOption(FString("200%"));
	UIScalingDropdown->AddOption(FString("150%"));
	UIScalingDropdown->AddOption(FString("100%"));

	/*
	 * Non video settings
	 */

	auto setupDropdown = [&](UComboBoxString* comboBox, int32 isShiftedDown = false) {
		comboBox->ClearOptions();
		int32 length = GraphicsOptions.Num() + (isShiftedDown ? -1 : 0); // Don't display ultra if isShiftedDown is true
		for (int32 i = 0; i < length; i++) {
			comboBox->AddOption(GraphicsOptions[i]);
		}
	};

	ResolutionSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnResolutionChanged);

	AntiAliasingDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnAntiAliasingDropdownChanged);
	//PostProcessingDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnPostProcessingDropdownChanged);
	ShadowsDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnShadowsDropdownChanged);
	TexturesDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnTexturesDropdownChanged);
	EffectsDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnEffectsDropdownChanged);

	MaxFrameRateDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnMaxFrameRateDropdownChanged);
	VSyncCheckBox->OnCheckStateChanged.AddDynamic(this, &UGameSettingsUI::OnVSyncCheckBoxChecked);

	setupDropdown(AntiAliasingDropdown);
	//setupDropdown(PostProcessingDropdown);
	setupDropdown(ShadowsDropdown, true);
	setupDropdown(TexturesDropdown);
	setupDropdown(EffectsDropdown);

	ResetTabSelection();
}

void UGameSettingsUI::Refresh()
{
	// Refresh does SetSelectedIndex which makes sound, suppress it...
	_lastOpened = UGameplayStatics::GetTimeSeconds(this);
	
	UGameUserSettings* settings = GetGameUserSettings();

	auto gameInst = gameInstance();

	FIntPoint currentResolution = settings->GetScreenResolution();
	int32 resolutionIndex = -1;
	for (int32 i = 0; i < resolutions.Num(); i++) {
		if (resolutions[i] == currentResolution) {
			resolutionIndex = i;
			break;
		}
	}
	if (resolutionIndex == -1) {
		resolutions.Add(currentResolution);
		ResolutionDropdown->AddOption(FString::FromInt(currentResolution.X) + "x" + FString::FromInt(currentResolution.Y));
		resolutionIndex = resolutions.Num() - 1;
	}
	
	ResolutionDropdown->SetSelectedIndex(resolutionIndex);
	WindowModeDropdown->SetSelectedIndex(static_cast<int32>(GEngine->GameViewport->Viewport->GetWindowMode()) - 1); // TODO: +1 for avoiding fullscreen which crashes
	UIScalingDropdown->SetSelectedIndex(0);

	//ResolutionSlider->SetValue(settings->ScalabilityQuality.ResolutionQuality / 100.0f);
	ResolutionSlider->SetValue(gameInst->resolutionQuality() / 100.0f);
	
	AntiAliasingDropdown->SetSelectedIndex(settings->ScalabilityQuality.AntiAliasingQuality);
	//PostProcessingDropdown->SetSelectedIndex(settings->ScalabilityQuality.PostProcessQuality);
	ShadowsDropdown->SetSelectedIndex(settings->ScalabilityQuality.ShadowQuality - 1); // Shadow is shifted by 1
	TexturesDropdown->SetSelectedIndex(settings->ScalabilityQuality.TextureQuality);
	EffectsDropdown->SetSelectedIndex(settings->ScalabilityQuality.EffectsQuality);

	if (settings->GetFrameRateLimit() <= 30.01) {
		MaxFrameRateDropdown->SetSelectedIndex(30);
	}
	else if (settings->GetFrameRateLimit() <= 60.01) {
		MaxFrameRateDropdown->SetSelectedIndex(60);
	}
	else if (settings->GetFrameRateLimit() <= 90.01) {
		MaxFrameRateDropdown->SetSelectedIndex(90);
	}
	else if (settings->GetFrameRateLimit() <= 120.01) {
		MaxFrameRateDropdown->SetSelectedIndex(120);
	}
	else {
		MaxFrameRateDropdown->SetSelectedIndex(0);
	}
	VSyncCheckBox->SetIsChecked(settings->IsVSyncEnabled());
	
	
	MasterSlider->SetValue(gameInst->masterVolume());
	MusicSlider->SetValue(gameInst->musicVolume());
	SoundEffectsSlider->SetValue(gameInst->soundEffectsVolume());
	AmbientSoundsSlider->SetValue(gameInst->ambientVolume());
	
	MouseWheelSpeedSlider->SetValue(gameInst->mouseZoomSpeedFraction);

	ResetTabSelection();
	SetButtonHighlight(GraphicsSettingsButton, true);
	SettingsMenu->SetActiveWidgetIndex(0);
}

void UGameSettingsUI::ResetTabSelection()
{
	SetButtonHighlight(GraphicsSettingsButton, false);
	SetButtonHighlight(AudioSettingsButton, false);
	SetButtonHighlight(InputSettingsButton, false);
}

void UGameSettingsUI::OnClickGraphicsSettings()
{
	ResetTabSelection();
	SetButtonHighlight(GraphicsSettingsButton, true);
	SettingsMenu->SetActiveWidgetIndex(0);

	Spawn2DSound("UI", "ButtonClick");
}
void UGameSettingsUI::OnClickAudioSettings()
{
	ResetTabSelection();
	SetButtonHighlight(AudioSettingsButton, true);
	SettingsMenu->SetActiveWidgetIndex(1);

	Spawn2DSound("UI", "ButtonClick");
}
void UGameSettingsUI::OnClickInputSettings()
{
	ResetTabSelection();
	SetButtonHighlight(InputSettingsButton, true);
	SettingsMenu->SetActiveWidgetIndex(2);

	Spawn2DSound("UI", "ButtonClick");
}

void UGameSettingsUI::OnResolutionDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	_videoModeChanged = true;

	int32 optionIndex = ResolutionDropdown->FindOptionIndex(sItem);
	if (optionIndex != -1)
	{
		FIntPoint resolution = resolutions[optionIndex];
		GetGameUserSettings()->SetScreenResolution(resolution);

		Spawn2DSound("UI", "DropdownChange");
	}
}
void UGameSettingsUI::OnUIScalingDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	//GetGameUserSettings()->();

	Spawn2DSound("UI", "DropdownChange");
}
void UGameSettingsUI::OnWindowModeDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	_videoModeChanged = true;

	int32 optionIndex = WindowModeDropdown->FindOptionIndex(sItem) + 1; // TODO: +1 for avoiding fullscreen which crashes
	EWindowMode::Type windowMode = EWindowMode::ConvertIntToWindowMode(optionIndex);
	GetGameUserSettings()->SetFullscreenMode(windowMode);

	Spawn2DSound("UI", "DropdownChange");
}


void UGameSettingsUI::OnResolutionChanged(float value)
{
	//GetGameUserSettings()->SetResolutionScaleValueEx(volume * 100.0f);
	gameInstance()->SetResolutionQuality(value * 65 + 35);
}
void UGameSettingsUI::OnAntiAliasingDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct)
	{
		GetGameUserSettings()->SetAntiAliasingQuality(GetGraphicsOptionIndex(sItem));
		Spawn2DSound("UI", "DropdownChange");
	}
}
void UGameSettingsUI::OnPostProcessingDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct)
	{
		GetGameUserSettings()->SetPostProcessingQuality(GetGraphicsOptionIndex(sItem));
		Spawn2DSound("UI", "DropdownChange");
	}
}
void UGameSettingsUI::OnShadowsDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct)
	{
		GetGameUserSettings()->SetShadowQuality(GetGraphicsOptionIndex(sItem) + 1); // Low shadow quality is unacceptable...
		Spawn2DSound("UI", "DropdownChange");
	}
}
void UGameSettingsUI::OnTexturesDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct)
	{
		GetGameUserSettings()->SetTextureQuality(GetGraphicsOptionIndex(sItem));
		Spawn2DSound("UI", "DropdownChange");
	}
}
void UGameSettingsUI::OnEffectsDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct)
	{
		GetGameUserSettings()->SetVisualEffectQuality(GetGraphicsOptionIndex(sItem));
		Spawn2DSound("UI", "DropdownChange");
	}
}

void UGameSettingsUI::OnMaxFrameRateDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct)
	{
		_videoModeChanged = true;

		if (sItem == FString("None")) {
			GetGameUserSettings()->SetFrameRateLimit(600); // unlimited fps
		}
		else if (sItem == FString("30 FPS")) {
			GetGameUserSettings()->SetFrameRateLimit(30);
		}
		else if (sItem == FString("60 FPS")) {
			GetGameUserSettings()->SetFrameRateLimit(60);
		}
		else if (sItem == FString("90 FPS")) {
			GetGameUserSettings()->SetFrameRateLimit(90);
		}
		else if (sItem == FString("120 FPS")) {
			GetGameUserSettings()->SetFrameRateLimit(120);
		}
	}
}
void UGameSettingsUI::OnVSyncCheckBoxChecked(bool active)
{
	_videoModeChanged = true;
	
	GetGameUserSettings()->SetVSyncEnabled(active);

	Spawn2DSound("UI", "DropdownChange");

	PUN_LOG("Checked settings->IsVSyncDirty(): %d", GetGameUserSettings()->IsVSyncDirty());
}


void UGameSettingsUI::RestoreDefault()
{
	auto settings = GetGameUserSettings();
	settings->SetScreenResolution(settings->GetDefaultResolution());
	settings->SetFullscreenMode(settings->GetDefaultWindowMode());
	
	settings->SetAntiAliasingQuality(2);
	settings->SetPostProcessingQuality(2);
	settings->SetShadowQuality(3);
	settings->SetTextureQuality(2);
	settings->SetVisualEffectQuality(2);

	gameInstance()->RestoreDefaults();

	Spawn2DSound("UI", "ButtonClick");

	Refresh();
}


void UGameSettingsUI::OnClickSettingsApply()
{
	auto settings = GetGameUserSettings();

	if (_videoModeChanged) {
		_videoModeChanged = false;
		settings->ConfirmVideoMode();
		//settings->SetScreenResolution(settings->GetLastConfirmedScreenResolution());
		//settings->SetFullscreenMode(settings->GetLastConfirmedFullscreenMode());
		settings->ApplyResolutionSettings(false);
	}
	settings->ApplyNonResolutionSettings();

	//if (GEngine->IsInitialized()) {
	//	Scalability::SetQualityLevels(settings->ScalabilityQuality);
	//}

	settings->RequestUIUpdate();
	settings->SaveSettings();
	gameInstance()->SaveSettingsToFile();

	PunSettings::bShouldRefreshMainMenuDisplay = true;

	Spawn2DSound("UI", "ButtonClick");

	PUN_LOG("settings->IsVSyncDirty(): %d", settings->IsVSyncDirty());
}

// !!! Note that Civ 6 only has Confirm/Back ... we just have Apply/Close??
void UGameSettingsUI::OnClickSettingsConfirm()
{
	auto settings = GetGameUserSettings();

	OnClickSettingsApply();
	
	
	settings->RequestUIUpdate();
	settings->SaveSettings();
	gameInstance()->SaveSettingsToFile();

	Spawn2DSound("UI", "ButtonClick");
	
	_callbackParent->CallBack2(_callbackParent, CallbackEnum::CloseGameSettingsUI);
}

void UGameSettingsUI::OnClickSettingsCancel()
{
	auto settings = GetGameUserSettings();

	if (_videoModeChanged) {
		_videoModeChanged = false;
		settings->RevertVideoMode();
	}

	Spawn2DSound("UI", "UIWindowClose");
	
	settings->LoadSettings(true);
	gameInstance()->LoadSoundAndOtherSettingsFromFile();
	
	_callbackParent->CallBack2(_callbackParent, CallbackEnum::CloseGameSettingsUI);
}