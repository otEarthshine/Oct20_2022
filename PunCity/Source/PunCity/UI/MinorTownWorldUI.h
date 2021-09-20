// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "MinorTownWorldUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UMinorTownWorldUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UButton* DiplomacyButton;
	
	UPROPERTY(meta = (BindWidget)) UButton* AttackButton1;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* AttackButton1RichText;
	UPROPERTY(meta = (BindWidget)) UButton* AttackButton2;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* AttackButton2RichText;

	UPROPERTY(meta = (BindWidget)) UTextBlock* CityNameText;
};
