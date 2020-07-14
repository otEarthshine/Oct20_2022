// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "ResourceStatTableRow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UResourceStatTableRow : public UPunWidget
{
	GENERATED_BODY()
public:
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* ProductionSeason0;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ProductionSeason1;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ProductionSeason2;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ProductionSeason3;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ProductionTotal;

	UPROPERTY(meta = (BindWidget)) UTextBlock* ConsumptionSeason0;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ConsumptionSeason1;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ConsumptionSeason2;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ConsumptionSeason3;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ConsumptionTotal;

	UPROPERTY(meta = (BindWidget)) UImage* ResourceImage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResourceText;
};
