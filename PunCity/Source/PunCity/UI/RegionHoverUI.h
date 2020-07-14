// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "RegionHoverUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API URegionHoverUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* ClaimingText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* AutoChoseText;
	
	UPROPERTY(meta = (BindWidget)) USizeBox* ClockBox;
	UPROPERTY(meta = (BindWidget)) UImage* ClockImage;

	UPROPERTY(meta = (BindWidget)) USizeBox* IconSizeBox;
	UPROPERTY(meta = (BindWidget)) UImage* IconImage;
};
