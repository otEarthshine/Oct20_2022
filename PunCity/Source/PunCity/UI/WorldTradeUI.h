// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "WorldTradeRow.h"

#include "WorldTradeUI.generated.h"

/**
 * 
 */
UCLASS()
class UWorldTradeUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void PunInit();

	void OpenUI(int32 objectId = -1);
	void CloseUI() {
		if (GetVisibility() != ESlateVisibility::Collapsed) {
			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}
		SetVisibility(ESlateVisibility::Collapsed);
	}
	
	void TickUI();
	void UpdateTotal();

	int32 worldTradeUITownId() { return simulation().building(punId).townId(); }
	
	UFUNCTION() void ClickedTradeButton();
	UFUNCTION() void ClickedDismissButton() {
		CloseUI();
	}

	UPROPERTY(meta = (BindWidget)) UOverlay* TradeOverlay;

	UPROPERTY(meta = (BindWidget)) UScrollBox* WorldTradeRowBox;

	UPROPERTY(meta = (BindWidget)) UButton* TradeButton;
	UPROPERTY(meta = (BindWidget)) UButton* DismissButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* TradeCloseXButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* QuantityText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* FeePercentText;

	UPROPERTY(meta = (BindWidget)) USizeBox* BuyAmountTitle;
	
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* BuyMoney;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* SellMoney;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* FeeTextPair; // Having separate fee makes it easier for players to see the fee's effect and make better decisions.
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* TotalTextPair;

	UPROPERTY(meta = (BindWidget)) UEditableTextBox* SearchBox;


	int32 tradeCategoryState = -1;
	UPROPERTY(meta = (BindWidget)) UImage* TradeNeedsActiveImage;
	UPROPERTY(meta = (BindWidget)) UImage* TradeConstructionActiveImage;
	UPROPERTY(meta = (BindWidget)) UImage* TradeLuxuryActiveImage;
	UPROPERTY(meta = (BindWidget)) UImage* TradeOthersActiveImage;

	UPROPERTY(meta = (BindWidget)) UButton* TradeNeedsButton;
	UPROPERTY(meta = (BindWidget)) UButton* TradeConstructionButton;
	UPROPERTY(meta = (BindWidget)) UButton* TradeLuxuryButton;
	UPROPERTY(meta = (BindWidget)) UButton* TradeOthersButton;

	UFUNCTION() void ClickTradeNeedsButton() {
		tradeCategoryState = (tradeCategoryState != 0) ? 0 : -1;
	}
	UFUNCTION() void ClickTradeConstructionButton() {
		tradeCategoryState = (tradeCategoryState != 1) ? 1 : -1;
	}
	UFUNCTION() void ClickTradeLuxuryButton() {
		tradeCategoryState = (tradeCategoryState != 2) ? 2 : -1;
	}
	UFUNCTION() void ClickTradeOthersButton() {
		tradeCategoryState = (tradeCategoryState != 3) ? 3 : -1;
	}

private:
	int32 _totalCoinGain = 0;
	int32 _quantity = 0;
	//const int32_t _maxQuantity = 500;
};
