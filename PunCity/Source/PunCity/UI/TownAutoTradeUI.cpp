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
		TownManager& townManager = sim.townManager(uiTownId);

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

				if (autoTradeRow->uiHash != hash) {
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

						if (!networkInterface()->HasUserFocus(numberBox->EditableNumber)) {
							numberBox->UpdateText();
						}
					};

					updateNumberBox(autoTradeRow->TargetInventoryEditableNumber, targetInventory, CallbackEnum::AutoTradeRowTargetInventoryChanged);
					updateNumberBox(autoTradeRow->MaxTradeAmountEditableNumber, maxTradeAmount, CallbackEnum::AutoTradeRowMaxTradeAmountChanged);

					autoTradeRow->RoundTradeAmount->SetText(FText::AsNumber(tradeAmountNextRound));

					totalTradeAmount += tradeAmountNextRound;
				}

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
	{
		int32 index = 0;

		// TODO: Show tradable minor cities

		sim.ExecuteOnAllTowns([&](int32 townId)
		{
			if (townId == uiTownId) {
				return;
			}
			
			auto& townManager = sim.townManager(townId);

			const std::vector<AutoTradeElement>& autoExportElements = townManager.autoExportElements();
			const std::vector<AutoTradeElement>& autoImportElements = townManager.autoImportElements();


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
						if (element.calculatedFulfillmentLeft() > 0)
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
								iconPair->SetText(FText(), TEXT_NUM(element.calculatedFulfillmentLeft()));
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
			tradeRoute.townId1 = townId;
			tradeRoute.townId2 = uiTownId;
			worldTradeSys.CalculateTradeRouteBuildings(tradeRoute);

			bool hasTradeRoute = worldTradeSys.HasTradeRoute(tradeRoute);
			bool canCreateTradeRoute = !hasTradeRoute && sim.IsResearched(sim.townPlayerId(uiTownId), TechEnum::IntercityRoad);
			tradeOffersRow->EstablishTradeRouteButton->SetVisibility(canCreateTradeRoute ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			tradeOffersRow->TradeRouteCloseXButton->SetVisibility(hasTradeRoute ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			tradeOffersRow->TradeRouteConnectedText->SetVisibility(hasTradeRoute ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

			tradeOffersRow->callbackParent = this;
			tradeOffersRow->callbackVar1 = tradeRoute.townId1;
			tradeOffersRow->callbackVar2 = tradeRoute.buildingId1;
			tradeOffersRow->callbackVar3 = tradeRoute.townId2;
			tradeOffersRow->callbackVar4 = tradeRoute.buildingId2;
		});

		BoxAfterAdd(CitiesTradeOffersBox, index);
	}

	/*
	 * Money
	 */
	{
		auto& townManager = sim.townManager(uiTownId);

		const std::vector<AutoTradeElement>& autoExportElements = townManager.autoExportElements();
		const std::vector<AutoTradeElement>& autoImportElements = townManager.autoImportElements();

		// Check all Trade Route to find any fee discount
		// - Do this separately with TradeRoutePair so we can show which TradeRoute is giving the discount
		std::vector<ResourcePair> directExportResources;
		std::vector<ResourcePair> directImportResources;

		auto addResource = [&](std::vector<ResourcePair>& vec, ResourcePair pair)
		{
			for (int32 i = 0; i < vec.size(); i++) {
				if (vec[i].resourceEnum == pair.resourceEnum) {
					vec[i].count += pair.count;
					return;
				}
			}
			vec.push_back(pair);
		};

		auto findDirectResource = [&](std::vector<ResourcePair>& vec, ResourceEnum resourceEnum){
			for (int32 i = 0; i < vec.size(); i++) {
				if (vec[i].resourceEnum == resourceEnum) {
					return vec[i].count;
				}
			}
			return 0;
		};

		TArray<FText> feeDiscountTooltipText;
		
		const std::vector<TradeRoutePair>& tradeRoutePairs = worldTradeSys.tradeRoutePairs();
		for (const TradeRoutePair& tradeRoutePair : tradeRoutePairs)
		{
			if (tradeRoutePair.HasTownId(uiTownId))
			{
				for (const TradeRouteResourcePair& tradeResourcePair : tradeRoutePair.tradeResources)
				{
					bool isExport = (tradeRoutePair.townId1 == uiTownId && tradeResourcePair.isTown1ToTown2) ||
									(tradeRoutePair.townId2 == uiTownId && !tradeResourcePair.isTown1ToTown2);

					ResourcePair pair = tradeResourcePair.resourcePair;
					
					if (isExport) {
						addResource(directExportResources, pair);
					} else {
						addResource(directImportResources, pair);
					}

					// feeDiscountTooltipText
					int32 feeDiscount = pair.count * worldTradeSys.price100(pair.resourceEnum);
					if (feeDiscount > 0) {
						FText mainText = isExport ?
							LOCTEXT("feeDiscountTooltipText_Export", "  {0}<img id=\"Coin\"/> from Exporting {1} {2} to {3}") :
							LOCTEXT("feeDiscountTooltipText_Import", "  {0}<img id=\"Coin\"/> from Importing {1} {2} to {3}");
						
						feeDiscountTooltipText.Add(FText::Format(
							mainText,
							TEXT_NUM(feeDiscount),
							TEXT_NUM(pair.count),
							GetResourceInfo(pair.resourceEnum).name,
							sim.townNameT(uiTownId)
						));
					}
				}
			}
		}

		int32 exportMoneyTotal = 0;
		int32 importMoneyTotal = 0;
		int32 feeTotal = 0;
		int32 feeDiscountTotal = 0;
		
		TArray<FText> exportTooltipText;
		TArray<FText> importTooltipText;
		TArray<FText> feeTooltipText;

		const int32 feePercent = 40; // TODO: allow people to manipulate this by building more Trading Company etc.

		for (const AutoTradeElement& exportElement : autoExportElements) 
		{
			int32 price100 = worldTradeSys.price100(exportElement.resourceEnum);
			int32 exportMoney = exportElement.calculatedTradeAmountNextRound * price100;

			FText resourceName = GetResourceInfo(exportElement.resourceEnum).GetName();
			
			exportMoneyTotal += exportMoney;
			exportTooltipText.Add(FText::Format(
				LOCTEXT("exportTooltipText_Elem", "  {0}<img id=\"Coin\"/> from {1}"), TEXT_NUM(exportMoney), resourceName
			));

			int32 directTradeAmount = findDirectResource(directExportResources, exportElement.resourceEnum);
			
			int32 feeDiscount = directTradeAmount * price100 * feePercent / 100;
			int32 fee = exportMoney - feeDiscount;

			feeDiscountTotal += feeDiscount;
			feeTotal += fee;

			if (fee > 0) {
				feeTooltipText.Add(FText::Format(
					INVTEXT("  {0}<img id=\"Coin\"/> from {1}"), TEXT_NUM(fee), resourceName
				));
			}
		}
		
		for (const AutoTradeElement& importElement : autoImportElements) 
		{
			int32 price100 = worldTradeSys.price100(importElement.resourceEnum);
			int32 importMoney = importElement.calculatedTradeAmountNextRound * price100;

			FText resourceName = GetResourceInfo(importElement.resourceEnum).GetName();

			importMoneyTotal += importMoney;
			importTooltipText.Add(FText::Format(
				LOCTEXT("importTooltipText_Elem", "  {0}<img id=\"Coin\"/> from {1}"), TEXT_NUM(importMoney), resourceName
			));

			int32 directTradeAmount = findDirectResource(directImportResources, importElement.resourceEnum);

			int32 feeDiscount = directTradeAmount * price100 * feePercent / 100;
			int32 fee = importMoney - feeDiscount;

			feeDiscountTotal += feeDiscount;
			feeTotal += fee;

			if (fee > 0) {
				feeTooltipText.Add(FText::Format(
					INVTEXT("  {0}<img id=\"Coin\"/> from {1}"), TEXT_NUM(fee), resourceName
				));
			}
		}

		// Fee Discount
		if (feeDiscountTotal > 0) {
			FeeDiscountText->SetText(FText::Format(
				LOCTEXT("FeeDiscountText", "Fee Discount: <img id=\"Coin\"/>{0}"), TEXT_NUM(feeDiscountTotal)
			));
			
			feeDiscountTooltipText.Insert(FText::Format(
				LOCTEXT("feeTooltipText", "Fee Discount from fulfilling other City's Trade Offers<space>Fee Discount: <img id=\"Coin\"/>{0}"), TEXT_NUM(feeDiscountTotal)
			), 0);
			AddToolTip(FeeDiscountText, feeDiscountTooltipText);
			
			FeeDiscountText->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			FeeDiscountText->SetVisibility(ESlateVisibility::Collapsed);
		}

		
		// Export Money
		ExportMoneyText->SetText(FText::Format(
			INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_NUM(exportMoneyTotal))
		);
		exportTooltipText.Insert(FText::Format(
			LOCTEXT("exportTooltipText", "Export: <img id=\"Coin\"/>{0}"), TEXT_NUM(exportMoneyTotal)
		), 0);
		AddToolTip(ExportMoneyText, exportTooltipText);

		
		// Import Money
		ImportMoneyText->SetText(FText::Format(
			INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_NUM(importMoneyTotal))
		);
		importTooltipText.Insert(FText::Format(
			LOCTEXT("importTooltipText", "Import: <img id=\"Coin\"/>{0}"), TEXT_NUM(importMoneyTotal)
		), 0);
		AddToolTip(ImportMoneyText, importTooltipText);
		

		// Fee
		FeeText->SetText(FText::Format(
			INVTEXT("<img id=\"Coin\"/>{0} ({1}%)"), TEXT_NUM(feeTotal), TEXT_NUM(feePercent))
		);
		feeTooltipText.Insert(FText::Format(
			LOCTEXT("feeTooltipText", "Fee: <img id=\"Coin\"/>{0} ({1}%)"), TEXT_NUM(feeTotal), TEXT_NUM(feePercent)
		), 0);
		AddToolTip(FeeText, feeTooltipText);
		

		// Net Total
		NetTotalText->SetText(FText::Format(
			INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_NUM(feeTotal), TEXT_NUM(feePercent))
		);
		AddToolTip(FeeText, LOCTEXT("NetTotal_Tip", "Net Profit per Round"));


		
	}

}

#undef LOCTEXT_NAMESPACE 