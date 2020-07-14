// Fill out your copyright notice in the Description page of Project Settings.

#include "TradeUIWidgetBase.h"

using namespace std;

//void UTradeUIWidgetBase::PunInit()
//{
//	TradeButton->OnClicked.AddDynamic(this, &UTradeUIWidgetBase::ClickedTradeButton);
//	DismissButton->OnClicked.AddDynamic(this, &UTradeUIWidgetBase::ClickedDismissButton);
//
//	this->SetVisibility(ESlateVisibility::Collapsed);
//}
//
//void UTradeUIWidgetBase::OpenUI(int32_t objectId)
//{
//	BuyRows->ClearChildren();
//	SellRows->ClearChildren();
//	_buyPairs.clear();
//	_sellPairs.clear();
//
//	ResourceSystem& resourceSystem = dataSource()->simulation().resourceSystem(playerId());
//
//	punId = objectId;
//
//	for (ResourceInfo info : ResourceInfos) 
//	{
//		// Create Buy Row
//		{
//			auto tradeRow = AddWidget<UTradeRowUIWidgetBase>(UIEnum::TradeRow);
//			tradeRow->SetupInterfaces(dataSource(), networkInterface(), this);
//			BuyRows->AddChild(tradeRow);
//
//			tradeRow->resourceEnum = info.resourceEnum;
//			tradeRow->tradeAmount = 0;
//			tradeRow->maxTradeAmount = 500;
//			tradeRow->CommitTradeAmount(true);
//			_buyPairs.emplace(info.resourceEnum, tradeRow);
//		}
//
//		// Create SellRow Row
//		int32 resourceCount = resourceSystem.resourceCount(info.resourceEnum);
//		if (resourceCount > 0) 
//		{
//			auto tradeRow = AddWidget<UTradeRowUIWidgetBase>(UIEnum::TradeRow);
//			tradeRow->SetupInterfaces(dataSource(), networkInterface(), this);
//			SellRows->AddChild(tradeRow);
//
//			tradeRow->resourceEnum = info.resourceEnum;
//			tradeRow->tradeAmount = 0;
//			tradeRow->maxTradeAmount = 0; // Starts with 0, but update will change this
//			tradeRow->CommitTradeAmount(true);
//			_sellPairs.emplace(info.resourceEnum, tradeRow);
//		}
//	}
//
//	SetVisibility(ESlateVisibility::Visible);
//	TickUI();
//}
//
//void UTradeUIWidgetBase::TickUI()
//{
//	if (GetVisibility() != ESlateVisibility::Visible) return;
//
//	ResourceSystem& resourceSystem = dataSource()->simulation().resourceSystem(playerId());
//
//	for (auto& it : _buyPairs) {
//		it.second->UpdateNonTradeAmountTexts();
//		it.second->CommitTradeAmount(false);
//	}
//
//	for (auto& it : _sellPairs) {
//		bool sameMaxTradeAmount = it.second->maxTradeAmount == resourceSystem.resourceCount(it.first);
//		it.second->maxTradeAmount = resourceSystem.resourceCount(it.first);
//		it.second->UpdateNonTradeAmountTexts();
//		it.second->CommitTradeAmount(!sameMaxTradeAmount);
//	}
//
//	UpdateTotal();
//}
//
//void UTradeUIWidgetBase::UpdateTotal()
//{
//	_totalGain = 0;
//	for (auto& it : _sellPairs) {
//		int32 unitCost = ResourceInfos[(int)it.first].basePrice;
//		_totalGain += it.second->tradeAmount * unitCost;
//	}
//
//	for (auto& it : _buyPairs) {
//		int32 unitCost = ResourceInfos[(int)it.first].basePrice;
//		_totalGain -= it.second->tradeAmount * unitCost;
//	}
//
//	Total->SetText(FText::FromString(FString::FromInt(_totalGain)));
//}
//
//void UTradeUIWidgetBase::ClickedTradeButton()
//{
//	ResourceSystem& resourceSystem = dataSource()->simulation().resourceSystem(playerId());
//	if (_totalGain + resourceSystem.money() < 0) return;
//
//	auto tradeCommand = make_shared<FTradeResource>();
//
//	//// Add resources bought
//	//for (auto& it : _buyPairs) {
//	//	tradeCommand->buyEnums.Add((uint8)it.first);
//	//	tradeCommand->buyAmounts.Add(it.second->tradeAmount);
//	//	it.second->tradeAmount = 0;
//	//	//if (tradeAmount) resourceSystem.AddResource(it.first, tradeAmount, simulation);
//	//}
//
//	//// Remove resources sold
//	//for (auto& it : _sellPairs) {
//	//	tradeCommand->sellEnums.Add((uint8)it.first);
//	//	tradeCommand->sellAmounts.Add(it.second->tradeAmount);
//	//	it.second->tradeAmount = 0;
//	//	//if (tradeAmount) resourceSystem.RemoveResource(it.first, tradeAmount, simulation);
//	//}
//
//	// Change money from trade's Total
//	//resourceSystem.ChangeMoney(_totalGain);
//	tradeCommand->totalGain = _totalGain;
//	_totalGain = 0;
//
//	tradeCommand->objectId = punId;
//
//	networkInterface()->SendNetworkCommand(tradeCommand);
//
//	this->SetVisibility(ESlateVisibility::Collapsed);
//}
//
//void UTradeUIWidgetBase::ClickedDismissButton()
//{
//	this->SetVisibility(ESlateVisibility::Collapsed);
//}