// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "TownBonusIcon.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTownBonusIcon : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* BuildingIcon;
};
