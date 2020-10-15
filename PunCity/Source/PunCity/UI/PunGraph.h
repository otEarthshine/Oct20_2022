// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "TimeSeriesPlot.h"
#include "GraphDataSource.h"
#include "PunGraph.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunGraph : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) USizeBox* GraphSizeBox;
	
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* Graph;

	void SetDataSource(UGraphDataSource* graphDataSource)
	{
		const std::vector<GraphSeries>& seriesList = graphDataSource->GetSeries();
		for (int32 i = 0; i < seriesList.size(); i++)
		{
			FName seriesId = FName(*seriesList[i].seriesId);
			Graph->EnableSeries(seriesId, true);
			Graph->ConfigureSeries(seriesId, false, true);
			Graph->AddSeriesStyleOverride(seriesId, nullptr, seriesList[i].color);
			Graph->SetDatasource(graphDataSource);
		}
	}
	
};
