// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "PunCity/Sound/SoundSystemComponent.h"

#include "HiddenSettingsRow.generated.h"


/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UHiddenSettingsRow : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* SettingNameText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* CurrentValueText;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* ValueInputBox;
	UPROPERTY(meta = (BindWidget)) UButton* ApplyButton;

	void InitRow(std::vector<std::string> keys)
	{
		_keys = keys;

		if (_keys.size() == 3) {
			SettingNameText->SetText(ToFText("    "+ _keys[2]));
		} else {
			SettingNameText->SetText(ToFText("  " + _keys[1]));
		}

		float value = GetValueReference();
		CurrentValueText->SetText(FText::FromString(FString::SanitizeFloat(value, 2)));
		ApplyButton->OnClicked.AddDynamic(this, &UHiddenSettingsRow::OnApplySetting);
	}

	UFUNCTION() void OnApplySetting()
	{
		float value = FCString::Atof(*ValueInputBox->GetText().ToString());
		CurrentValueText->SetText(FText::FromString(FString::SanitizeFloat(value, 2)));

		GetValueReference() = value;

		// Apply sound settings right away (Pitch, Volume etc.)
		//if (_keys.size() == 3) {
		//	dataSource()->ambientSound()->ApplySoundSettings(_keys[0], _keys[1]);
		//}
	}

	float defaultValue = 1.0f;
	float& GetValueReference()
	{
		PUN_CHECK(_keys.size() == 2 || _keys.size() == 3);
		
		if (_keys.size() == 2) {
			return dataSource()->soundSystem()->GetGroupPropertyRef(_keys[0], _keys[1]);
		}
		return dataSource()->soundSystem()->GetSoundPropertyRef(_keys[0], _keys[1], _keys[2]);
	}

private:
	std::vector<std::string> _keys;
	
	//FString _hiddenSettingKey;
};
