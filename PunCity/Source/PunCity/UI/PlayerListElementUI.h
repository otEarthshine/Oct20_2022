// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PunWidget.h"
#include "PlayerListElementUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPlayerListElementUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerName;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* FactionName;

	UPROPERTY(meta = (BindWidget)) UButton* PlayerReadyButton;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerReadyFill;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerReadyText;

	UPROPERTY(meta = (BindWidget)) UImage* PlayerHighlight;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ReadyParent;

	UPROPERTY(meta = (BindWidget)) UImage* SaveGameTransferBar;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SaveGameTransferText;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* EmptyText;
	UPROPERTY(meta = (BindWidget)) UButton* EmptySelectButton;
	
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* PlayerKickButton;

	UPROPERTY(meta = (BindWidget)) UButton* PlayerLogoChangeButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* PreviousPlayerText;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* DebugText;

	UPROPERTY(meta = (BindWidget)) UImage* BottomBlackFade;
	UPROPERTY(meta = (BindWidget)) UImage* FactionBackground;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerLogoBackground;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerLogoForeground;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerCharacterImage;

	UPROPERTY(meta = (BindWidget)) UComboBoxString* FactionSelectionDropdown;


	FString playerName;
	
	void PunInit(UPunWidget* parent, int32 slotIdIn) {
		_parent = parent;
		slotId = slotIdIn;
		BUTTON_ON_CLICK(PlayerKickButton->CoreButton, this, &UPlayerListElementUI::OnClickPlayerKickButton);
		BUTTON_ON_CLICK(EmptySelectButton, this, &UPlayerListElementUI::OnClickEmptySelectButton);
		BUTTON_ON_CLICK(PlayerLogoChangeButton, this, &UPlayerListElementUI::OnClickPlayerLogoChangeButton);

		FactionSelectionDropdown->OnSelectionChanged.Clear();
		FactionSelectionDropdown->OnSelectionChanged.AddDynamic(this, &UPlayerListElementUI::OnFactionSelectionDropdownChanged);
		FactionSelectionDropdown->ClearOptions();
		for (const FactionInfo& factionInfo : FactionInfos) {
			FactionSelectionDropdown->AddOption(factionInfo.name.ToString());
		}
		FactionSelectionDropdown->SetSelectedIndex(0);
	}
	
	UFUNCTION() void OnClickPlayerKickButton();

	UFUNCTION() void OnClickEmptySelectButton() {
		_parent->CallBack1(this, CallbackEnum::SelectEmptySlot);
	}

	UFUNCTION() void OnClickPlayerLogoChangeButton() {
		_parent->CallBack1(this, CallbackEnum::LobbyChoosePlayerSettings);
	}

	UFUNCTION() void OnFactionSelectionDropdownChanged(FString sItem, ESelectInfo::Type seltype)
	{
		if (seltype != ESelectInfo::Direct) 
		{
			replyFactionEnum = FindFactionEnumFromName(sItem);
			_parent->CallBack1(this, CallbackEnum::LobbyChooseFactionFromDropdown);
		}
	}


	int32 slotId = -1;

	FactionEnum replyFactionEnum = FactionEnum::Europe;
private:
	UPROPERTY() UPunWidget* _parent;
};
