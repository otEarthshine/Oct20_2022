// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"

#include "TechLineUI.generated.h"

/**
 * 
 */
UCLASS()
class UTechLineUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* Image1;
	UPROPERTY(meta = (BindWidget)) UImage* Image2;
	UPROPERTY(meta = (BindWidget)) UImage* Image3;
	UPROPERTY(meta = (BindWidget)) UImage* Image4;
	UPROPERTY(meta = (BindWidget)) UImage* Image5;
};
