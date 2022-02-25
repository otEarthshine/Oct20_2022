// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunBoxWidget.h"
#include "Components/WidgetSwitcher.h"
#include "TimeSeriesPlot.h"
#include "GraphDataSource.h"
#include "KantanChartLegend.h"
#include "StatisticsUI.generated.h"


/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UStatisticsUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void InitStatisticsUI();

	void TickUI();

public:
	//UPROPERTY(meta = (BindWidget)) USizeBox* StatisticsPanel;
	UPROPERTY(meta = (BindWidget)) UButton* StatisticsCloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* StatisticsXButton;

	// Town Swap (Stats)
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* TownSwapHorizontalBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TownSwapText;
	UPROPERTY(meta = (BindWidget)) UButton* TownSwapArrowLeftButton;
	UPROPERTY(meta = (BindWidget)) UButton* TownSwapArrowRightButton;

	UPROPERTY(meta = (BindWidget)) UButton* OverviewStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* BuildingsStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* PopulationStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* IncomeStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* ScienceStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* FoodFuelStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* FoodUsageStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* HappinessStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* ImportExportStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* MarketStatButton;

	UPROPERTY(meta = (BindWidget)) UWidgetSwitcher* StatSwitcher;

	UPROPERTY(meta = (BindWidget)) UTextBlock* ResourceSeasonText0;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResourceSeasonText1;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResourceSeasonText2;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResourceSeasonText3;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ResourceStatisticsBox;

	UPROPERTY(meta = (BindWidget)) UScrollBox* BuildingsStatBox;
	int32 lastRefreshBuildingStatBox = -INT32_MIN;

	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* PopulationGraph;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* IncomeGraph;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* ScienceGraph;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* FoodFuelGraph;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* FoodUsageGraph;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* HappinessStatisticsBox;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* ImportExportGraph;

	/*
	 * Market price
	 */
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* MarketPriceGraph;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* MarketResourceDropdown;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MarketCurrentPrice;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MarketBasePrice;

	UPROPERTY(meta = (BindWidget)) UTextBlock* NetExportText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* NetExportAmount;
	UPROPERTY(meta = (BindWidget)) UImage* NetExportImage;

	UPROPERTY(meta = (BindWidget)) UTextBlock* MainExportersText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MainImportersText;

	UPROPERTY(meta = (BindWidget)) UTextBlock* BoughtRecordLeftText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* BoughtRecordRightText;

	UPROPERTY(meta = (BindWidget)) UTextBlock* SoldRecordLeftText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SoldRecordRightText;

	//UPROPERTY() TArray<UGraphDataSource*> dataSources;

	int32 uiTownId = -1;
	bool showCombinedStatistics = false;

public:
	void OpenStatisticsUI(int32 playerIdIn) {
		uiTownId = playerIdIn;
		showCombinedStatistics = true;
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	UFUNCTION() void CloseStatisticsUI() {
		SetVisibility(ESlateVisibility::Collapsed);
	}

	void SetTabSelection(UButton* button) {
		auto setHighlight = [&](UButton* buttonLocal) {
			SetButtonHighlight(buttonLocal, buttonLocal == button);
		};

		setHighlight(OverviewStatButton);
		setHighlight(BuildingsStatButton);
		setHighlight(PopulationStatButton);
		setHighlight(IncomeStatButton);
		setHighlight(ScienceStatButton);

		setHighlight(FoodFuelStatButton);
		setHighlight(FoodUsageStatButton);
		setHighlight(HappinessStatButton);
		setHighlight(ImportExportStatButton);
		setHighlight(MarketStatButton);
	}

	UButton* GetButtonFromWidgetIndex(int32 widgetIndex)
	{
		switch(widgetIndex)
		{
		case 0: return OverviewStatButton;
		case 1: return BuildingsStatButton;
		case 2: return PopulationStatButton;
		case 3: return IncomeStatButton;
		case 4: return ScienceStatButton;
			
		case 5: return FoodFuelStatButton;
		case 6: return FoodUsageStatButton;
		case 7: return HappinessStatButton;
		case 8: return ImportExportStatButton;
		default:
			return MarketStatButton;
		}
	}

	int32 GetWidgetIndexFromStatButton(UButton* button)
	{
		for (int32 i = 0; i < 10; i++) {
			if (GetButtonFromWidgetIndex(i) == button) {
				return i;
			}
		}
		return 0;
	}

private:
	/*
	 * Buttons
	 */
	void OnStatButtonClick(int32 widgetIndex) {
		SetTabSelection(GetButtonFromWidgetIndex(widgetIndex));
		StatSwitcher->SetActiveWidgetIndex(widgetIndex);
		dataSource()->Spawn2DSound("UI", "ButtonClick");
	}

	UFUNCTION() void OnOverviewStatButtonClick() { OnStatButtonClick(0); }
	UFUNCTION() void OnBuildingsStatButtonClick() { OnStatButtonClick(1); }
	UFUNCTION() void OnPopulationStatButtonClick() { OnStatButtonClick(2); }
	UFUNCTION() void OnIncomeStatButtonClick() { OnStatButtonClick(3); }
	UFUNCTION() void OnScienceStatButtonClick() { OnStatButtonClick(4); }
	UFUNCTION() void OnFoodFuelStatButtonClick() { OnStatButtonClick(5); }
	UFUNCTION() void OnFoodUsageStatButtonClick() { OnStatButtonClick(6); }
	UFUNCTION() void OnHappinessStatButtonClick() { OnStatButtonClick(7); }
	UFUNCTION() void OnImportExportStatButtonClick() { OnStatButtonClick(8); }
	UFUNCTION() void OnMarketStatButtonClick() { OnStatButtonClick(9); }

	UFUNCTION() void OnMarketDropDownChanged(FString sItem, ESelectInfo::Type seltype) {
		if (seltype == ESelectInfo::Type::Direct) return;

		std::wstring resourceName = ToWString(sItem);
		ResourceEnum resourceEnum = FindResourceEnumByName(resourceName);
		if (resourceEnum != ResourceEnum::None) {
			simulation().worldTradeSystem().resourceEnumToShowStat = resourceEnum;
		}
	}

	UFUNCTION() void OnClickTownSwapArrowLeftButton() {
		if (!showCombinedStatistics && uiTownId == playerId()) {
			showCombinedStatistics = true;
		}
		else if (showCombinedStatistics) {
			showCombinedStatistics = false;
			uiTownId = simulation().GetNextTown(false, uiTownId, playerId());
		}
		else {
			uiTownId = simulation().GetNextTown(false, uiTownId, playerId());
		}
		SetGraphSeries();
		lastRefreshBuildingStatBox = 0;
	}
	UFUNCTION() void OnClickTownSwapArrowRightButton() {
		if (showCombinedStatistics) {
			showCombinedStatistics = false;
		} else {
			uiTownId = simulation().GetNextTown(true, uiTownId, playerId());
			if (uiTownId == playerId()) {
				showCombinedStatistics = true;
			}
		}
		SetGraphSeries();
		lastRefreshBuildingStatBox = 0;
	}

private:
	void SetGraphSeries();
};
