// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
//#include "EditableTextBox.h"
#include "IconTextPairWidget.h"
//#include "PunEditableText.h"
#include "PunEditableNumberBox.h"
#include "TradeRowBase.h"
#include "WorldTradeRow.generated.h"

/**
 * !!! This is the one in use
 */
UCLASS()
class UWorldTradeRow : public UTradeRowBase
{
	GENERATED_BODY()
public:
	void Init(class UWorldTradeUI* worldTradeUI, ResourceEnum resourceEnumIn);
	void UpdateTexts();

	int32 buyAmount() {
		return PunTradeAmount->amount;
	}

	// This can be negative for sell
	void SetBuyAmount(int32 amount) {
		PunTradeAmount->amount = amount;
	}

	void UpdateEditableBoxTexts()
	{
		PunTradeAmount->UpdateText();
	}

	void RefreshSellOnlyState(bool isSellOnly) {
		// Hide arrow up when we are only selling. Also, arrow up must be shown when the amount is negative (selling) so players can increase the amount back.
		PunTradeAmount->ArrowUpButton->SetVisibility((isSellOnly && PunTradeAmount->amount >= 0) ? ESlateVisibility::Hidden : ESlateVisibility::Visible);

		// Also set the maximum to 0 (sell amount is negative, subtracting off inventory)
		PunTradeAmount->maxAmount = isSellOnly ? 0 : MAX_int32;
	}

	// Callback for PunEditableNumberBox
	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum) final;

private:
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* PunTradeAmount;

private:
	UWorldTradeUI* _worldTradeUI;
};
