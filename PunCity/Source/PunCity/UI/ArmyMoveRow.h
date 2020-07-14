// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "PunEditableNumberBox.h"
#include "ArmyMoveRow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UArmyMoveRow : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* ArmyUnitBackground;
	UPROPERTY(meta = (BindWidget)) UImage* ArmyUnitIcon;
	UPROPERTY(meta = (BindWidget)) UTextBlock* FromText;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* ToText;
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* ArmyCount;

	void OnInit() override {
		SetChildHUD(ArmyCount);
	}

	int32 armyEnumInt = -1;
};
