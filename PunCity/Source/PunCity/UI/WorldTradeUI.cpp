// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldTradeUI.h"
#include "PunCity/Simulation/GameSimulationCore.h"
#include "PunCity/Simulation/Buildings/GathererHut.h"

using namespace std;

#define LOCTEXT_NAMESPACE "WorldTradeUI"

void UWorldTradeUI::PunInit()
{
	TradeButton->OnClicked.AddDynamic(this, &UWorldTradeUI::ClickedTradeButton);
	DismissButton->OnClicked.AddDynamic(this, &UWorldTradeUI::ClickedDismissButton);

	SetVisibility(ESlateVisibility::Collapsed);
}

void UWorldTradeUI::OpenUI(int32 objectId)
{
	WorldTradeRowBox->ClearChildren();
	punId = objectId;
	
	ResourceSystem& resourceSystem = simulation().resourceSystem(worldTradeUITownId());

	bool isSellOnly = simulation().building(objectId).isEnum(CardEnum::Townhall);

	// First loop, only the resources we have for selling, sorted by amount...
	std::vector<ResourcePair> availableForSellingPairs;
	for (ResourceInfo info : ResourceInfos) {
		int32 count = resourceSystem.resourceCount(info.resourceEnum);
		if (IsTradeResource(info.resourceEnum) && count > 0)
		{
			availableForSellingPairs.push_back(ResourcePair(info.resourceEnum, count));
		}
	}

	std::sort(availableForSellingPairs.begin(), availableForSellingPairs.end(), [&](ResourcePair a, ResourcePair b) {
		return a.count > b.count;
	});

	for (ResourcePair pair : availableForSellingPairs) {
		auto tradeRow = AddWidget<UWorldTradeRow>(UIEnum::TradeRow);
		tradeRow->Init(this, pair.resourceEnum);
		tradeRow->RefreshSellOnlyState(isSellOnly);
		WorldTradeRowBox->AddChild(tradeRow);
	}

	BuyAmountTitle->SetVisibility(isSellOnly ? ESlateVisibility::Hidden : ESlateVisibility::Visible);

	
	if (!isSellOnly)
	{
		// Second loop, all other resources
		for (const ResourceInfo& info : SortedNameResourceInfo)
		{
			if (IsTradeResource(info.resourceEnum) &&
				resourceSystem.resourceCount(info.resourceEnum) == 0)
			{
				auto tradeRow = AddWidget<UWorldTradeRow>(UIEnum::TradeRow);
				tradeRow->Init(this, info.resourceEnum);
				WorldTradeRowBox->AddChild(tradeRow);
			}
		}
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	TickUI();
	UpdateTotal();

	dataSource()->Spawn2DSound("UI", "UIWindowOpen");
}

void UWorldTradeUI::TickUI()
{
	if (GetVisibility() != ESlateVisibility::SelfHitTestInvisible) {
		return;
	}

	auto& sim = simulation();
	ResourceSystem& resourceSys = sim.resourceSystem(worldTradeUITownId());

	Building& tradeBuilding = sim.building(punId);
	bool isSellOnly = tradeBuilding.isEnum(CardEnum::Townhall);

	int32 maxTradeQuantity = tradeBuilding.maxTradeQuatity() - _quantity;

	{
		FString searchString = SearchBox->GetText().ToString();
		searchString = searchString.Replace(TEXT(" "), TEXT(""));

		
		// Recalculate trade amount as resourceCount changes
		TArray<UWidget*> tradeRows = WorldTradeRowBox->GetAllChildren();
		bool amountUpdated = false;
		for (int32 i = 0; i < tradeRows.Num(); i++)
		{
			auto tradeRow = CastChecked<UWorldTradeRow>(tradeRows[i]);
			int32 resourceCount = resourceSys.resourceCount(tradeRow->resourceEnum());

			// Can't sell more than resourceCount
			if (tradeRow->buyAmount() < -resourceCount) {
				tradeRow->SetBuyAmount(-resourceCount);
				amountUpdated = true;
			}

			// update text if inventory changed
			tradeRow->inventory = resourceCount;
			tradeRow->UpdateTexts(); // Always updating tooltip

			tradeRow->PunTradeAmount->isUsingSpecialControl = true;
			tradeRow->PunTradeAmount->ctrlClickIncrementAmount = maxTradeQuantity;
			tradeRow->PunTradeAmount->ctrlClickDecrementAmount = maxTradeQuantity;
			
			
			// If this is a sellonly don't show left Arrow if the amount is 0 or more
			tradeRow->RefreshSellOnlyState(isSellOnly);

			// Hide if it is not searched...
			FString resourceName = ResourceNameF(tradeRow->resourceEnum());
			resourceName = resourceName.Replace(TEXT(" "), TEXT(""));

			if (searchString.IsEmpty() ||
				resourceName.Find(searchString, ESearchCase::Type::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE) 
			{
				tradeRow->SetVisibility(ESlateVisibility::Visible);
			} else {
				tradeRow->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		if (amountUpdated) {
			UpdateTotal();
		}
	}
}

void UWorldTradeUI::UpdateTotal()
{
	auto& worldTradeSystem = simulation().worldTradeSystem();
	Building& tradeBuilding = simulation().building(punId);

	int32 tradeFeePercent = tradeBuilding.tradingFeePercent();
	int32 maxQuantity = tradeBuilding.maxTradeQuatity();

	int32 buyMoneyBeforeFee = 0;
	int32 sellMoneyBeforeFee = 0;
	int32 buyMoneyAfterFee = 0;
	int32 sellMoneyAfterFee = 0;
	_quantity = 0;
	int32 totalFee100 = 0;
	TArray<UWidget*> tradeRows = WorldTradeRowBox->GetAllChildren();
	
	for (int32 i = 0; i < tradeRows.Num(); i++) 
	{
		auto tradeRow = CastChecked<UWorldTradeRow>(tradeRows[i]);

		int32 tradeMoney100BeforeFee = tradeRow->buyAmount() * worldTradeSystem.price100(tradeRow->resourceEnum());
		int32 tradeFee100 = abs(tradeMoney100BeforeFee) * tradeFeePercent / 100;
		int32 tradeMoney100AfterFee = tradeMoney100BeforeFee + tradeFee100;

		int32 tradeMoneyGainBeforeFee = -tradeMoney100BeforeFee / 100;
		int32 tradeMoneyGainAfterFee = -tradeMoney100AfterFee / 100;
		if (tradeMoneyGainAfterFee > 0) {
			buyMoneyBeforeFee += tradeMoneyGainBeforeFee;
			buyMoneyAfterFee += tradeMoneyGainAfterFee;
		} else {
			sellMoneyBeforeFee += tradeMoneyGainBeforeFee;
			sellMoneyAfterFee += tradeMoneyGainAfterFee;
		}
		
		totalFee100 += tradeFee100;
		_quantity += abs(tradeRow->buyAmount());

		//tradeRow->PunTradeAmount->UpdateText();
		tradeRow->UpdateEditableBoxTexts();
		tradeRow->UpdateTexts();
	}

	_totalCoinGain = sellMoneyAfterFee + buyMoneyAfterFee;

	/*
	 * 
	 */
	
	SetFString(QuantityText, FString::FromInt(_quantity) + "/" + FString::FromInt(maxQuantity));
	QuantityText->SetColorAndOpacity(_quantity <= maxQuantity ? FLinearColor::White : FLinearColor::Red);

	SetText(FeePercentText, to_string(tradeFeePercent) + "%");

	BuyMoney->SetImage(assetLoader()->CoinIcon);
	BuyMoney->SetFString(FString::FromInt(abs(-buyMoneyBeforeFee)), "");

	SellMoney->SetImage(assetLoader()->CoinIcon);
	SellMoney->SetFString(FString::FromInt(abs(sellMoneyBeforeFee)), "");

	FeeTextPair->SetImage(assetLoader()->CoinIcon);
	FeeTextPair->SetFString(FString::FromInt(abs(totalFee100 / 100)), "");

	TotalTextPair->SetImage(assetLoader()->CoinIcon);
	TotalTextPair->SetFString(_totalCoinGain < 0 ? "-" : "" ,FString::FromInt(abs(_totalCoinGain)));
}

void UWorldTradeUI::ClickedTradeButton()
{
	GlobalResourceSystem& globalResourceSys = dataSource()->simulation().globalResourceSystem(playerId());

	if (_quantity == 0) {
		simulation().AddPopupToFront(playerId(), 
			LOCTEXT("Need to specify resources and amount.", "Need to specify resources and amount."),
			ExclusiveUIEnum::Trading, "PopupCannot"
		);
		return;
	}
	
	if (_totalCoinGain < 0 &&
		_totalCoinGain + globalResourceSys.money() < 0)
	{
		simulation().AddPopupToFront(playerId(), 
			LOCTEXT("Not enough money for trade.", "Not enough money for trade."),
			ExclusiveUIEnum::Trading, "PopupCannot"
		);
		return;
	}

	Building& tradeBuilding = simulation().building(punId);
	
	if (_quantity > tradeBuilding.maxTradeQuatity()) {
		simulation().AddPopupToFront(playerId(), 
			LOCTEXT("ExceedTradeQuantity", "Exceed trade quantity available for this trading post."),
			ExclusiveUIEnum::Trading, "PopupCannot"
		);
		return;
	}

	auto tradeCommand = make_shared<FTradeResource>();

	TArray<UWidget*> tradeRows = WorldTradeRowBox->GetAllChildren();
	for (int32 i = 0; i < tradeRows.Num(); i++) {
		auto tradeRow = CastChecked<UWorldTradeRow>(tradeRows[i]);
		if (tradeRow->buyAmount() != 0) {
			tradeCommand->buyEnums.Add(static_cast<uint8>(tradeRow->resourceEnum()));
			tradeCommand->buyAmounts.Add(tradeRow->buyAmount());
		}
	}

	// Townhall special case, Yearly Buyer do now allow buy
	if (tradeBuilding.isEnum(CardEnum::Townhall))
	{
		for (int32 i = 0; i < tradeCommand->buyAmounts.Num(); i++) {
			if (tradeCommand->buyAmounts[i] > 0) {
				simulation().AddPopupToFront(playerId(), 
					LOCTEXT("CaravanNoGoodsToSell_Pop", "Caravan doesn't have any goods to sell to you. You can only sell to them. Please adjust the trade accordingly."), 
					ExclusiveUIEnum::Trading, "PopupCannot"
				);
				return;
			}
		}
	}
	

	// Change money from trade's Total
	tradeCommand->totalGain = _totalCoinGain;
	_totalCoinGain = 0;

	tradeCommand->objectId = punId;

	networkInterface()->SendNetworkCommand(tradeCommand);

	this->SetVisibility(ESlateVisibility::Collapsed);

	dataSource()->Spawn2DSound("UI", "TradeAction");
}


#undef LOCTEXT_NAMESPACE