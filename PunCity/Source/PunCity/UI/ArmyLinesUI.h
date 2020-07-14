// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "ArmyLinesUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UArmyLinesUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* BackLine;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* FrontLine;

	UPROPERTY(meta = (BindWidget)) UTextBlock* ArrivalText;

	
};
