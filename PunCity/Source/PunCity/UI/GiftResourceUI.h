// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunWidget.h"
#include "PunEditableNumberBox.h"
#include "GiftResourceUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UGiftResourceUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmButton;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton1;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton2;

	UPROPERTY(meta = (BindWidget)) UTextBlock* GiftTitleText;
	UPROPERTY(meta = (BindWidget)) UImage* GiftIcon;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* GiftTypeDropdown;
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* GiftTargetAmount;

	int32 targetPlayerId = -1;
	
	void PunInit()
	{
		BUTTON_ON_CLICK(ConfirmButton, this, &UGiftResourceUI::OnClickConfirmButton);
		BUTTON_ON_CLICK(CloseButton1, this, &UGiftResourceUI::OnClickCloseButton);
		BUTTON_ON_CLICK(CloseButton2, this, &UGiftResourceUI::OnClickCloseButton);

		GiftTypeDropdown->OnSelectionChanged.Clear();
		GiftTypeDropdown->OnSelectionChanged.AddDynamic(this, &UGiftResourceUI::OnDropDownChanged);
		GiftTypeDropdown->ClearOptions();
		GiftTypeDropdown->AddOption("Money");
		for (ResourceInfo info : ResourceInfos) {
			GiftTypeDropdown->AddOption(ToFString(info.name));
		}

		SetChildHUD(GiftTargetAmount);
		GiftTargetAmount->minAmount = 0;

		SetVisibility(ESlateVisibility::Collapsed);
	}

	void OpenUI(int32 targetPlayerIdIn)
	{
		targetPlayerId = targetPlayerIdIn;
		SetText(GiftTitleText, "Gift to " + simulation().playerName(targetPlayerId));

		GiftTypeDropdown->SetSelectedOption("Money");
		GiftIcon->SetBrushFromTexture(assetLoader()->CoinIcon);
		
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void CloseUI() {
		SetVisibility(ESlateVisibility::Collapsed);
	}

	void TickUI()
	{
		
	}

	UFUNCTION() void OnDropDownChanged(FString sItem, ESelectInfo::Type seltype)
	{
		std::string resourceName = ToStdString(GiftTypeDropdown->GetSelectedOption());
		if (resourceName == "") {
			return;
		}
		
		if (resourceName == "Money") {
			GiftIcon->SetBrushFromTexture(assetLoader()->CoinIcon);
		}
		else {
			ResourceEnum resourceEnum = FindResourceEnumByName(resourceName);
			GiftIcon->SetBrushFromMaterial(assetLoader()->GetResourceIconMaterial(resourceEnum));
		}
	}

	UFUNCTION() void OnClickConfirmButton()
	{
		if (targetPlayerId != -1)
		{
			std::string resourceName = ToStdString(GiftTypeDropdown->GetSelectedOption());
			ResourceEnum resourceEnum;
			int32 amount = GiftTargetAmount->amount;

			if (resourceName == "Money") {
				resourceEnum = ResourceEnum::Money;
				if (amount > simulation().money(playerId())) {
					simulation().AddPopupToFront(playerId(), "Not enough money to give.", ExclusiveUIEnum::GiftResourceUI, "PopupCannot");
					return;
				}
			} else {
				resourceEnum = FindResourceEnumByName(resourceName);
				if (amount > simulation().resourceCount(playerId(), resourceEnum)) {
					simulation().AddPopupToFront(playerId(), "Not enough resource to give.", ExclusiveUIEnum::GiftResourceUI, "PopupCannot");
					return;
				}
			}
			
			auto command = make_shared<FGenericCommand>();
			command->genericCommandType = FGenericCommand::Type::SendGift;
			command->intVar1 = static_cast<int>(targetPlayerId);
			command->intVar2 = static_cast<int>(resourceEnum);
			command->intVar3 = static_cast<int>(amount);

			networkInterface()->SendNetworkCommand(command);
		}
		
		SetVisibility(ESlateVisibility::Collapsed);
	}

	UFUNCTION() void OnClickCloseButton() {
		SetVisibility(ESlateVisibility::Collapsed);
	}
	
};
