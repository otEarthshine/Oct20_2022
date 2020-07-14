// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "Components/Overlay.h"
#include "ArmyUnitUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UArmyUnitUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* ArmyUnitBackground;
	
	UPROPERTY(meta = (BindWidget)) UImage* ArmyUnitHP;
	UPROPERTY(meta = (BindWidget)) UImage* ArmyUnitIcon;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ArmyUnitCount;

	UPROPERTY(meta = (BindWidget)) UOverlay* DamageOverlay;

	int32 lastAttackTick = -1;
};
