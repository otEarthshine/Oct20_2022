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

		Building& townhall = simulation().building(townhallId);
		int32 townId = townhall.townId();
		IntercityTradeOffer offer = simulation().worldTradeSystem().GetIntercityTradeOffer(townId, resourceEnum);

#define LOCTEXT_NAMESPACE "TargetConfirmUI"
		if (offer.offerEnum == IntercityTradeOfferEnum::BuyWhenBelow)
		{
			TitleIconPair->SetText(
				FText::Format(LOCTEXT("X is buying ", "{0} is buying "), simulation().townNameT(townId)),
				LOCTEXT("FullStop", ".")
			);
			TitleIconPair->SetImage(resourceEnum, assetLoader());

			int32 maxBuyAmount = offer.targetInventory - simulation().resourceCountTown(townId, resourceEnum);
			SetText(TargetAmountMaxText, FText::Format(INVTEXT("/ {0}"), TEXT_NUM(maxBuyAmount)));
			TargetAmount->maxAmount = maxBuyAmount;

			int32 moneyReceived = TargetAmount->amount * simulation().price(resourceEnum);
			BottomText->SetText(FText::Format(LOCTEXT("YouRecieveXCoin", "You receive: {0}<img id=\"Coin\"/>"), TEXT_NUM(moneyReceived)));
		}
		else if (offer.offerEnum == IntercityTradeOfferEnum::SellWhenAbove)
		{
			TitleIconPair->SetText(
				FText::Format(LOCTEXT("X is selling ", "{0} is selling "), simulation().townNameT(townId)),
				LOCTEXT("FullStop", ".")
			);
			TitleIconPair->SetImage(resourceEnum, assetLoader());

			int32 maxSellAmount = simulation().resourceCountTown(townId, resourceEnum) - offer.targetInventory;
			SetText(TargetAmountMaxText, FText::Format(INVTEXT("/ {0}"), TEXT_NUM(maxSellAmount)));
			TargetAmount->maxAmount = maxSellAmount;

			int32 moneyPaid = TargetAmount->amount * simulation().price(resourceEnum);
			BottomText->SetText(FText::Format(LOCTEXT("YouPayXCoin", "You pay: {0}<img id=\"Coin\"/>"), TEXT_NUM(moneyPaid)));
		}
#undef LOCTEXT_NAMESPACE
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
