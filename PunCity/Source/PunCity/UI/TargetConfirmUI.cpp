// Pun Dumnernchanvanit's


#include "TargetConfirmUI.h"

#define LOCTEXT_NAMESPACE "TargetConfirmUI"

void UTargetConfirmUI::OnClickConfirmButton()
{
	IntercityTradeOffer offer = simulation().worldTradeSystem().GetIntercityTradeOffer(simulation().building(townhallId).playerId(), resourceEnum);
	if (offer.offerEnum == IntercityTradeOfferEnum::None) {
		simulation().AddPopupToFront(playerId(),
			LOCTEXT("Offer no longer valid.", "Offer no longer valid."),
			ExclusiveUIEnum::TargetConfirm, "PopupCannot"
		);
		return;
	}

	int32 amount = TargetAmount->amount;

	if (amount <= 0) {
		simulation().AddPopupToFront(playerId(), 
			LOCTEXT("Invalid amount.", "Invalid amount."),
			ExclusiveUIEnum::TargetConfirm, "PopupCannot"
		);
		return;
	}

	if (offer.offerEnum == IntercityTradeOfferEnum::SellWhenAbove) {
		// This town is selling to you, ensure you have enough money to buy
		if (amount * simulation().price(resourceEnum) > simulation().money(playerId())) {
			simulation().AddPopupToFront(playerId(), 
				LOCTEXT("Not enough money for trade.", "Not enough money for trade."),
				ExclusiveUIEnum::TargetConfirm, "PopupCannot"
			);
			return;
		}
	}
	if (offer.offerEnum == IntercityTradeOfferEnum::BuyWhenBelow) {
		// This town is buying from you, ensure you have enough resource
		if (amount > simulation().resourceCount(playerId(), resourceEnum)) {
			simulation().AddPopupToFront(playerId(), 
				LOCTEXT("Not enough resource for trade.", "Not enough resource for trade."),
				ExclusiveUIEnum::TargetConfirm, "PopupCannot"
			);
			return;
		}
	}

	// Counter-party is selling, currentPlayer is buying
	int32 currentPlayerBuyAmount = (offer.offerEnum == IntercityTradeOfferEnum::SellWhenAbove) ? amount : -amount;

	auto tradeCommand = make_shared<FTradeResource>();
	tradeCommand->buyEnums.Add(static_cast<uint8>(resourceEnum));
	tradeCommand->objectId = townhallId;
	tradeCommand->isIntercityTrade = 1;

	// Current Player Trade Command
	tradeCommand->buyAmounts.Add(currentPlayerBuyAmount);
	networkInterface()->SendNetworkCommand(tradeCommand);

	// Townhall Player Trade Command
	tradeCommand->playerId = simulation().building(townhallId).playerId();
	tradeCommand->buyAmounts.Empty();
	tradeCommand->buyAmounts.Add(-currentPlayerBuyAmount);
	networkInterface()->SendNetworkCommand(tradeCommand);


	SetVisibility(ESlateVisibility::Collapsed);

	dataSource()->Spawn2DSound("UI", "TradeAction");
}


#undef LOCTEXT_NAMESPACE