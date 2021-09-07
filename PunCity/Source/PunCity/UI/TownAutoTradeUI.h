// Pun Dumnernchanvanit's

#pragma once

#include "PunBoxWidget.h"
#include "TownAutoTradeRow.h"
#include "TownTradeOffersRow.h"

#include "TownAutoTradeUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTownAutoTradeUI : public UPunWidget
{
	GENERATED_BODY()
public:

	void PunInit()
	{	
		CloseButton->OnClicked.AddUniqueDynamic(this, &UTownAutoTradeUI::CloseUI);
		CloseXButton->CoreButton->OnClicked.AddUniqueDynamic(this, &UTownAutoTradeUI::CloseUI);

		SetChildHUD(ChooseResourceBox);
		ChooseResourceCloseButton->OnClicked.AddUniqueDynamic(this, &UTownAutoTradeUI::CloseChooseResourceUI);
		ChooseResourceCloseXButton->CoreButton->OnClicked.AddUniqueDynamic(this, &UTownAutoTradeUI::CloseChooseResourceUI);

		AddAutoExportOrderButton->OnClicked.AddUniqueDynamic(this, &UTownAutoTradeUI::UTownAutoTradeUI::OnClickAddAutoExportOrder);
		AddAutoImportOrderButton->OnClicked.AddUniqueDynamic(this, &UTownAutoTradeUI::UTownAutoTradeUI::OnClickAddAutoImportOrder);

		CloseUI();
	}

	void OpenUI(int32 townIdIn)
	{
		uiTownId = townIdIn;
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		AutoExportRowsBox->ClearChildren();
		AutoImportRowsBox->ClearChildren();

		ChooseResourceOverlay->SetVisibility(ESlateVisibility::Collapsed);
		
		CitiesTradeOffersBox->ClearChildren();

		justOpenedUI = true;
	}
	
	void TickUI();

	UFUNCTION() void CloseUI() {
		SetVisibility(ESlateVisibility::Collapsed);
		AutoExportRowsBox->ClearChildren();
		AutoImportRowsBox->ClearChildren();
	}

	UFUNCTION() void OnClickAddAutoExportOrder() {
		ChooseResourceOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		chooseResourceForExport = true;
	}
	UFUNCTION() void OnClickAddAutoImportOrder() {
		ChooseResourceOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		chooseResourceForExport = false;
	}

	UFUNCTION() void CloseChooseResourceUI() {
		ChooseResourceOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	virtual void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = callbackEnum;
		command->townId = uiTownId;
		
		if (callbackEnum == CallbackEnum::EstablishTradeRoute ||
			callbackEnum == CallbackEnum::CancelTradeRoute)
		{
			command->intVar1 = punWidgetCaller->callbackVar1;
			command->intVar2 = punWidgetCaller->callbackVar2;

			networkInterface()->SendNetworkCommand(command);
			return;
		}
	
		// Only Major towns can have the Auto Trade UI
		TownManager& townManager = simulation().townManager(uiTownId);

		bool isImport = true;
		ResourceEnum resourceEnum = ResourceEnum::None;
		int32 targetInventory = 0;
		int32 maxTradeAmount = 0;
		
		if (callbackEnum == CallbackEnum::AddAutoTradeResource)
		{
			auto chooseButton = CastChecked<UChooseResourceElement>(punWidgetCaller);
			isImport = chooseResourceForExport;
			resourceEnum = chooseButton->resourceEnum;

			// Ensure it is not already added
			if (townManager.HasAutoTradeResource(resourceEnum)) {
				simulation().AddPopupToFront(playerId(), 
					NSLOCTEXT("TownAutoTradeUI", "TownAutoTradeUI_alreadyHasResource", "Resource already added for auto-trade."), 
					ExclusiveUIEnum::TownAutoTradeUI, "PopupCannot"
				);
				return;
			}
		}
		else if (IsAutoTradeCallback(callbackEnum))
		{
			auto autoTradeRow = CastChecked<UTownAutoTradeRow>(punWidgetCaller);

			isImport = autoTradeRow->isExport;
			resourceEnum = autoTradeRow->resourceEnum;
			targetInventory = autoTradeRow->TargetInventoryEditableNumber->amount;
			maxTradeAmount = autoTradeRow->MaxTradeAmountEditableNumber->amount;
		}
		else {
			UE_DEBUG_BREAK();
		}

		command->intVar1 = isImport;
		command->intVar2 = static_cast<int32>(resourceEnum);
		command->intVar3 = targetInventory;
		command->intVar4 = maxTradeAmount;

		townManager.AddPendingAutoTradeCommand(*command);
		networkInterface()->SendNetworkCommand(command);

		CloseChooseResourceUI();
	}



	UPROPERTY(meta = (BindWidget)) URichTextBlock* LastRoundProfitText;

	UPROPERTY(meta = (BindWidget)) UTextBlock* CityMaxAutoTradeAmount;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* FeeDiscountText;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* AutoExportRowsBox;
	UPROPERTY(meta = (BindWidget)) UButton* AddAutoExportOrderButton;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* AutoImportRowsBox;
	UPROPERTY(meta = (BindWidget)) UButton* AddAutoImportOrderButton;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* ExportMoneyText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ImportMoneyText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* FeeText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* NetTotalText;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* ExportMoneyLeftText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ImportMoneyLeftText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* FeeLeftText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* NetTotalLeftText;

	UPROPERTY(meta = (BindWidget)) UScrollBox* CitiesTradeOffersBox;
	

	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* CloseXButton;

	UPROPERTY(meta = (BindWidget)) UOverlay* ChooseResourceOverlay;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* ChooseResourceSearchBox;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* ChooseResourceBox;
	UPROPERTY(meta = (BindWidget)) UButton* ChooseResourceCloseButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* ChooseResourceCloseXButton;
	bool chooseResourceForExport = true;

	int32 uiTownId = -1;

	bool justOpenedUI = false;
};
