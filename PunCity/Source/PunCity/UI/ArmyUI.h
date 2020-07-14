// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "Components/Overlay.h"
#include "ArmyUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UArmyUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UOverlay* ArmyOverlay;
	
	UPROPERTY(meta = (BindWidget)) UScrollBox* AvailableArmyBox;

	UPROPERTY(meta = (BindWidget)) UTextBlock* RightColumnTitle;
	UPROPERTY(meta = (BindWidget)) UScrollBox* RightColumnBox;

	UPROPERTY(meta = (BindWidget)) UButton* DispatchButton;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
};
