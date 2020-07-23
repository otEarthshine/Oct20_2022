// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "PunCity/UI/PunBoxWidget.h"
#include "TargetConfirmUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTargetConfirmUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* TargetAmount;

	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* TitleIconPair;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TargetAmountMaxText;
	UPROPERTY(meta = (BindWidget)) UPunRichText* BottomText;
	
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmButton;
	UPROPERTY(meta = (BindWidget)) UButton* CancelButton;

	int32 townhallId = -1;
	ResourceEnum resourceEnum = ResourceEnum::None;

	void PunInit()
	{
		ConfirmButton->OnClicked.AddDynamic(this, &UTargetConfirmUI::OnClickConfirmButton);
		CancelButton->OnClicked.AddDynamic(this, &UTargetConfirmUI::OnClickCancelButton);

		SetVisibility(ESlateVisibility::Collapsed);
	}

	void TickUI()
	{
		if (townhallId == -1) {
			return;
		}
		
		int32 townPlayerId = simulation().building(townhallId).playerId();
		IntercityTradeOffer offer = simulation().worldTradeSystem().GetIntercityTradeOffer(townPlayerId, resourceEnum);

		if (offer.offerEnum == IntercityTradeOfferEnum::BuyWhenBelow)
		{
			TitleIconPair->SetText(simulation().townName(townhallId) + " is buying ", ".");
			TitleIconPair->SetImage(resourceEnum, assetLoader());

			int32 maxBuyAmount = offer.targetInventory - simulation().resourceCount(townPlayerId, resourceEnum);
			SetText(TargetAmountMaxText, "/ " + std::to_string(maxBuyAmount));
			TargetAmount->maxAmount = maxBuyAmount;

			BottomText->SetText("You receive: " + std::to_string(TargetAmount->amount) + "<img id=\"Coin\"/>");
		}
		else if (offer.offerEnum == IntercityTradeOfferEnum::SellWhenAbove)
		{
			TitleIconPair->SetText(simulation().townName(townhallId) + " is selling ", ".");
			TitleIconPair->SetImage(resourceEnum, assetLoader());

			int32 maxSellAmount = simulation().resourceCount(townPlayerId, resourceEnum) - offer.targetInventory;
			SetText(TargetAmountMaxText, "/ " + std::to_string(maxSellAmount));
			TargetAmount->maxAmount = maxSellAmount;

			BottomText->SetText("You pay: " + std::to_string(TargetAmount->amount) + "<img id=\"Coin\"/>");
		}
	}

	void OpenUI(int32 townhallIdIn, ResourceEnum resourceEnumIn)
	{
		townhallId = townhallIdIn;
		resourceEnum = resourceEnumIn;
		
		TargetAmount->Set(this, CallbackEnum::None);
		TargetAmount->amount = 0;
		TargetAmount->minAmount = 0;
		TargetAmount->UpdateText();
	}

	void CloseUI()
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}


	UFUNCTION() void OnClickConfirmButton()
	{
		IntercityTradeOffer offer = simulation().worldTradeSystem().GetIntercityTradeOffer(simulation().building(townhallId).playerId(), resourceEnum);
		if (offer.offerEnum == IntercityTradeOfferEnum::None) {
			simulation().AddPopupToFront(playerId(), "Offer no longer valid.", ExclusiveUIEnum::TargetConfirm, "PopupCannot");
			return;
		}
		
		int32 amount = TargetAmount->amount;
		
		if (amount <= 0) {
			simulation().AddPopupToFront(playerId(), "Invalid amount.", ExclusiveUIEnum::TargetConfirm, "PopupCannot");
			return;
		}

		if (offer.offerEnum == IntercityTradeOfferEnum::SellWhenAbove) {
			// This town is selling to you, ensure you have enough money to buy
			if (amount * simulation().price100(resourceEnum) > simulation().money(playerId())) {
				simulation().AddPopupToFront(playerId(), "Not enough money for trade.", ExclusiveUIEnum::Trading, "PopupCannot");
				return;
			}
		}
		if (offer.offerEnum == IntercityTradeOfferEnum::BuyWhenBelow) {
			// This town is buying from you, ensure you have enough resource
			if (amount > simulation().resourceCount(playerId(), resourceEnum)) {
				simulation().AddPopupToFront(playerId(), "Not enough resource for trade.", ExclusiveUIEnum::Trading, "PopupCannot");
				return;
			}
		}

		//
		int32 currentPlayerBuyAmount = (offer.offerEnum == IntercityTradeOfferEnum::SellWhenAbove) ? amount : -amount;
		
		auto tradeCommand = make_shared<FTradeResource>();
		tradeCommand->buyEnums.Add(static_cast<uint8>(resourceEnum));
		tradeCommand->objectId = townhallId;
		tradeCommand->isIntercityTrade = 1;
		
		// Current Player Trade Command
		tradeCommand->buyAmounts.Add(currentPlayerBuyAmount);
		networkInterface()->SendNetworkCommand(tradeCommand);

		// Townhall Player Trade Command
		tradeCommand->buyAmounts.Empty();
		tradeCommand->buyAmounts.Add(-currentPlayerBuyAmount);
		networkInterface()->SendNetworkCommand(tradeCommand);
		

		SetVisibility(ESlateVisibility::Collapsed);

		dataSource()->Spawn2DSound("UI", "TradeAction");
	}

	UFUNCTION() void OnClickCancelButton()
	{
		CloseUI();
	}
	
};
