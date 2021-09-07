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

	UPROPERTY(meta = (BindWidget)) UButton* TownZoomButton;
	UPROPERTY(meta = (BindWidget)) UImage* TownZoomImage;
	

	UPROPERTY() UPunWidget* callbackParent;
	TradeRoutePair tradeRoute;

	virtual void OnInit() override
	{
		ExportBox->ClearChildren();
		ImportBox->ClearChildren();
		
		EstablishTradeRouteButton->OnClicked.AddUniqueDynamic(this, &UTownTradeOffersRow::OnClickEstablishTradeRouteButton);
		TradeRouteCloseXButton->OnClicked.AddUniqueDynamic(this, &UTownTradeOffersRow::OnClickTradeRouteCloseXButton);

		TownZoomButton->OnHovered.AddUniqueDynamic(this, &UTownTradeOffersRow::OnHoveredTownZoomButton);
		TownZoomButton->OnUnhovered.AddUniqueDynamic(this, &UTownTradeOffersRow::OnUnhoveredTownZoomButton);
		TownZoomButton->OnClicked.AddUniqueDynamic(this, &UTownTradeOffersRow::OnClickTownZoomButton);
	}


	UFUNCTION() void OnClickEstablishTradeRouteButton() {
		callbackParent->CallBack1(this, CallbackEnum::EstablishTradeRoute);
	}

	UFUNCTION() void OnClickTradeRouteCloseXButton() {
		callbackParent->CallBack1(this, CallbackEnum::CancelTradeRoute);
	}

	UFUNCTION() void OnHoveredTownZoomButton() {
		TownZoomImage->GetDynamicMaterial()->SetScalarParameterValue("IsHovered", 1.0f);
	}
	UFUNCTION() void OnUnhoveredTownZoomButton() {
		TownZoomImage->GetDynamicMaterial()->SetScalarParameterValue("IsHovered", 0.0f);
	}

	UFUNCTION() void OnClickTownZoomButton() {
		networkInterface()->SetCameraToTown(tradeRoute.townId2);
	}

	

};
