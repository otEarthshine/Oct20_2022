// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "BattleFieldArmyUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UBattleFieldArmyUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ArmyBox;



	
};
