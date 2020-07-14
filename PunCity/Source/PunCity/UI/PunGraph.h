// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "TimeSeriesPlot.h"
#include "PunGraph.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunGraph : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* Graph;

	
};
