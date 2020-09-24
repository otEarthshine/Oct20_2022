// Pun Dumnernchanvanit's


#include "StatisticsUI.h"
#include "ResourceStatTableRow.h"
#include "BuildingStatTableRow.h"

void UStatisticsUI::InitStatisticsUI()
{
	StatisticsCloseButton->OnClicked.AddDynamic(this, &UStatisticsUI::CloseStatisticsUI);
	StatisticsXButton->OnClicked.AddDynamic(this, &UStatisticsUI::CloseStatisticsUI);

	lastRefreshBuildingStatBox = -INT32_MIN;


	/*
	 * Stats
	 */
	OverviewStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnOverviewStatButtonClick);
	BuildingsStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnBuildingsStatButtonClick);
	PopulationStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnPopulationStatButtonClick);
	IncomeStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnIncomeStatButtonClick);
	ScienceStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnScienceStatButtonClick);
	FoodFuelStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnFoodFuelStatButtonClick);
	ImportExportStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnImportExportStatButtonClick);
	MarketStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnMarketStatButtonClick);

	SetTabSelection(OverviewStatButton);

	BuildingsStatBox->ClearChildren();
	ResourceStatisticsBox->ClearChildren();

	AddSeries(PopulationGraph, {
		{ FString("Adult population"), PlotStatEnum::AdultPopulation, FLinearColor::Yellow },
		{ FString("Child population"), PlotStatEnum::ChildPopulation, FLinearColor::Blue },
		{ FString("Population"), PlotStatEnum::Population, FLinearColor::Green },
		});

	AddSeries(IncomeGraph, {
		{FString("Income"), PlotStatEnum::Income, FLinearColor::Green },
		{FString("Revenue"), PlotStatEnum::Revenue, FLinearColor::Blue },
		{FString("Expense"), PlotStatEnum::Expense, FLinearColor::Red },
		});
	AddSeries(ScienceGraph, { {FString("Science"), PlotStatEnum::Science, FLinearColor::Blue } });

	AddSeries(FoodFuelGraph, {
		{FString("Food"), PlotStatEnum::Food, FLinearColor::Green },
		{FString("Fuel"), PlotStatEnum::Fuel, FLinearColor::Yellow },
		});
	AddSeries(ImportExportGraph, {
		{FString("Trade balance"), PlotStatEnum::TradeBalance, FLinearColor::Green },
		{FString("Export"), PlotStatEnum::Export, FLinearColor::Blue },
		{FString("Import"), PlotStatEnum::Import, FLinearColor::Red },
		});

	MarketResourceDropdown->ClearOptions();
	for (int32 i = 0; i < ResourceEnumCount; i++) {
		MarketResourceDropdown->AddOption(ResourceNameF(static_cast<ResourceEnum>(i)));
	}
	MarketResourceDropdown->SetSelectedOption(ResourceNameF(ResourceEnum::Wood));
	MarketResourceDropdown->OnSelectionChanged.AddDynamic(this, &UStatisticsUI::OnMarketDropDownChanged);

	AddSeries(MarketPriceGraph, {
		{FString("Current Price"), PlotStatEnum::MarketPrice, FLinearColor(0.7, 1, 0.7) },
	});
}

void UStatisticsUI::TickUI()
{
	if (GetVisibility() == ESlateVisibility::Collapsed) {
		return;
	}
	
	SubStatSystem& statSystem = simulation().statSystem().playerStatSystem(uiPlayerId);

	/*
	 * Production VS Consumption
	 */
	 //ResourceStatTableRow
	{
		const std::vector<std::vector<int32>>& productionStats = statSystem.GetResourceStat(ResourceSeasonStatEnum::Production);
		const std::vector<std::vector<int32>>& consumptionStats = statSystem.GetResourceStat(ResourceSeasonStatEnum::Consumption);
		PUN_CHECK(productionStats.size() == ResourceEnumCount);
		PUN_CHECK(consumptionStats.size() == ResourceEnumCount);

		// Set the season text
		SetText(ResourceSeasonText0, Time::SeasonName((Time::Seasons() + 4 - 3) % 4));
		SetText(ResourceSeasonText1, Time::SeasonName((Time::Seasons() + 4 - 2) % 4));
		SetText(ResourceSeasonText2, Time::SeasonName((Time::Seasons() + 4 - 1) % 4));
		SetText(ResourceSeasonText3, Time::SeasonName((Time::Seasons() + 4 - 0) % 4));

		// Sort by max of either production or consumption
		struct SortStruct
		{
			ResourceEnum resourceEnum;
			std::vector<int32> productionStats;
			std::vector<int32> consumptionStats;
		};
		std::vector<SortStruct> sortedStats;

		for (int i = 0; i < ResourceEnumCount; i++) {
			sortedStats.push_back({ static_cast<ResourceEnum>(i), productionStats[i], consumptionStats[i] });
		}

		std::sort(sortedStats.begin(), sortedStats.end(),
			[&](const SortStruct& a, const SortStruct& b) -> bool
		{
			int32 sumProdA = CppUtils::Sum(a.productionStats);
			int32 sumProdB = CppUtils::Sum(b.productionStats);
			if (sumProdA == sumProdB) {
				return CppUtils::Sum(a.consumptionStats) > CppUtils::Sum(b.consumptionStats);
			}
			return sumProdA > sumProdB;
		});

		// This changes the arrangement from [Spring Summer Autumn Winter] to [Latest-3 Latest-2 Latest-1 Latest]
		// [0][1][2][3]
		// 0 .. productionstats [1][2][3][0]
		// 1 .. productionstats [2][3][0][1]
		// 2 .. productionstats [3][0][1][2]
		int32 seasonShift = Time::SeasonMod() + 1;

		int32 resourceStatIndex = 0;

		for (const SortStruct& stat : sortedStats)
		{
			check(stat.productionStats.size() == 4);
			check(stat.consumptionStats.size() == 4);

			// If any season has this resource... display the graph
			if (CppUtils::Sum(stat.productionStats) > 0 ||
				CppUtils::Sum(stat.consumptionStats) > 0 ||
				simulation().resourceCount(uiPlayerId, stat.resourceEnum) > 0)
			{
				auto row = GetBoxChild<UResourceStatTableRow>(ResourceStatisticsBox, resourceStatIndex, UIEnum::ResourceStatTableRow, true);

				SetText(row->ProductionSeason0, to_string(stat.productionStats[(0 + seasonShift) % 4]));
				SetText(row->ProductionSeason1, to_string(stat.productionStats[(1 + seasonShift) % 4]));
				SetText(row->ProductionSeason2, to_string(stat.productionStats[(2 + seasonShift) % 4]));
				SetText(row->ProductionSeason3, to_string(stat.productionStats[(3 + seasonShift) % 4]));
				SetText(row->ProductionTotal, to_string(CppUtils::Sum(stat.productionStats)));

				SetText(row->ConsumptionSeason0, to_string(stat.consumptionStats[(0 + seasonShift) % 4]));
				SetText(row->ConsumptionSeason1, to_string(stat.consumptionStats[(1 + seasonShift) % 4]));
				SetText(row->ConsumptionSeason2, to_string(stat.consumptionStats[(2 + seasonShift) % 4]));
				SetText(row->ConsumptionSeason3, to_string(stat.consumptionStats[(3 + seasonShift) % 4]));
				SetText(row->ConsumptionTotal, to_string(CppUtils::Sum(stat.consumptionStats)));

				SetResourceImage(row->ResourceImage, stat.resourceEnum, assetLoader());
				SetText(row->ResourceText, GetResourceInfo(stat.resourceEnum).name);
			}
		}

		BoxAfterAdd(ResourceStatisticsBox, resourceStatIndex);

		// Need refresh every 3 sec for Goto click to work...
		if (Time::Ticks() > lastRefreshBuildingStatBox + Time::TicksPerSecond * 3)
		{
			lastRefreshBuildingStatBox = Time::Ticks();


			const std::vector<std::vector<int32>>& jobBuildingEnumToIds = simulation().playerOwned(playerId()).jobBuildingEnumToIds();

			int32 buildingStatIndex = 0;

			auto addRows = [&](CardEnum cardEnum)
			{
				// Doesn't have the building...
				if (jobBuildingEnumToIds.size() <= static_cast<int>(cardEnum)) {
					return;
				}

				std::vector<int32> buildingIds = jobBuildingEnumToIds[static_cast<int>(cardEnum)];

				for (int32 buildingId : buildingIds)
				{
					Building& bld = simulation().building(buildingId);

					auto row = GetBoxChild<UBuildingStatTableRow>(BuildingsStatBox, buildingStatIndex, UIEnum::BuildingStatTableRow, true);
					//UBuildingStatTableRow* row = AddWidget<UBuildingStatTableRow>(UIEnum::BuildingStatTableRow);
					//buildingsBox->AddChild(row);

					SetText(row->BuildingName, GetBuildingInfo(cardEnum).name);
					SetText(row->Upkeep, to_string(bld.upkeep()) + "<img id=\"Coin\"/>");

					std::string allowedOccupants = to_string(bld.allowedOccupants());
					if (bld.allowedOccupants() < bld.maxOccupants()) {
						allowedOccupants = "<Yellow>" + allowedOccupants + "</>";
					}
					SetText(row->WorkForce, to_string(bld.occupantCount()) + "/" + allowedOccupants);


					// Buildings without a fixed produce()
					if (IsRanch(bld.buildingEnum()) ||
						bld.buildingEnum() == CardEnum::Farm ||
						bld.buildingEnum() == CardEnum::HuntingLodge ||
						bld.buildingEnum() == CardEnum::FruitGatherer)
					{
						std::vector<ResourcePair> pairs = bld.seasonalProductionPairs();

						// Need to show farm with 0 production so ppl won't be confused
						if (pairs.size() == 0) {
							if (bld.isEnum(CardEnum::Farm)) {
								pairs.push_back({ bld.product(), 0 });
							}
						}

						if (pairs.size() > 0) {
							row->Production1->SetImage(pairs[0].resourceEnum, assetLoader());
							row->Production1->SetText(to_string(pairs[0].count), "");
							row->Production1->SetVisibility(ESlateVisibility::HitTestInvisible);
						}
						else {
							row->Production1->SetVisibility(ESlateVisibility::Collapsed);
						}
						if (pairs.size() > 1) {
							row->Production2->SetImage(pairs[1].resourceEnum, assetLoader());
							row->Production2->SetText(to_string(pairs[1].count), "");
							row->Production2->SetVisibility(ESlateVisibility::HitTestInvisible);
						}
						else {
							row->Production2->SetVisibility(ESlateVisibility::Collapsed);
						}
					}
					else
					{
						if (bld.product() != ResourceEnum::None)
						{
							row->Production1->SetImage(bld.product(), assetLoader());
							row->Production1->SetText(to_string(bld.seasonalProduction()), "");
							row->Production1->SetVisibility(ESlateVisibility::HitTestInvisible);
						}
						else {
							row->Production1->SetVisibility(ESlateVisibility::Collapsed);
						}

						row->Production2->SetVisibility(ESlateVisibility::Collapsed);
					}

					{
						if (bld.hasInput1()) {
							row->Consumption1->SetImage(bld.input1(), assetLoader());
							row->Consumption1->SetText(to_string(bld.seasonalConsumption1()), "");
							row->Consumption1->SetVisibility(ESlateVisibility::HitTestInvisible);
						}
						else {
							row->Consumption1->SetVisibility(ESlateVisibility::Collapsed);
						}
						if (bld.hasInput2()) {
							row->Consumption2->SetImage(bld.input2(), assetLoader());
							row->Consumption2->SetText(to_string(bld.seasonalConsumption2()), "");
							row->Consumption2->SetVisibility(ESlateVisibility::HitTestInvisible);
						}
						else {
							row->Consumption2->SetVisibility(ESlateVisibility::Collapsed);
						}
					}

					row->SetBuildingId(buildingId);
				}
			};

			// Put the food buildings first.
			for (CardEnum foodBuilding : FoodBuildings) {
				addRows(foodBuilding);
			}

			// Then show the rest.
			for (int32 i = 0; i < BuildingEnumCount; i++)
			{
				CardEnum cardEnum = static_cast<CardEnum>(i);
				if (!IsAgricultureBuilding(cardEnum)) {
					addRows(cardEnum);
				}
			}

			BoxAfterAdd(BuildingsStatBox, buildingStatIndex);
		}

	}

	// MarketPrice
	{
		auto& worldTradeSystem = simulation().worldTradeSystem();
		ResourceEnum resourceEnum = worldTradeSystem.resourceEnumToShowStat;
		
		std::stringstream ss;
		ss << fixed << setprecision(2);
		ss << simulation().price100(resourceEnum) / 100.0f;
		SetText(MarketCurrentPrice, ss);

		ss << GetResourceInfo(resourceEnum).basePrice100() / 100.0f;
		SetText(MarketBasePrice, ss);
		ss.str("");

		int32 netSupplyChange = worldTradeSystem.GetNetPlayerSupplyChange(uiPlayerId, resourceEnum);
		SetText(NetExportText, netSupplyChange >= 0 ? "Net export (yearly):" : "Net import (yearly):");
		SetText(NetExportAmount, to_string(abs(netSupplyChange)));
		SetResourceImage(NetExportImage, resourceEnum, assetLoader());

		std::vector<std::pair<int32, int32>> mainImporters = worldTradeSystem.GetTwoMainTraders(resourceEnum, true);
		ss << "Main importers: ";
		for (size_t i = 0; i < mainImporters.size(); i++) {
			ss << simulation().playerName(mainImporters[i].first);
			if (i < mainImporters.size() - 1) ss << ", ";
		}
		SetText(MainImportersText, ss.str());
		ss.str("");

		std::vector<std::pair<int32, int32>> mainExporters = worldTradeSystem.GetTwoMainTraders(resourceEnum, false);
		ss << "Main exporters: ";
		for (size_t i = 0; i < mainExporters.size(); i++) {
			ss << simulation().playerName(mainExporters[i].first);
			if (i < mainExporters.size() - 1) ss << ", ";
		}
		SetText(MainExportersText, ss.str());
		ss.str("");

		std::stringstream boughtLeftSS;
		std::stringstream boughtRightSS;
		std::stringstream soldLeftSS;
		std::stringstream soldRightSS;
		const std::vector<PlayerSupplyChange>& supplyChanges = worldTradeSystem.GetSupplyChanges(resourceEnum);
		for (const PlayerSupplyChange& supplyChange : supplyChanges) {
			if (supplyChange.amount > 0) {
				soldLeftSS << simulation().playerName(supplyChange.playerId) << "\n";
				soldRightSS << supplyChange.amount << "\n";
			}
			else {
				boughtLeftSS << simulation().playerName(supplyChange.playerId) << "\n";
				boughtRightSS << supplyChange.amount << "\n";
			}
		}
		SetText(BoughtRecordLeftText, boughtLeftSS);
		SetText(BoughtRecordRightText, boughtRightSS);
		SetText(SoldRecordLeftText, soldLeftSS);
		SetText(SoldRecordRightText, soldRightSS);
	}
	
}