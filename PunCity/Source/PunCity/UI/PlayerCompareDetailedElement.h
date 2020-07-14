// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "PlayerCompareDetailedElement.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPlayerCompareDetailedElement : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* HighlightImage;
	
	UPROPERTY(meta = (BindWidget)) URichTextBlock* PlayerNameText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* PopulationText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* TechnologyText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RevenueText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* MilitarySizeText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* LandText;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* AlliesText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* VassalsText;
	
	UPROPERTY(meta = (BindWidget)) URichTextBlock* GDPText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* MainProductionText;
};
