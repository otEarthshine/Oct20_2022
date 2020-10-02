// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "Components/WidgetSwitcher.h"


#include "GameSettingsUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UGameSettingsUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void PunInit(UPunWidget* callbackParent);

	void RefreshUI(bool resetTabs = true);
	
	UPROPERTY(meta = (BindWidget)) UWidgetSwitcher* SettingsMenu;

	// Settings Apply
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmChangesButton;
	UPROPERTY(meta = (BindWidget)) UButton* DiscardChangesButton;
	UPROPERTY(meta = (BindWidget)) UOverlay* ConfirmOverlay;
	
	UPROPERTY(meta = (BindWidget)) UButton* BackButton;
	UPROPERTY(meta = (BindWidget)) UButton* SettingsConfirmButton;
	UPROPERTY(meta = (BindWidget)) UButton* SettingsApplyButton;

	// Graphics
	UPROPERTY(meta = (BindWidget)) UButton* GraphicsSettingsButton;

	UPROPERTY(meta = (BindWidget)) UComboBoxString* ResolutionDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* WindowModeDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* UIScalingDropdown;

	UPROPERTY(meta = (BindWidget)) USlider* ResolutionSlider;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResolutionNumber;
	
	UPROPERTY(meta = (BindWidget)) UComboBoxString* AntiAliasingDropdown;
	//UPROPERTY(meta = (BindWidget)) UComboBoxString* PostProcessingDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* ShadowsDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* TexturesDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* EffectsDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* MaxFrameRateDropdown;
	UPROPERTY(meta = (BindWidget)) UCheckBox* VSyncCheckBox;
	
	// Audio
	UPROPERTY(meta = (BindWidget)) UButton* AudioSettingsButton;
	UPROPERTY(meta = (BindWidget)) USlider* MasterSlider;
	UPROPERTY(meta = (BindWidget)) USlider* MusicSlider;
	UPROPERTY(meta = (BindWidget)) USlider* SoundEffectsSlider;
	UPROPERTY(meta = (BindWidget)) USlider* AmbientSoundsSlider;

	UPROPERTY(meta = (BindWidget)) UTextBlock* MasterNumber;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MusicNumber;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SoundEffectsNumber;
	UPROPERTY(meta = (BindWidget)) UTextBlock* AmbientSoundsNumber;

	// Input
	UPROPERTY(meta = (BindWidget)) UButton* InputSettingsButton;

	UPROPERTY(meta = (BindWidget)) USlider* MouseWheelSpeedSlider;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MouseWheelSpeedNumber;
	
	UPROPERTY(meta = (BindWidget)) USlider* MouseDragRotateSpeedSlider;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MouseDragRotateSpeedNumber;

	UPROPERTY(meta = (BindWidget)) UButton* RestoreDefaultsButton;

	// Others
	UPROPERTY(meta = (BindWidget)) UButton* OtherSettingsButton;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* AutosaveDropdown;

private:
	void SetupResolutionDropdown() {
		ResolutionDropdown->ClearOptions();
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(resolutions);
		FIntPoint screenResolution = GetGameUserSettings()->GetScreenResolution();
		
		int32 selectedIndex = 0;
		for (FIntPoint point : resolutions) {
			ResolutionDropdown->AddOption(FString::FromInt(point.X) + "x" + FString::FromInt(point.Y));
			if (screenResolution == point) {
				selectedIndex = ResolutionDropdown->GetOptionCount() - 1;
			}
		}
		ResolutionDropdown->SetSelectedIndex(selectedIndex);
	}

	void ApplyChanges();
	void UndoChanges();
	void ExecuteAfterConfirmOrDiscard();

	bool _isSettingsDirty = false;
	int32 _tabIndexToChangeTo = -1; // -1 is for UI Close


private:
	/*
	 * Confirmation Popup
	 */
	UFUNCTION() void OnClickedConfirmChangesButton() {
		ConfirmOverlay->SetVisibility(ESlateVisibility::Collapsed);
		ApplyChanges();
		ExecuteAfterConfirmOrDiscard();
	}
	UFUNCTION() void OnClickedDiscardChangesButton() {
		ConfirmOverlay->SetVisibility(ESlateVisibility::Collapsed);
		UndoChanges();
		ExecuteAfterConfirmOrDiscard();
	}

	/*
	 * Settings Apply
	 */
	UFUNCTION() void OnClickSettingsApply() {
		ApplyChanges();
		Spawn2DSound("UI", "ButtonClick");
	}
	UFUNCTION() void OnClickSettingsConfirm() {
		TryOpenConfirmUI(-1);
		Spawn2DSound("UI", "ButtonClick");
	}
	UFUNCTION() void OnClickSettingsCancel() {
		TryOpenConfirmUI(-1); // Note, same path as Confirm...
		Spawn2DSound("UI", "UIWindowClose");
	}

	void ChangeTab(int32 tabIndex);
	void TryOpenConfirmUI(int32 tabIndexToChangeTo) {
		if (_isSettingsDirty) {
			ConfirmOverlay->SetVisibility(ESlateVisibility::Visible);
			_tabIndexToChangeTo = tabIndexToChangeTo;
		} else {
			_tabIndexToChangeTo = tabIndexToChangeTo;
			ExecuteAfterConfirmOrDiscard();
		}
	}

	UFUNCTION() void OnClickGraphicsSettings() { TryOpenConfirmUI(0); }
	UFUNCTION() void OnClickAudioSettings() { TryOpenConfirmUI(1); }
	UFUNCTION() void OnClickInputSettings() { TryOpenConfirmUI(2); }
	UFUNCTION() void OnClickOtherSettings() { TryOpenConfirmUI(3); }

	/*
	 * 
	 */
	UFUNCTION() void OnMasterVolumeChanged(float volume) {
		_isSettingsDirty = true;
		gameInstance()->SetMasterVolume(std::max(volume, MinVolume)); // 0.01 is so that it doesn't Stop()
		gameInstance()->RefreshSoundSettings();
	}
	UFUNCTION() void OnMusicVolumeChanged(float volume) {
		_isSettingsDirty = true;
		gameInstance()->SetMusicVolume(std::max(volume, MinVolume));
		gameInstance()->RefreshSoundSettings();
	}
	UFUNCTION() void OnSoundEffectsVolumeChanged(float volume) {
		_isSettingsDirty = true;
		gameInstance()->SetSoundEffectsVolume(std::max(volume, MinVolume));
		gameInstance()->RefreshSoundSettings();
	}
	UFUNCTION() void OnAmbientSoundsVolumeChanged(float volume) {
		_isSettingsDirty = true;
		gameInstance()->SetAmbientVolume(std::max(volume, MinVolume));
		gameInstance()->RefreshSoundSettings();
	}

	UFUNCTION() void OnMouseWheelSpeedChanged(float fraction) {
		_isSettingsDirty = true;
		gameInstance()->mouseZoomSpeedFraction = fraction;
		gameInstance()->RefreshSoundSettings();
	}
	UFUNCTION() void OnMouseDragRotateSpeedChanged(float fraction) {
		_isSettingsDirty = true;
		gameInstance()->mouseRotateSpeedFraction = fraction;
		gameInstance()->RefreshSoundSettings();
	}

	UFUNCTION() void OnResolutionDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnUIScalingDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnWindowModeDropdownChanged(FString sItem, ESelectInfo::Type seltype);

	UFUNCTION() void OnResolutionChanged(float value);
	UFUNCTION() void OnAntiAliasingDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnPostProcessingDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnShadowsDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnTexturesDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnEffectsDropdownChanged(FString sItem, ESelectInfo::Type seltype);

	UFUNCTION() void OnMaxFrameRateDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnVSyncCheckBoxChecked(bool active);
	
	UFUNCTION() void RestoreDefault();

	UFUNCTION() void OnAutosaveDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	
	void ResetTabSelection();

private:

	void RefreshResolutionDropdown();

	static UGameUserSettings* GetGameUserSettings() {
		if (GEngine != nullptr) {
			PUN_CHECK(GEngine->GameUserSettings);
			return GEngine->GameUserSettings;
		}
		UE_DEBUG_BREAK();
		return nullptr;
	}

	
private:
	bool _videoModeChanged = false;

	TArray<FIntPoint> resolutions;
	
	UPROPERTY() UPunWidget* _callbackParent;
};
