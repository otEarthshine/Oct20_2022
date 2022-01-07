// Pun Dumnernchanvanit's


#include "TownAutoTradeUI.h"

#define LOCTEXT_NAMESPACE "TownAutoTradeUI"

void UTownAutoTradeUI::TickUI()
{
	if (!IsVisible()) {
		return;
	}
	check(uiTownId != -1);

	/*
	 * Fill choose resource box
	 */
	if (ChooseResourceOverlay->IsVisible())
	{
		FString searchString = ChooseResourceSearchBox->GetText().ToString();

		for (const ResourceInfo& info : SortedNameResourceInfo)
		{
			FString name = info.name.ToString();

			if (IsTradeResource(info.resourceEnum))
			{
				if (searchString.IsEmpty() ||
					name.Find(searchString, ESearchCase::Type::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE)
				{
					ChooseResourceBox->AddChooseResourceElement(info.resourceEnum, this, CallbackEnum::AddAutoTradeResource);
				}
			}

		}

		ChooseResourceBox->AfterAdd();
	}

	/*
	 * Prepare
	 */

	auto& sim = simulation();

	// Refresh AutoTrade Status
	sim.RefreshAutoTradeAmount();
	sim.RefreshAutoTradeFulfillment();

	WorldTradeSystem& worldTradeSys = sim.worldTradeSystem();


	/*
	 * Auto Trade Rows
	 */
	{
		TownManager& townManager = sim.townManager(uiTownId); // AutoTrade UI's townId can only be major town

		LastRoundProfitText->SetText(FText::Format(
			INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_100(townManager.lastRoundAutoTradeProfit())
		));

		std::vector<AutoTradeElement> autoExportElements;
		std::vector<AutoTradeElement> autoImportElements;
		townManager.GetAutoTradeElementsDisplay(autoExportElements, autoImportElements);
		townManager.CalculateAutoTradeAmountNextRound_Helper(autoExportElements, autoImportElements);

		int32 totalTradeAmount = 0;

		auto fillTradeRows = [&](UVerticalBox* AutoTradeRowsBox, std::vector<AutoTradeElement>& autoTradeElements, bool isExport)
		{
			int32 index = 0;
			for (const AutoTradeElement& autoTradeElement : autoTradeElements)
			{
				int32 thisIndex = index;
				auto autoTradeRow = GetBoxChild<UTownAutoTradeRow>(AutoTradeRowsBox, index, UIEnum::TownAutoTradeRow, true);

				autoTradeRow->ArrowUpButton->SetVisibility(thisIndex > 0 ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
				autoTradeRow->ArrowDownButton->SetVisibility(thisIndex < static_cast<int32>(autoTradeElements.size()) - 1 ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
				autoTradeRow->ArrowFastUpButton->SetVisibility(thisIndex > 1 ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
				autoTradeRow->ArrowFastDownButton->SetVisibility(thisIndex < static_cast<int32>(autoTradeElements.size()) - 2 ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
				autoTradeRow->CloseButton->SetVisibility(ESlateVisibility::Visible);

				// Resource/Price
				ResourceEnum resourceEnum = autoTradeElement.resourceEnum;
				int32 basePrice100 = GetResourceInfo(resourceEnum).basePrice100();
				int32 price100 = simulation().worldTradeSystem().price100(resourceEnum);
				int32 inventoryCount = simulation().resourceCountTown(uiTownId, resourceEnum);
				int32 targetInventory = autoTradeElement.targetInventory;
				int32 maxTradeAmount = autoTradeElement.maxTradeAmount;
				int32 tradeAmountNextRound = autoTradeElement.calculatedTradeAmountNextRound;

				//autoTradeElement.

				int32 hash = FastHashCombineMany({
					static_cast<int32>(resourceEnum),
					basePrice100,
					price100,
					inventoryCount,
					targetInventory,
					maxTradeAmount,
					tradeAmountNextRound,
				});

				// Refresh Texts
				if (justOpenedUI ||
					autoTradeRow->uiHash != hash)
				{
					autoTradeRow->uiHash = hash;

					autoTradeRow->resourceEnum = resourceEnum;
					autoTradeRow->isExport = isExport;
					autoTradeRow->parentWidget = this;

					autoTradeRow->ResourceTextPair->SetImage(resourceEnum, assetLoader());
					autoTradeRow->ResourceTextPair->SetText(FText(), ResourceNameT(resourceEnum));

					autoTradeRow->PriceTextPair->SetImage(assetLoader()->CoinIcon);
					autoTradeRow->PriceTextPair->SetText(FText(), TEXT_100_2(price100));
					SetPriceColor(autoTradeRow->PriceTextPair, price100, basePrice100);


					autoTradeRow->InventoryText->SetText(TEXT_NUM(inventoryCount));

					auto updateNumberBox = [&](UPunEditableNumberBox* numberBox, int32 amount, CallbackEnum callbackEnum)
					{
						numberBox->Set(this, callbackEnum);
						numberBox->resourceEnum = resourceEnum;
						numberBox->amount = amount;
						numberBox->callbackWidgetCaller = autoTradeRow;
						numberBox->minAmount = 0;
						numberBox->ctrlClickIncrementAmount = 100;
						numberBox->ctrlClickDecrementAmount = 100;

						if (!networkInterface()->HasUserFocus(numberBox->EditableNumber)) {
							numberBox->UpdateText();
						}
					};

					updateNumberBox(autoTradeRow->TargetInventoryEditableNumber, targetInventory, CallbackEnum::AutoTradeRowTargetInventoryChanged);
					updateNumberBox(autoTradeRow->MaxTradeAmountEditableNumber, maxTradeAmount, CallbackEnum::AutoTradeRowMaxTradeAmountChanged);

					autoTradeRow->RoundTradeAmount->SetText(FText::AsNumber(tradeAmountNextRound));
				}

				totalTradeAmount += tradeAmountNextRound;

				AddResourceTooltip(autoTradeRow->ResourceTextPair, resourceEnum, false);
				AddResourceTooltip(autoTradeRow->PriceTextPair, resourceEnum, false);
			}

			BoxAfterAdd(AutoTradeRowsBox, index);
		};

		fillTradeRows(AutoExportRowsBox, autoExportElements, true);
		fillTradeRows(AutoImportRowsBox, autoImportElements, false);

		// TotalMaxTrade
		int32 maxAutoTradeAmount = townManager.GetMaxAutoTradeAmount();
		CityMaxAutoTradeAmount->SetText(FText::Format(INVTEXT("{0}/{1}"),
			TEXT_NUM(totalTradeAmount),
			TEXT_NUM(maxAutoTradeAmount)
		));
	}

	/*
	 * Cities Trade Offers
	 * - Calculating BuyMoney Fee etc. is here since this is calculated with actual value (which is delayed)
	 */
	if (sim.IsResearched(playerId(), TechEnum::TradeRoute))
	{
		CitiesTradeOffersOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		
		int32 index = 0;

		// TODO: Show tradable minor cities

		sim.ExecuteOnAllTowns([&](int32 townId)
		{
			if (townId == uiTownId) {
				return;
			}
			
			TownManagerBase* townManagerBase = sim.townManagerBase(townId);

			const std::vector<AutoTradeElement>& autoExportElements = townManagerBase->autoExportElementsConst();
			const std::vector<AutoTradeElement>& autoImportElements = townManagerBase->autoImportElementsConst();


			int32 hash = townId;
			for (const AutoTradeElement& element : autoExportElements) {
				hash = FastHashCombine(hash, element.GetHash());
			}
			for (const AutoTradeElement& element : autoImportElements) {
				hash = FastHashCombine(hash, element.GetHash());
			}

			auto tradeOffersRow = GetBoxChild<UTownTradeOffersRow>(CitiesTradeOffersBox, index, UIEnum::TownTradeOffersRow, true);

			if (tradeOffersRow->uiHash != hash) 
			{
				tradeOffersRow->uiHash = hash;

				tradeOffersRow->TownName->SetText(sim.townNameT(townId));

				auto fillTradeBox = [&](UVerticalBox* box, const std::vector<AutoTradeElement>& elements, bool isExport)
				{
					int32 indexLocal = 0;
					for (const AutoTradeElement& element : elements)
					{
						if (element.calculatedTradeAmountNextRound > 0)
						{
							UIconTextPairWidget* iconPair = GetBoxChild<UIconTextPairWidget>(box, indexLocal, UIEnum::IconTextPair, true);
							iconPair->SetImage(element.resourceEnum, assetLoader(), false);

							if (element.calculatedFulfilledTradeAmountNextRound > 0) {
								iconPair->SetText(FText(), FText::Format(
									INVTEXT("{0} ({1})"),
									TEXT_NUM(element.calculatedFulfillmentLeft()),
									TEXT_NUM(element.calculatedTradeAmountNextRound)
								));
							}
							else {
								iconPair->SetText(FText(), TEXT_NUM(element.calculatedTradeAmountNextRound));
							}
						}
					}
					BoxAfterAdd(box, indexLocal);

					if (isExport) {
						tradeOffersRow->ExportText->SetText(
							indexLocal == 0 ?
							LOCTEXT("Export: None", "Export: None") :
							LOCTEXT("Export:", "Export:")
						);
					}
					else {
						tradeOffersRow->ImportText->SetText(
							indexLocal == 0 ?
							LOCTEXT("Import: None", "Import: None") :
							LOCTEXT("Import:", "Import:")
						);
					}
				};
				fillTradeBox(tradeOffersRow->ExportBox, autoExportElements, true);
				fillTradeBox(tradeOffersRow->ImportBox, autoImportElements, false);
			}

			// Show Trade Route Status
			TradeRoutePair tradeRoute;
			tradeRoute.townId1 = townId; // target
			tradeRoute.townId2 = playerId(); // self

			bool hasTradeRoute = worldTradeSys.HasTradeRoute(tradeRoute);
			bool canCreateTradeRoute = !hasTradeRoute && sim.IsResearched(sim.townPlayerId(uiTownId), TechEnum::TradeRoute);
			tradeOffersRow->EstablishTradeRouteButton->SetVisibility(canCreateTradeRoute ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			tradeOffersRow->TradeRouteCloseXButton->SetVisibility(hasTradeRoute ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			tradeOffersRow->TradeRouteConnectedText->SetVisibility(hasTradeRoute ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

			// CallbackEnum::EstablishTradeRoute
			tradeOffersRow->callbackParent = this;
			tradeOffersRow->callbackVar1 = tradeRoute.townId1; // target
			tradeOffersRow->callbackVar2 = tradeRoute.townId2; // self

			tradeOffersRow->tradeRoute = tradeRoute;
		});

		BoxAfterAdd(CitiesTradeOffersBox, index);
	}
	else {
		CitiesTradeOffersOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	/*
	 * Money
	 */
	{
		int32 exportMoneyTotal = 0;
		int32 importMoneyTotal = 0;
		int32 feeTotal = 0;
		int32 feeDiscountTotal = 0;
		
		TArray<FText> exportTooltipText;
		TArray<FText> importTooltipText;
		TArray<FText> feeTooltipText;
		TArray<FText> feeDiscountTooltipText;

		sim.townManager(uiTownId).CalculateAutoTradeProfit(
			exportMoneyTotal, 
			importMoneyTotal, 
			feeTotal, 
			feeDiscountTotal,

			exportTooltipText, 
			importTooltipText, 
			feeTooltipText, 
			feeDiscountTooltipText,
			true
		);
		
		

		// Fee Discount
		if (feeDiscountTotal > 0) {
			FeeDiscountText->SetText(FText::Format(
				LOCTEXT("FeeDiscountText", "Fee Discount Total: <img id=\"Coin\"/>{0}"), TEXT_100(feeDiscountTotal)
			));
			
			feeDiscountTooltipText.Insert(FText::Format(
				LOCTEXT("feeTooltipText", "Fee Discount from fulfilling other City's Trade Offers<space>Fee Discount: <img id=\"Coin\"/>{0}"), TEXT_100(feeDiscountTotal)
			), 0);
			AddToolTip(FeeDiscountText, feeDiscountTooltipText);
			
			FeeDiscountText->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			FeeDiscountText->SetVisibility(ESlateVisibility::Collapsed);
		}

		
		// Export Money
		ExportMoneyText->SetText(FText::Format(
			INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_100(exportMoneyTotal))
		);
		exportTooltipText.Insert(FText::Format(
			LOCTEXT("exportTooltipText", "Export: <img id=\"Coin\"/>{0}<space>"), TEXT_100(exportMoneyTotal)
		), 0);
		AddToolTip(ExportMoneyText, exportTooltipText, false);
		AddToolTip(ExportMoneyLeftText, exportTooltipText);

		
		// Import Money
		ImportMoneyText->SetText(FText::Format(
			INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_100(importMoneyTotal))
		);
		importTooltipText.Insert(FText::Format(
			LOCTEXT("importTooltipText", "Import: <img id=\"Coin\"/>{0}<space>"), TEXT_100(importMoneyTotal)
		), 0);
		AddToolTip(ImportMoneyText, importTooltipText, false);
		AddToolTip(ImportMoneyLeftText, importTooltipText);

		// Fee
		int32 tradingFeePercent = Building::baseTradingFeePercent(uiTownId, &sim, true);
		FeeText->SetText(FText::Format(
			INVTEXT("<img id=\"Coin\"/>{0} ({1}%)"), TEXT_100(feeTotal), TEXT_NUM(tradingFeePercent))
		);
		feeTooltipText.Insert(FText::Format(
			LOCTEXT("feeTooltipText", "Fee: <img id=\"Coin\"/>{0} ({1}%)<space>"), TEXT_100(feeTotal), TEXT_NUM(tradingFeePercent)
		), 0);
		
		AddToolTip(FeeText, feeTooltipText, false);
		AddToolTip(FeeLeftText, feeTooltipText);

		// Net Total
		int32 netTotal = exportMoneyTotal - importMoneyTotal - feeTotal;
		NetTotalText->SetText(FText::Format(
			INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_100(netTotal))
		);
		FText netTotalTip = LOCTEXT("NetTotal_Tip", "Net Profit per Round");
		AddToolTip(NetTotalText, netTotalTip);
		AddToolTip(NetTotalLeftText, netTotalTip);

		
	}

	justOpenedUI = false;
}

#undef LOCTEXT_NAMESPACE 