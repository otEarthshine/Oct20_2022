// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "ArmyRow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UArmyRow : public UPunWidget
{
	GENERATED_BODY()
public:
	//UPROPERTY(meta = (BindWidget)) UImage* ArmyImage;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* ArmyName;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* ArmySize;
	//UPROPERTY(meta = (BindWidget)) UButton* ActivateButton;

	//void PunInit(ArmyEnum armyEnum, std::string armyName, std::string armySize, bool showActivateButton = false) {
	//	ArmyImage->GetDynamicMaterial()->SetTextureParameterValue("ArmyIcon", assetLoader()->GetArmyIcon(armyEnum));
	//	SetText(ArmyName, armyName);
	//	SetText(ArmySize, armySize);
	//	ActivateButton->SetVisibility(showActivateButton ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	//}
};
