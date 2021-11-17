// Pun Dumnernchanvanit's

#pragma once

#include "PunSpineWidget.h"
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
	UPROPERTY(meta = (BindWidget)) UPunSpineWidget* UnitImage;
	UPROPERTY(meta = (BindWidget)) UImage* BackgroundImage;
	
	UPROPERTY(meta = (BindWidget)) UOverlay* DamageFloatupOverlay;

	UPROPERTY(meta = (BindWidget)) UTextBlock* UnitCountText;

	int32 lastDamageTick = -1;
	int32 lastAttackTick = -1;
};
