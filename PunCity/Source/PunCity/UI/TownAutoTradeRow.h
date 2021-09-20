// Pun Dumnernchanvanit's

#pragma once

#include "IconTextPairWidget.h"
#include "PunEditableNumberBox.h"
#include "PunWidget.h"

#include "TownAutoTradeRow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTownAutoTradeRow : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* ResourceTextPair;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* PriceTextPair;

	UPROPERTY(meta = (BindWidget)) UTextBlock* InventoryText;
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* TargetInventoryEditableNumber;

	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* MaxTradeAmountEditableNumber;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RoundTradeAmount;

	UPROPERTY(meta = (BindWidget)) UButton* ArrowUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowDownButton;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowFastUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowFastDownButton;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;

	bool isExport;
	ResourceEnum resourceEnum;
	UPROPERTY() UPunWidget* parentWidget;

	virtual void OnInit() override
	{
		SetChildHUD(ResourceTextPair);
		SetChildHUD(PriceTextPair);
		
		SetChildHUD(TargetInventoryEditableNumber);
		SetChildHUD(MaxTradeAmountEditableNumber);

		BUTTON_ON_CLICK(ArrowUpButton, this, &UTownAutoTradeRow::OnClickedArrowUpButton);
		BUTTON_ON_CLICK(ArrowDownButton, this, &UTownAutoTradeRow::OnClickedArrowDownButton);
		BUTTON_ON_CLICK(ArrowFastUpButton, this, &UTownAutoTradeRow::OnClickedArrowFastUpButton);
		BUTTON_ON_CLICK(ArrowFastDownButton, this, &UTownAutoTradeRow::OnClickedArrowFastDownButton);
		BUTTON_ON_CLICK(CloseButton, this, &UTownAutoTradeRow::OnClickedArrowCloseXButton);
	}

	UFUNCTION() void OnClickedArrowUpButton() { parentWidget->CallBack1(this, CallbackEnum::ShiftAutoTradeRowUp); }
	UFUNCTION() void OnClickedArrowDownButton() { parentWidget->CallBack1(this, CallbackEnum::ShiftAutoTradeRowDown); }
	UFUNCTION() void OnClickedArrowFastUpButton() { parentWidget->CallBack1(this, CallbackEnum::ShiftAutoTradeRowFastUp); }
	UFUNCTION() void OnClickedArrowFastDownButton() { parentWidget->CallBack1(this, CallbackEnum::ShiftAutoTradeRowFastDown); }
	UFUNCTION() void OnClickedArrowCloseXButton() { parentWidget->CallBack1(this, CallbackEnum::RemoveAutoTradeRow); }

	
};
