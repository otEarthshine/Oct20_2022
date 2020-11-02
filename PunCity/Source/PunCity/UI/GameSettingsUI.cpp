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

static const TArray<FString> GraphicsOptionsWithNone =
{
	"None",
	"Low",
	"Medium",
	"High",
};

static int32 GetGraphicsOptionIndex(FString optionIn, bool useOptionsWithNone = false)
{
	const TArray<FString>& options = useOptionsWithNone ? GraphicsOptionsWithNone : GraphicsOptions;
	for (int32 i = 0; i < options.Num(); i++) {
		if (options[i] == optionIn) {
			return i;
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
		ApplyChanges(); // Save to file
		
		gameInstance()->RestoreDefaultsAll();
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

	ConfirmChangesButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickedConfirmChangesButton);
	DiscardChangesButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickedDiscardChangesButton);

	SettingsApplyButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickSettingsApply);
	SettingsConfirmButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickSettingsConfirm);
	BackButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickSettingsCancel);
	
	/*
	 * 
	 */

	MasterSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnMasterVolumeChanged);
	MusicSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnMusicVolumeChanged);
	SoundEffectsSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnSoundEffectsVolumeChanged);
	AmbientSoundsSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnAmbientSoundsVolumeChanged);

	GraphicsSettingsButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickGraphicsSettings);
	AudioSettingsButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickAudioSettings);
	InputSettingsButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickInputSettings);
	OtherSettingsButton->OnClicked.AddDynamic(this, &UGameSettingsUI::OnClickOtherSettings);

	MouseWheelSpeedSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnMouseWheelSpeedChanged);
	MouseDragRotateSpeedSlider->OnValueChanged.AddDynamic(this, &UGameSettingsUI::OnMouseDragRotateSpeedChanged);

	RestoreDefaultsButton->OnClicked.AddDynamic(this, &UGameSettingsUI::RestoreDefault);


	/*
	 * Video settings
	 */

	ResolutionDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnResolutionDropdownChanged);
	WindowModeDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnWindowModeDropdownChanged);
	
	UIScalingDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnUIScalingDropdownChanged);
	UIScalingDropdown->SetVisibility(ESlateVisibility::Collapsed);
	
	SetupResolutionDropdown();

	WindowModeDropdown->ClearOptions();
	WindowModeDropdown->AddOption("Fullscreen (Borderless Windowed)");
	WindowModeDropdown->AddOption("Windowed");

	
	UIScalingDropdown->ClearOptions();
	UIScalingDropdown->AddOption(FString("200%"));
	UIScalingDropdown->AddOption(FString("150%"));
	UIScalingDropdown->AddOption(FString("100%"));

	/*
	 * Non video settings
	 */

	auto setupDropdown = [&](UComboBoxString* comboBox, bool hasNone = false) {
		comboBox->ClearOptions();
		const TArray<FString>& options = hasNone ? GraphicsOptionsWithNone : GraphicsOptions;
		for (int32 i = 0; i < options.Num(); i++) {
			comboBox->AddOption(options[i]);
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


	AutosaveDropdown->OnSelectionChanged.AddDynamic(this, &UGameSettingsUI::OnAutosaveDropdownChanged);
	AutosaveDropdown->ClearOptions();
	for (size_t i = 0; i < AutosaveOptions.size(); i++) {
		AutosaveDropdown->AddOption(AutosaveOptions[i]);
	}

	ResetTabSelection();
}

void UGameSettingsUI::RefreshUI(bool resetTabs)
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
	//WindowModeDropdown->SetSelectedIndex(static_cast<int32>(GEngine->GameViewport->Viewport->GetWindowMode()) - 1); // TODO: +1 for avoiding fullscreen which crashes
	if (resetTabs) { // Reset tabs means this is when Settings UI gets opened
		WindowModeDropdown->SetSelectedIndex(GEngine->GameViewport->Viewport->GetWindowMode() == EWindowMode::WindowedFullscreen ? 0 : 1);
	} else {
		WindowModeDropdown->SetSelectedIndex(settings->GetFullscreenMode() == EWindowMode::WindowedFullscreen ? 0 : 1);
	}
	RefreshResolutionDropdown();
	
	UIScalingDropdown->SetSelectedIndex(0);

	//ResolutionSlider->SetValue(settings->ScalabilityQuality.ResolutionQuality / 100.0f);
	ResolutionSlider->SetValue(gameInst->resolutionQuality() / 100.0f);
	SetText(ResolutionNumber, std::to_string(gameInst->GetDisplayResolutionQuality()) + "%");
	
	AntiAliasingDropdown->SetSelectedIndex(settings->ScalabilityQuality.AntiAliasingQuality);
	//PostProcessingDropdown->SetSelectedIndex(settings->ScalabilityQuality.PostProcessQuality);
	ShadowsDropdown->SetSelectedIndex(settings->ScalabilityQuality.ShadowQuality);
	TexturesDropdown->SetSelectedIndex(settings->ScalabilityQuality.TextureQuality);
	EffectsDropdown->SetSelectedIndex(settings->ScalabilityQuality.EffectsQuality);

	//if (settings->GetFrameRateLimit() <= 30.01) {
	//	MaxFrameRateDropdown->SetSelectedOption("30 FPS");
	//}
	//else 
	if (settings->GetFrameRateLimit() <= 60.01) {
		MaxFrameRateDropdown->SetSelectedOption("60 fps");
	}
	else if (settings->GetFrameRateLimit() <= 90.01) {
		MaxFrameRateDropdown->SetSelectedOption("90 fps");
	}
	else if (settings->GetFrameRateLimit() <= 120.01) {
		MaxFrameRateDropdown->SetSelectedOption("120 fps");
	}
	else if (settings->GetFrameRateLimit() <= 144.01) {
		MaxFrameRateDropdown->SetSelectedOption("144 fps");
	}
	else if (settings->GetFrameRateLimit() <= 240.01) {
		MaxFrameRateDropdown->SetSelectedOption("240 fps");
	}
	else {
		MaxFrameRateDropdown->SetSelectedOption("None");
	}
	VSyncCheckBox->SetIsChecked(settings->IsVSyncEnabled());
	
	// Sound
	auto setSlider = [&](USlider* slider, UTextBlock* textBlock, float number) {
		slider->SetValue(number);
		SetText(textBlock, to_string(FMath::RoundToInt(number * 100)));
	};
	
	setSlider(MasterSlider, MasterNumber, gameInst->masterVolume());
	setSlider(MusicSlider, MusicNumber, gameInst->musicVolume());
	setSlider(SoundEffectsSlider, SoundEffectsNumber, gameInst->soundEffectsVolume());
	setSlider(AmbientSoundsSlider, AmbientSoundsNumber, gameInst->ambientVolume());

	
	// Inputs
	setSlider(MouseWheelSpeedSlider, MouseWheelSpeedNumber, gameInst->mouseZoomSpeedFraction);
	setSlider(MouseDragRotateSpeedSlider, MouseDragRotateSpeedNumber, gameInst->mouseRotateSpeedFraction);


	if (resetTabs) {
		ResetTabSelection();
		SetButtonHighlight(GraphicsSettingsButton, true);
		SettingsMenu->SetActiveWidgetIndex(0);
	}

	AutosaveDropdown->SetSelectedIndex(static_cast<int32>(gameInst->autosaveEnum));

	_isSettingsDirty = false;
	ConfirmOverlay->SetVisibility(ESlateVisibility::Collapsed);
}

void UGameSettingsUI::ResetTabSelection()
{
	SetButtonHighlight(GraphicsSettingsButton, false);
	SetButtonHighlight(AudioSettingsButton, false);
	SetButtonHighlight(InputSettingsButton, false);
	SetButtonHighlight(OtherSettingsButton, false);
}

void UGameSettingsUI::ChangeTab(int32 tabIndex)
{
	ResetTabSelection();
	switch (tabIndex)
	{
	case 0: SetButtonHighlight(GraphicsSettingsButton, true); break;
	case 1: SetButtonHighlight(AudioSettingsButton, true); break;
	case 2: SetButtonHighlight(InputSettingsButton, true); break;
	case 3: SetButtonHighlight(OtherSettingsButton, true); break;
	default: break;
	}
	
	SettingsMenu->SetActiveWidgetIndex(tabIndex);

	Spawn2DSound("UI", "ButtonClick");
}

/*
 * Graphics
 */

void UGameSettingsUI::OnResolutionDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct) {
		_isSettingsDirty = true;
		_videoModeChanged = true;

		int32 optionIndex = ResolutionDropdown->FindOptionIndex(sItem);
		if (optionIndex != -1)
		{
			FIntPoint resolution = resolutions[optionIndex];
			GetGameUserSettings()->SetScreenResolution(resolution);

			Spawn2DSound("UI", "DropdownChange");
		}
	}
}
void UGameSettingsUI::OnUIScalingDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct) {
		_isSettingsDirty = true;
		//GetGameUserSettings()->();

		Spawn2DSound("UI", "DropdownChange");
	}
}
void UGameSettingsUI::OnWindowModeDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct) {
		_isSettingsDirty = true;
		_videoModeChanged = true;

		EWindowMode::Type windowMode = (sItem == "Fullscreen") ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed;
		//EWindowMode::Type windowMode = EWindowMode::ConvertIntToWindowMode(optionIndex);
		GetGameUserSettings()->SetFullscreenMode(windowMode);
		RefreshResolutionDropdown();

		Spawn2DSound("UI", "DropdownChange");
	}
}


void UGameSettingsUI::OnResolutionChanged(float value)
{
	_isSettingsDirty = true;
	//GetGameUserSettings()->SetResolutionScaleValueEx(volume * 100.0f);
	gameInstance()->SetResolutionQuality(value * 100);
	SetText(ResolutionNumber, std::to_string(gameInstance()->GetDisplayResolutionQuality()) + "%");
}
void UGameSettingsUI::OnAntiAliasingDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct) {
		_isSettingsDirty = true;
		
		GetGameUserSettings()->SetAntiAliasingQuality(GetGraphicsOptionIndex(sItem));
		Spawn2DSound("UI", "DropdownChange");
	}
}
void UGameSettingsUI::OnPostProcessingDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct) {
		_isSettingsDirty = true;
		
		GetGameUserSettings()->SetPostProcessingQuality(GetGraphicsOptionIndex(sItem));
		Spawn2DSound("UI", "DropdownChange");
	}
}
void UGameSettingsUI::OnShadowsDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct) {
		_isSettingsDirty = true;
		
		//GetGameUserSettings()->SetShadowQuality(GetGraphicsOptionIndex(sItem) + 1); // Low shadow quality is unacceptable...
		GetGameUserSettings()->SetShadowQuality(GetGraphicsOptionIndex(sItem, true));
		Spawn2DSound("UI", "DropdownChange");
	}
}
void UGameSettingsUI::OnTexturesDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct) {
		_isSettingsDirty = true;
		
		GetGameUserSettings()->SetTextureQuality(GetGraphicsOptionIndex(sItem));
		Spawn2DSound("UI", "DropdownChange");
	}
}
void UGameSettingsUI::OnEffectsDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct) {
		_isSettingsDirty = true;
		
		GetGameUserSettings()->SetVisualEffectQuality(GetGraphicsOptionIndex(sItem));
		Spawn2DSound("UI", "DropdownChange");
	}
}

void UGameSettingsUI::OnMaxFrameRateDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct) {
		_isSettingsDirty = true;
		_videoModeChanged = true;

		if (sItem == FString("None")) {
			GetGameUserSettings()->SetFrameRateLimit(600); // unlimited fps
		}
		//else if (sItem == FString("30 FPS")) {
		//	GetGameUserSettings()->SetFrameRateLimit(30);
		//}
		else if (sItem == FString("60 fps")) {
			GetGameUserSettings()->SetFrameRateLimit(60);
		}
		else if (sItem == FString("90 fps")) {
			GetGameUserSettings()->SetFrameRateLimit(90);
		}
		else if (sItem == FString("120 fps")) {
			GetGameUserSettings()->SetFrameRateLimit(120);
		}
		else if (sItem == FString("144 fps")) {
			GetGameUserSettings()->SetFrameRateLimit(144);
		}
		else if (sItem == FString("240 fps")) {
			GetGameUserSettings()->SetFrameRateLimit(240);
		}
	}
}
void UGameSettingsUI::OnVSyncCheckBoxChecked(bool active)
{
	_isSettingsDirty = true;
	_videoModeChanged = true;
	
	GetGameUserSettings()->SetVSyncEnabled(active);

	Spawn2DSound("UI", "DropdownChange");

	PUN_LOG("Checked settings->IsVSyncDirty(): %d", GetGameUserSettings()->IsVSyncDirty());
}

void UGameSettingsUI::OnAutosaveDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype != ESelectInfo::Type::Direct) {
		_isSettingsDirty = true;

		for (size_t i = 0; i < AutosaveOptions.size(); i++) {
			if (AutosaveOptions[i] == sItem) {
				gameInstance()->autosaveEnum = static_cast<AutosaveEnum>(i);
				break;
			}
		}

		Spawn2DSound("UI", "DropdownChange");
	}
}



void UGameSettingsUI::RestoreDefault()
{
	_isSettingsDirty = true;
	
	if (SettingsMenu->GetActiveWidgetIndex() == 0)
	{
		auto settings = GetGameUserSettings();
		settings->SetScreenResolution(FIntPoint(1920, 1080));
		settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
		RefreshResolutionDropdown();

		// Set to fullscreen
		settings->ConfirmVideoMode();
		settings->ApplyResolutionSettings(false);

		settings->SetAntiAliasingQuality(2);
		settings->SetPostProcessingQuality(2);
		settings->SetShadowQuality(3);
		settings->SetTextureQuality(0);
		settings->SetVisualEffectQuality(0);

		settings->SetVSyncEnabled(true);

		gameInstance()->RestoreDefaultsGraphics();

		Spawn2DSound("UI", "ButtonClick");

		RefreshUI(false);
	}
	else if (SettingsMenu->GetActiveWidgetIndex() == 1)
	{
		gameInstance()->RestoreDefaultsSound();
		RefreshUI(false);
	}
	else if (SettingsMenu->GetActiveWidgetIndex() == 2)
	{
		gameInstance()->RestoreDefaultsInputs();
		RefreshUI(false);
	}
	else if (SettingsMenu->GetActiveWidgetIndex() == 3)
	{
		gameInstance()->RestoreDefaultsOthers();
		RefreshUI(false);
	}
	else {
		PUN_NOENTRY();
	}
}

void UGameSettingsUI::ApplyChanges()
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

	settings->RequestUIUpdate();
	settings->SaveSettings();
	gameInstance()->SaveSettingsToFile();

	PunSettings::bShouldRefreshMainMenuDisplay = true;

	RefreshUI(false);

	PUN_LOG("settings->IsVSyncDirty(): %d", settings->IsVSyncDirty());
}
void UGameSettingsUI::UndoChanges()
{
	auto settings = GetGameUserSettings();

	if (_videoModeChanged) {
		_videoModeChanged = false;
		settings->RevertVideoMode();
	}

	settings->LoadSettings(true);
	gameInstance()->LoadSoundAndOtherSettingsFromFile();
	gameInstance()->RefreshSoundSettings();

	RefreshUI(false);
}

void UGameSettingsUI::ExecuteAfterConfirmOrDiscard()
{
	if (_tabIndexToChangeTo == -1) {
		_callbackParent->CallBack2(_callbackParent, CallbackEnum::CloseGameSettingsUI);
	}
	else {
		ChangeTab(_tabIndexToChangeTo);
	}
	RefreshUI(false);
}


void UGameSettingsUI::RefreshResolutionDropdown()
{
	auto settings = GetGameUserSettings();
	
	// Disable the resolution combobox if it is in windwed fullscreen mode
	if (settings->GetFullscreenMode() == EWindowMode::Windowed) {
		SetupResolutionDropdown();
		ResolutionDropdown->SetIsEnabled(true);
		ResolutionDropdown->SetVisibility(ESlateVisibility::Visible);
		ResolutionHiddenText->SetVisibility(ESlateVisibility::Hidden);
	}
	else {
		ResolutionDropdown->SetIsEnabled(false);
		ResolutionDropdown->SetVisibility(ESlateVisibility::Hidden);
		ResolutionHiddenText->SetVisibility(ESlateVisibility::Visible);

		FIntPoint point = settings->GetDesktopResolution();
		//ResolutionHiddenText->SetText(FText::FromString(FString::FromInt(point.X) + "x" + FString::FromInt(point.Y)));
		//ResolutionHiddenText->SetText(FText::FromString(FString::FromInt(point.X) + "x" + FString::FromInt(point.Y) + "(Change to windowed "));
		ResolutionHiddenText->SetText(FText::FromString(FString("To adjust, change to windowed mode")));
	}
}
