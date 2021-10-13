// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"

#include "PunRichText.h"
#include "PunDropdown.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunDropdown : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UComboBoxString* Dropdown;

	int32 dropdownIndex = 0;

	void OnInit() override {
		Dropdown->OnSelectionChanged.Clear();
		Dropdown->OnSelectionChanged.AddDynamic(this, &UPunDropdown::OnDropdownChanged);
	}

	void Set(int32 objectId, TArray<FText> options, FText selectedOption, std::function<void(int32, FString, IGameUIDataSource*, IGameNetworkInterface*, int32, int32)> onDropdownChanged, TArray<int32> optionInts = {})
	{
		if (!TextArrayEquals(options, _lastOptions)) 
		{
			_lastOptions = options;
			_lastOptionInts = optionInts;

			//Dropdown->OnGenerateWidgetEvent.BindUFunction(this, "OnGenerateWidget");
			
			Dropdown->ClearOptions();
			for (const auto& option : options) {
				Dropdown->AddOption(option.ToString());
			}
			Dropdown->SetSelectedOption(selectedOption.ToString());
		}
		
		_onDropdownChanged = onDropdownChanged;
		punId = objectId;
	}
	

	UFUNCTION() void OnDropdownChanged(FString sItem, ESelectInfo::Type seltype)
	{
		if (seltype == ESelectInfo::Type::Direct) return;

		PUN_LOG("OnDropDownChanged: %s ... %d", *sItem, (int)seltype);
		if (sItem.IsEmpty()) return;

		
		int32 optionInt = -1;
		for (int32 i = 0; i < _lastOptions.Num(); i++) {
			if (_lastOptions[i].ToString() == sItem) {
				optionInt = _lastOptionInts[i];
			}
		}
		
		_onDropdownChanged(punId, sItem, dataSource(), networkInterface(), dropdownIndex, optionInt);
	}

private:
	TArray<FText> _lastOptions;
	TArray<int32> _lastOptionInts;
	
	std::function<void(int32, FString, IGameUIDataSource*, IGameNetworkInterface*, int32, int32)> _onDropdownChanged;
};
