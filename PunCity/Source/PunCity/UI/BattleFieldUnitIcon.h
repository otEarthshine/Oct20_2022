// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "BattleFieldUnitIcon.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UBattleFieldUnitIcon : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* UnitImage;
	UPROPERTY(meta = (BindWidget)) UOverlay* DamageFloatupOverlay;

	UPROPERTY(meta = (BindWidget)) UTextBlock* UnitCountText;

	int32 lastDamageTick = -1;
};
