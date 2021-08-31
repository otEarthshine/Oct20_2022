// Pun Dumnernchanvanit's

#pragma once

#include "IconTextPairWidget.h"
#include "PunEditableNumberBox.h"
#include "PunWidget.h"
#include "TownTradeOffersRow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTownTradeOffersRow : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* TownName;

	UPROPERTY(meta = (BindWidget)) UTextBlock* ExportText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ImportText;
	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ExportBox;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ImportBox;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* TradeRouteTradingBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TradeRouteConnectedText;
	UPROPERTY(meta = (BindWidget)) UButton* EstablishTradeRouteButton;
	UPROPERTY(meta = (BindWidget)) UButton* TradeRouteCloseXButton;

	UPROPERTY() UPunWidget* callbackParent;

	virtual void OnInit() override
	{
		ExportBox->ClearChildren();
		ImportBox->ClearChildren();
		
		EstablishTradeRouteButton->OnClicked.AddUniqueDynamic(this, &UTownTradeOffersRow::OnClickEstablishTradeRouteButton);
		TradeRouteCloseXButton->OnClicked.AddUniqueDynamic(this, &UTownTradeOffersRow::OnClickTradeRouteCloseXButton);
	}


	UFUNCTION() void OnClickEstablishTradeRouteButton() {
		callbackParent->CallBack1(this, CallbackEnum::EstablishTradeRoute);
	}

	UFUNCTION() void OnClickTradeRouteCloseXButton() {
		callbackParent->CallBack1(this, CallbackEnum::CancelTradeRoute);
	}

	
};
