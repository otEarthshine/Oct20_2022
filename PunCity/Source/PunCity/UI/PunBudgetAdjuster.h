// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "PunBudgetAdjuster.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunBudgetAdjuster : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* Icon1;
	UPROPERTY(meta = (BindWidget)) UImage* Icon2;
	UPROPERTY(meta = (BindWidget)) UImage* Icon3;
	UPROPERTY(meta = (BindWidget)) UImage* Icon4;
	UPROPERTY(meta = (BindWidget)) UImage* Icon5;

	UPROPERTY(meta = (BindWidget)) UButton* BackButton1;
	UPROPERTY(meta = (BindWidget)) UButton* BackButton2;
	UPROPERTY(meta = (BindWidget)) UButton* BackButton3;
	UPROPERTY(meta = (BindWidget)) UButton* BackButton4;
	UPROPERTY(meta = (BindWidget)) UButton* BackButton5;



	
};
