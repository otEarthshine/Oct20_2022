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

	const FText MoneyText = NSLOCTEXT("UGiftResourceUI", "Money", "Money");
	
	void PunInit()
	{
		BUTTON_ON_CLICK(ConfirmButton, this, &UGiftResourceUI::OnClickConfirmButton);
		BUTTON_ON_CLICK(CloseButton1, this, &UGiftResourceUI::OnClickCloseButton);
		BUTTON_ON_CLICK(CloseButton2, this, &UGiftResourceUI::OnClickCloseButton);

		GiftTypeDropdown->OnSelectionChanged.Clear();
		GiftTypeDropdown->OnSelectionChanged.AddDynamic(this, &UGiftResourceUI::OnDropDownChanged);

		SetChildHUD(GiftTargetAmount);
		GiftTargetAmount->minAmount = 0;

		SetVisibility(ESlateVisibility::Collapsed);
	}

	void OpenUI(int32 targetPlayerIdIn)
	{
		targetPlayerId = targetPlayerIdIn;
		
		SetText(GiftTitleText, FText::Format(
			NSLOCTEXT("GiftResourceUI", "GiftToX", "Gift to {0}"),
			simulation().playerNameT(targetPlayerId)
		));

		// Gift Type Dropdown
		GiftTypeDropdown->ClearOptions();
		GiftTypeDropdown->AddOption(MoneyText.ToString());
		for (ResourceInfo info : SortedNameResourceInfo) {
			GiftTypeDropdown->AddOption(info.name.ToString());
		}
		GiftTypeDropdown->SetSelectedOption(MoneyText.ToString());

		
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
		FString resourceName = GiftTypeDropdown->GetSelectedOption();
		if (resourceName.IsEmpty()) {
			return;
		}
		
		if (resourceName == MoneyText.ToString()) {
			GiftIcon->SetBrushFromTexture(assetLoader()->CoinIcon);
		}
		else {
			ResourceEnum resourceEnum = FindResourceEnumByName(ToWString(resourceName));
			GiftIcon->SetBrushFromMaterial(assetLoader()->GetResourceIconMaterial(resourceEnum));
		}
	}

	UFUNCTION() void OnClickConfirmButton();

	UFUNCTION() void OnClickCloseButton() {
		SetVisibility(ESlateVisibility::Collapsed);
	}
	
};
