// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "PlayerCompareElement.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPlayerCompareElement : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* PopulationText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerNameText;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* PopulationBoldText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerNameBoldText;
	
	UPROPERTY(meta = (BindWidget)) UButton* PlayerZoomButton;

	UPROPERTY(meta = (BindWidget)) USizeBox* HomeTownIcon;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerLogo;

	int32 uiPlayerId = -1;
	int32 population = -1;

	void PunInit(int32 playerIdIn, int32 populationIn)
	{
		uiPlayerId = playerIdIn;
		population = populationIn;
		
		PlayerZoomButton->OnClicked.Clear();
		PlayerZoomButton->OnClicked.AddDynamic(this, &UPlayerCompareElement::OnClickPlayerZoomButton);
	}

	UFUNCTION() void OnClickPlayerZoomButton() {
		if (simulation().HasTownhall(uiPlayerId)) {
			networkInterface()->SetCameraAtom(simulation().GetTownhallGateCapital(uiPlayerId).worldAtom2());
		}
	}

};
