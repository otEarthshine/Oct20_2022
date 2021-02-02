// Pun Dumnernchanvanit's


#include "StatisticsUI.h"
#include "ResourceStatTableRow.h"
#include "BuildingStatTableRow.h"

#define LOCTEXT_NAMESPACE "StatisticsUI"

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
	FoodUsageStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnFoodUsageStatButtonClick);
	ImportExportStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnImportExportStatButtonClick);
	MarketStatButton->OnClicked.AddDynamic(this, &UStatisticsUI::OnMarketStatButtonClick);

	SetTabSelection(OverviewStatButton);

	BuildingsStatBox->ClearChildren();
	ResourceStatisticsBox->ClearChildren();

	
	AddSeries(PopulationGraph, {
		//{ FString("Adult population"), PlotStatEnum::AdultPopulation, FLinearColor::Yellow },
		{ LOCTEXT("Population", "Population").ToString(), PlotStatEnum::Population, FLinearColor(0.3, 1, 0.3) },
		{ LOCTEXT("Children", "Children").ToString(), PlotStatEnum::ChildPopulation, FLinearColor(0.3, 0.3, 1) },
	});

	AddSeries(IncomeGraph, {
		{ LOCTEXT("Income", "Income").ToString(), PlotStatEnum::Income, FLinearColor(0.3, 1, 0.3) },
		{ LOCTEXT("Revenue", "Revenue").ToString(), PlotStatEnum::Revenue, FLinearColor(0.3, 0.3, 1) },
		{ LOCTEXT("Expense", "Expense").ToString(), PlotStatEnum::Expense, FLinearColor(1, 0.3, 0.3) },
	});
	AddSeries(ScienceGraph, { {FString("Science"), PlotStatEnum::Science, FLinearColor(0.3, 0.3, 1) } });

	AddSeries(FoodFuelGraph, {
		{ LOCTEXT("Food", "Food").ToString(), PlotStatEnum::Food, FLinearColor(0.3, 1, 0.3) },
		{ LOCTEXT("Fuel", "Fuel").ToString(), PlotStatEnum::Fuel, FLinearColor(1, 1, 0.3) },
	});
	AddSeries(FoodUsageGraph, {
		{ LOCTEXT("Food Production (per season)", "Food Production (per season)").ToString(), PlotStatEnum::FoodProduction, FLinearColor(0.3, 1, 0.3) },
		{ LOCTEXT("Food Consumption (per season)", "Food Consumption (per season)").ToString(), PlotStatEnum::FoodConsumption, FLinearColor(1, 0.3, 0.3) },
	});
	
	AddSeries(ImportExportGraph, {
		//{FString("Trade balance"), PlotStatEnum::TradeBalance, FLinearColor::Green },
		{ LOCTEXT("Export", "Export").ToString(), PlotStatEnum::Export, FLinearColor(0.3, 0.3, 1) },
		{ LOCTEXT("Import", "Import").ToString(), PlotStatEnum::Import, FLinearColor(1, 0.3, 0.3) },
	});

	MarketResourceDropdown->ClearOptions();
	for (int32 i = 0; i < ResourceEnumCount; i++) {
		MarketResourceDropdown->AddOption(ResourceNameF(static_cast<ResourceEnum>(i)));
	}
	MarketResourceDropdown->SetSelectedOption(ResourceNameF(ResourceEnum::Wood));
	MarketResourceDropdown->OnSelectionChanged.AddDynamic(this, &UStatisticsUI::OnMarketDropDownChanged);

	AddSeries(MarketPriceGraph, {
		{ LOCTEXT("Current Price", "Current Price").ToString(), PlotStatEnum::MarketPrice, FLinearColor(0.7, 1, 0.7) },
	});

}

void UStatisticsUI::TickUI()
{
	if (GetVisibility() == ESlateVisibility::Collapsed) {
		return;
	}
	
	SubStatSystem& statSystem = simulation().statSystem(uiTownId);

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
				simulation().resourceCountTown(uiTownId, stat.resourceEnum) > 0)
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


			const std::vector<std::vector<int32>>& jobBuildingEnumToIds = simulation().townManager(uiTownId).jobBuildingEnumToIds();

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
					SetText(row->Upkeep, FText::Format(INVTEXT("{0}<img id=\"Coin\"/>"), TEXT_NUM(bld.upkeep())));

					FText allowedOccupants = TEXT_NUM(bld.allowedOccupants());
					if (bld.allowedOccupants() < bld.maxOccupants()) {
						allowedOccupants = TEXT_TAG("<Yellow>", allowedOccupants);
					}
					SetText(row->WorkForce, 
						FText::Format(INVTEXT("{0}/{1}"), TEXT_NUM(bld.occupantCount()), allowedOccupants)
					);


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
		
		SetText(MarketCurrentPrice, TEXT_100(simulation().price100(resourceEnum)));

		SetText(MarketBasePrice, TEXT_100(GetResourceInfo(resourceEnum).basePrice100()));

		int32 netSupplyChange = worldTradeSystem.GetNetPlayerSupplyChange(simulation().townPlayerId(uiTownId), resourceEnum);
		SetText(NetExportText, netSupplyChange >= 0 ? 
			LOCTEXT("Net export (yearly):", "Net export (yearly):") :
			LOCTEXT("Net import (yearly):", "Net import (yearly):")
		);
		SetText(NetExportAmount, to_string(abs(netSupplyChange)));
		SetResourceImage(NetExportImage, resourceEnum, assetLoader());

		TArray<FText> args;

		std::vector<std::pair<int32, int32>> mainImporters = worldTradeSystem.GetTwoMainTraders(resourceEnum, true);
		ADDTEXT_LOCTEXT("Main importers", "Main importers");
		ADDTEXT_INV_(": ");
		for (size_t i = 0; i < mainImporters.size(); i++) {
			ADDTEXT__(simulation().playerNameT(mainImporters[i].first));
			if (i < mainImporters.size() - 1) {
				ADDTEXT_INV_(", ");
			}
		}
		SetText(MainImportersText, args);

		std::vector<std::pair<int32, int32>> mainExporters = worldTradeSystem.GetTwoMainTraders(resourceEnum, false);
		ADDTEXT_LOCTEXT("Main exporters", "Main exporters");
		ADDTEXT_INV_(": ");
		for (size_t i = 0; i < mainExporters.size(); i++) {
			ADDTEXT__(simulation().playerNameT(mainExporters[i].first));
			if (i < mainExporters.size() - 1) {
				ADDTEXT_INV_(", ");
			}
		}
		SetText(MainExportersText, args);
		

		TArray<FText> boughtLeftArgs;
		TArray<FText> boughtRightArgs;
		TArray<FText> soldLeftArgs;
		TArray<FText> soldRightArgs;
		const std::vector<PlayerSupplyChange>& supplyChanges = worldTradeSystem.GetSupplyChanges(resourceEnum);
		for (const PlayerSupplyChange& supplyChange : supplyChanges) {
			if (supplyChange.amount > 0) {
				ADDTEXT(soldLeftArgs, INVTEXT("{0}\n"), simulation().playerNameT(supplyChange.playerId));
				ADDTEXT(soldRightArgs, INVTEXT("{0}\n"), TEXT_NUM(supplyChange.amount));
			} else {
				ADDTEXT(boughtLeftArgs, INVTEXT("{0}\n"), simulation().playerNameT(supplyChange.playerId));
				ADDTEXT(boughtRightArgs, INVTEXT("{0}\n"), TEXT_NUM(supplyChange.amount));
			}
		}
		SetText(BoughtRecordLeftText, boughtLeftArgs);
		SetText(BoughtRecordRightText, boughtRightArgs);
		SetText(SoldRecordLeftText, soldLeftArgs);
		SetText(SoldRecordRightText, soldRightArgs);
	}
	
}


#undef LOCTEXT_NAMESPACE