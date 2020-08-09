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

	void Refresh();
	
	UPROPERTY(meta = (BindWidget)) UWidgetSwitcher* SettingsMenu;

	UPROPERTY(meta = (BindWidget)) UButton* BackButton;
	UPROPERTY(meta = (BindWidget)) UButton* SettingsCancelButton;
	UPROPERTY(meta = (BindWidget)) UButton* SettingsConfirmButton;
	UPROPERTY(meta = (BindWidget)) UButton* SettingsApplyButton;

	// Graphics
	UPROPERTY(meta = (BindWidget)) UButton* GraphicsSettingsButton;

	UPROPERTY(meta = (BindWidget)) UComboBoxString* ResolutionDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* WindowModeDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* UIScalingDropdown;

	UPROPERTY(meta = (BindWidget)) USlider* ResolutionSlider;
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

	// Input
	UPROPERTY(meta = (BindWidget)) UButton* InputSettingsButton;
	UPROPERTY(meta = (BindWidget)) USlider* MouseWheelSpeedSlider;

	UPROPERTY(meta = (BindWidget)) UButton* RestoreDefaultsButton;


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
	

private:
	UFUNCTION() void OnMasterVolumeChanged(float volume) {
		gameInstance()->SetMasterVolume(std::max(volume, MinVolume)); // 0.01 is so that it doesn't Stop()
		gameInstance()->RefreshSoundSettings();
	}
	UFUNCTION() void OnMusicVolumeChanged(float volume) {
		gameInstance()->SetMusicVolume(std::max(volume, MinVolume));
		gameInstance()->RefreshSoundSettings();
	}
	UFUNCTION() void OnSoundEffectsVolumeChanged(float volume) {
		gameInstance()->SetSoundEffectsVolume(std::max(volume, MinVolume));
		gameInstance()->RefreshSoundSettings();
	}
	UFUNCTION() void OnAmbientSoundsVolumeChanged(float volume) {
		gameInstance()->SetAmbientVolume(std::max(volume, MinVolume));
		gameInstance()->RefreshSoundSettings();
	}

	UFUNCTION() void OnMouseWheelSpeedChanged(float fraction) {
		gameInstance()->mouseZoomSpeedFraction = fraction;
		gameInstance()->RefreshSoundSettings();
	}

	UFUNCTION() void OnClickSettingsApply();
	UFUNCTION() void OnClickSettingsConfirm();
	UFUNCTION() void OnClickSettingsCancel();

	UFUNCTION() void OnClickGraphicsSettings();
	UFUNCTION() void OnClickAudioSettings();
	UFUNCTION() void OnClickInputSettings();

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
	
	void ResetTabSelection();

private:

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
