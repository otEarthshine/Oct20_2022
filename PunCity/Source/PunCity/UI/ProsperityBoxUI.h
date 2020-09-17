// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/Simulation/UnlockSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PunCity/UI/PunBoxWidget.h"
#include "ProsperityBoxUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UProsperityBoxUI : public UUserWidget
{
	GENERATED_BODY()
public:


	
	TechEnum techEnum = TechEnum::None;

	void Init(UPunWidget* callbackParent, TechEnum techEnumIn)
	{

	}
	
public:
	UPROPERTY(meta = (BindWidget)) UImage* InnerImage;
	UPROPERTY(meta = (BindWidget)) UImage* OuterImage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TechName;

	UPROPERTY(meta = (BindWidget)) UTextBlock* HouseCountText;

	UPROPERTY(meta = (BindWidget)) UImage* RewardBuildingIcon1;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBuildingIcon2;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBuildingIcon3;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBonusIcon1;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBonusIcon2;
};
