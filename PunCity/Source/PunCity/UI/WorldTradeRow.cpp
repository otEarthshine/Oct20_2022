// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldTradeRow.h"
#include "../Simulation/GameSimulationCore.h"
#include "WorldTradeUI.h"

using namespace std;

void UWorldTradeRow::Init(UWorldTradeUI* worldTradeUI, ResourceEnum resourceEnumIn)
{
	_worldTradeUI = worldTradeUI;

	_resourceEnum = resourceEnumIn;
	inventory = 0;

	auto setupTradeAmount = [&](UPunEditableNumberBox* tradeAmountBox)
	{
		SetChildHUD(tradeAmountBox);
		tradeAmountBox->Set(this, CallbackEnum::None);
		tradeAmountBox->amount = 0;
		//tradeAmountBox->minAmount = 0;
		tradeAmountBox->UpdateText();
	};
	
	setupTradeAmount(PunTradeAmount);
	
	//SetChildHUD(PunTradeAmount);
	//PunTradeAmount->Set(this, CallbackEnum::None);
	//PunTradeAmount->amount = 0;
	//PunTradeAmount->UpdateText();

	
	UpdateTexts();
}

void UWorldTradeRow::UpdateTexts()
{
	UpdateTextsBase();

	int32 townId = _worldTradeUI->worldTradeUITownId();
	
	FString inventoryCount = FString::FromInt(simulation().resourceCountTown(townId, _resourceEnum));

	int32 tradeAmount = PunTradeAmount->amount;

	//if (PunTradeAmount->amount != 0) {
	if (tradeAmount != 0) {
		if (tradeAmount > 0) {
			inventoryCount.Append("+");
		}
		inventoryCount.Append(FString::FromInt(tradeAmount));
	}
	SetFString(InventoryText, inventoryCount);
}

void UWorldTradeRow::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum)
{
	_worldTradeUI->UpdateTotal();
}