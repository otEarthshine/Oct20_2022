// Pun Dumnernchanvanit's

#pragma once

#include "IconTextPairWidget.h"
#include "PunEditableNumberBox.h"
#include "PunWidget.h"
#include "TradeDealResourceRow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTradeDealResourceRow : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* ResourcePair;

	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* TargetAmount;

	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* CloseXButton;

	UPROPERTY() UPunWidget* callbackParent;

	bool isLeft = false;
	ResourceEnum resourceEnum;

	void OnInit() override {
		SetChildHUD(TargetAmount);
		CloseXButton->CoreButton->OnClicked.AddUniqueDynamic(this, &UTradeDealResourceRow::OnClickCloseXButton);
	}

	UFUNCTION() void OnClickCloseXButton() {
		callbackParent->CallBack1(this, CallbackEnum::RemoveTradeDealResource);
	}
};
