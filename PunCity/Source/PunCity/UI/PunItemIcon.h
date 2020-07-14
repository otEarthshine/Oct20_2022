// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "PunItemIcon.generated.h"

/**
 * 
 */
UCLASS()
class UPunItemIcon : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* ItemIconText;

	
};
