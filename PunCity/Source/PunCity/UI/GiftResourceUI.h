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
		GiftTypeDropdown->AddOption(NSLOCTEXT("GiftResourceUI", "Money", "Money").ToString());
		
		for (ResourceInfo info : SortedNameResourceInfo) {
			GiftTypeDropdown->AddOption(info.name.ToString());
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
		std::wstring resourceName = ToWString(GiftTypeDropdown->GetSelectedOption());
		if (resourceName == TEXT("")) {
			return;
		}
		
		if (resourceName == TEXT("Money")) {
			GiftIcon->SetBrushFromTexture(assetLoader()->CoinIcon);
		}
		else {
			ResourceEnum resourceEnum = FindResourceEnumByName(resourceName);
			GiftIcon->SetBrushFromMaterial(assetLoader()->GetResourceIconMaterial(resourceEnum));
		}
	}

	UFUNCTION() void OnClickConfirmButton();

	UFUNCTION() void OnClickCloseButton() {
		SetVisibility(ESlateVisibility::Collapsed);
	}
	
};
