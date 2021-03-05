// Fill out your copyright notice in the Description page of Project Settings.


#include "PunEditableNumberBox.h"

using namespace std;

void UPunEditableNumberBox::ClickArrow(bool isDown)
{
	int32 changeAmount = isDown ? -1 : 1;

	if (dataSource()->isCtrlDown()) 
	{
		if (isUsingSpecialControl)
		{
			if (isDown) {
				if (amount > 0) {
					changeAmount = -amount; // Go to 0 first if the amount is more than 0
				}
				else {
					changeAmount *= ctrlClickDecrementAmount;
				}
			}
			else {
				if (amount < 0) {
					changeAmount = -amount; // Go to 0 first if the amount is less than 0
				}
				else {
					changeAmount *= ctrlClickIncrementAmount;
				}
			}
		}
		else
		{
			changeAmount *= isDown ? ctrlClickDecrementAmount : ctrlClickIncrementAmount;
		}
	}
	else if (dataSource()->isShiftDown()) {
		changeAmount *= shiftIncrementMultiplier;
	}
	else {
		changeAmount *= incrementMultiplier;
	}
	
	amount += changeAmount;

	amount = min(maxAmount, max(minAmount, amount));

	UpdateText();

	if (_callbackTarget) {
		_callbackTarget->CallBack1(this, _callbackEnum);
	}
	else if (onEditNumber) {
		onEditNumber(punId, uiIndex, amount, networkInterface());
	}

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
	if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus) 
	{
		//PUN_DEBUG(FString("TradeAmountTextChanged Inside"));
		FString tradeAmountString = Text.ToString();
		amount = tradeAmountString.IsNumeric() ? FCString::Atoi(*tradeAmountString) : 0;
		amount = min(maxAmount, max(minAmount, amount));

		UpdateText();

		if (_callbackTarget) {
			_callbackTarget->CallBack1(this, _callbackEnum);
		}
		else if (onEditNumber) {
			onEditNumber(punId, uiIndex, amount, networkInterface());
		}
	}
}
