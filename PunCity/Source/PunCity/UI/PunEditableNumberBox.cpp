// Fill out your copyright notice in the Description page of Project Settings.


#include "PunEditableNumberBox.h"

using namespace std;

void UPunEditableNumberBox::ClickArrow(bool isDown)
{
	int32 changeAmount = isDown ? -1 : 1;
	if (dataSource()->isCtrlDown() || dataSource()->isShiftDown()) {
		changeAmount *= 10;
	}
	amount += changeAmount * incrementMultiplier;

	amount = min(maxAmount, max(minAmount, amount));

	UpdateText();
	_callbackTarget->CallBack1(this, _callbackEnum);

	dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
}

void UPunEditableNumberBox::ClickArrowDownButton() {
	ClickArrow(true);
}
void UPunEditableNumberBox::ClickArrowUpButton() {
	ClickArrow(false);
}

void UPunEditableNumberBox::NumberChanged(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus) {
		//PUN_DEBUG(FString("TradeAmountTextChanged Inside"));
		FString tradeAmountString = Text.ToString();
		amount = tradeAmountString.IsNumeric() ? FCString::Atoi(*tradeAmountString) : 0;

		UpdateText();
		_callbackTarget->CallBack1(this, _callbackEnum);
	}
}