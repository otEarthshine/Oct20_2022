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

	void PunInit(int32 playerIdIn) {
		_playerId = playerIdIn;
		PlayerZoomButton->OnClicked.Clear();
		PlayerZoomButton->OnClicked.AddDynamic(this, &UPlayerCompareElement::OnClickPlayerZoomButton);
	}

	UFUNCTION() void OnClickPlayerZoomButton() {
		if (simulation().HasTownhall(_playerId)) {
			networkInterface()->SetCameraAtom(simulation().GetTownhallGateCapital(_playerId).worldAtom2());
		}
	}

	int32 GetElementPlayerId() { return _playerId; }

private:
	int32 _playerId = -1;
};
