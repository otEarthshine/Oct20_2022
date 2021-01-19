// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "Components/Overlay.h"
#include "TopLayerGameUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTopLayerGameUI : public UPunWidget
{
	GENERATED_BODY()
public:
	/*
	 * GamePause
	 */
	UPROPERTY(meta = (BindWidget)) UOverlay* GamePauseOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* GamePauseText;


	UPROPERTY(meta = (BindWidget)) UOverlay* MidScreenMessagePermanent;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MidScreenMessageTextPermanent;


	void PunInit()
	{
		GamePauseOverlay->SetVisibility(ESlateVisibility::Collapsed);
		MidScreenMessagePermanent->SetVisibility(ESlateVisibility::Collapsed);
	}

	void Tick();
	
};
