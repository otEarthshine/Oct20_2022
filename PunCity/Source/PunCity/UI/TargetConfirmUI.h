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

		SetChildHUD(TargetAmount);

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
			TitleIconPair->SetText(simulation().townName(townPlayerId) + " is buying ", ".");
			TitleIconPair->SetImage(resourceEnum, assetLoader());

			int32 maxBuyAmount = offer.targetInventory - simulation().resourceCount(townPlayerId, resourceEnum);
			SetText(TargetAmountMaxText, "/ " + std::to_string(maxBuyAmount));
			TargetAmount->maxAmount = maxBuyAmount;

			BottomText->SetText("You receive: " + std::to_string(TargetAmount->amount * simulation().price(resourceEnum)) + "<img id=\"Coin\"/>");
		}
		else if (offer.offerEnum == IntercityTradeOfferEnum::SellWhenAbove)
		{
			TitleIconPair->SetText(simulation().townName(townPlayerId) + " is selling ", ".");
			TitleIconPair->SetImage(resourceEnum, assetLoader());

			int32 maxSellAmount = simulation().resourceCount(townPlayerId, resourceEnum) - offer.targetInventory;
			SetText(TargetAmountMaxText, "/ " + std::to_string(maxSellAmount));
			TargetAmount->maxAmount = maxSellAmount;

			BottomText->SetText("You pay: " + std::to_string(TargetAmount->amount * simulation().price(resourceEnum)) + "<img id=\"Coin\"/>");
		}
	}

	void OpenUI(int32 townhallIdIn, ResourceEnum resourceEnumIn)
	{
		_LOG(LogNetworkInput, "TargetConfirm OpenUI %d %s", townhallIdIn, ToTChar(ResourceName(resourceEnumIn)));
		
		townhallId = townhallIdIn;
		resourceEnum = resourceEnumIn;
		
		TargetAmount->Set(this, CallbackEnum::None);
		TargetAmount->amount = 0;
		TargetAmount->minAmount = 0;
		TargetAmount->UpdateText();

		SetVisibility(ESlateVisibility::Visible);
	}

	void CloseUI()
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}


	UFUNCTION() void OnClickConfirmButton();

	UFUNCTION() void OnClickCancelButton()
	{
		CloseUI();
	}
	
};
