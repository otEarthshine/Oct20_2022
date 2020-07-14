// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameUIDataSource.h"
#include "TimeSeriesPlot.h"
#include "../Simulation/GameSimulationCore.h"
#include <functional>

#include "GraphDataSource.generated.h"

struct GraphSeries
{
	FString seriesId;
	PlotStatEnum plotStatEnum;
	FLinearColor color;
};

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UGraphDataSource : public UWidget , public IKantanCartesianDatasourceInterface
{
	GENERATED_BODY()
public:
	void Init(int32 playerId, IGameUIDataSource* dataSource) {
		_dataSource = dataSource;
		_playerId = playerId;
	}

	//void AddSeries(FString seriesName, std::function<void(TArray<FKantanCartesianDatapoint>&, IGameUIDataSource*, int32)> fillDataFunc) {
	//	_seriesNames.Add(seriesName);
	//	_seriesFillDataFunc.push_back(fillDataFunc);
	//}

	void AddSeries(std::vector<GraphSeries> seriesList) {
		_seriesList = seriesList;
	}

	int32 GetNumSeries_Implementation() const override { return _seriesList.size(); }
	FName GetSeriesId_Implementation(int32 CatIdx) const override { return FName(*_seriesList[CatIdx].seriesId); }
	FText GetSeriesName_Implementation(int32 SeriesIdx) const override { return FText::FromString(_seriesList[SeriesIdx].seriesId); }

	TArray<FKantanCartesianDatapoint> GetSeriesDatapoints_Implementation(int32 SeriesIdx) const override
	{
		TArray<FKantanCartesianDatapoint> results;
		if (_dataSource && _dataSource->simulationPtr()) 
		{
			PlotStatEnum plotStatEnum = _seriesList[SeriesIdx].plotStatEnum;

			std::vector<int32> statVec;
			if (plotStatEnum == PlotStatEnum::MarketPrice ||
				plotStatEnum == PlotStatEnum::TipMarketPrice) 
			{
				auto& worldTradeSys = _dataSource->simulation().worldTradeSystem();

				// Last element being the current price
				ResourceEnum graphResourceEnum = worldTradeSys.resourceEnumToShowStat;
				if (plotStatEnum == PlotStatEnum::TipMarketPrice) {
					graphResourceEnum = resourceEnum;
				}

				statVec = worldTradeSys.GetStatVec(graphResourceEnum);
				
				statVec.push_back(worldTradeSys.price100(graphResourceEnum));
				
			} else {
				statVec = _dataSource->simulation().playerOwned(_playerId).GetStatVec(plotStatEnum);
			}

			float timeIntervalYear = 0.025f;
			float multiplier = 1.0f;

			// Import/Export have season as interval instead of 30 sec
			if (plotStatEnum == PlotStatEnum::Import ||
				plotStatEnum == PlotStatEnum::Export ||
				plotStatEnum == PlotStatEnum::TradeBalance) 
			{
				timeIntervalYear = 0.25f;
			}

			if (plotStatEnum == PlotStatEnum::MarketPrice ||
				plotStatEnum == PlotStatEnum::TipMarketPrice) {
				multiplier = 0.01f;
			}
			
			for (int32 i = 0; i < statVec.size(); i++) {
				results.Add({ FVector2D(i * timeIntervalYear, statVec[i] * multiplier) });
			}
		}
		return results;
	}

public:
	ResourceEnum resourceEnum = ResourceEnum::None;

private:
	IGameUIDataSource* _dataSource = nullptr;
	int32 _playerId = -1;
	
	std::vector<GraphSeries> _seriesList;
	//std::vector<std::function<void(TArray<FKantanCartesianDatapoint>&, IGameUIDataSource*, int32)>> _seriesFillDataFunc;
};
