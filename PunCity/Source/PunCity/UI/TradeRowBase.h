// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "IconTextPairWidget.h"
#include "TradeRowBase.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTradeRowBase : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* ResourceTextPair;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* PriceTextPair;
	UPROPERTY(meta = (BindWidget)) UTextBlock* InventoryText;

	void UpdateTextsBase()
	{
		ResourceTextPair->SetImage(_resourceEnum, assetLoader());
		ResourceTextPair->SetFString("", FString(ResourceName(_resourceEnum).c_str()));

		int32 basePrice100 = GetResourceInfo(_resourceEnum).basePrice100();
		int32 price100 = simulation().worldTradeSystem().price100(_resourceEnum);
		PriceTextPair->SetImage(assetLoader()->CoinIcon);

		PriceTextPair->SetText(FText(), TEXT_100_2(price100));

		SetPriceColor(PriceTextPair, price100, basePrice100);


		AddResourceTooltip(ResourceTextPair, _resourceEnum, true);
		AddResourceTooltip(PriceTextPair, _resourceEnum, true);
	}

	ResourceEnum resourceEnum() { return _resourceEnum; }

public:
	int32 inventory = 0; //! Set by parent

protected:
	ResourceEnum _resourceEnum;
};
