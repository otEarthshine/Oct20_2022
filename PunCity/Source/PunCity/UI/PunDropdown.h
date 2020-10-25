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

	void Set(int32 objectId, std::vector<std::string> options, std::string selectedOption, std::function<void(int32, FString, IGameUIDataSource*, IGameNetworkInterface*, int32)> onDropdownChanged)
	{
		if (options != _lastOptions) {
			_lastOptions = options;

			//Dropdown->OnGenerateWidgetEvent.BindUFunction(this, "OnGenerateWidget");
			
			Dropdown->ClearOptions();
			for (const auto& option : options) {
				Dropdown->AddOption(ToFString(option));
			}
			Dropdown->SetSelectedOption(ToFString(selectedOption));
		}
		
		_onDropdownChanged = onDropdownChanged;
		punId = objectId;
	}
	

	UFUNCTION() void OnDropdownChanged(FString sItem, ESelectInfo::Type seltype)
	{
		if (seltype == ESelectInfo::Type::Direct) return;

		PUN_LOG("OnDropDownChanged: %s ... %d", *sItem, (int)seltype);
		if (sItem.IsEmpty()) return;

		_onDropdownChanged(punId, sItem, dataSource(), networkInterface(), dropdownIndex);
	}

private:
	std::vector<std::string> _lastOptions;
	
	std::function<void(int32, FString, IGameUIDataSource*, IGameNetworkInterface*, int32)> _onDropdownChanged;
};
