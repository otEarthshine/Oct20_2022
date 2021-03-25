// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "PunMidRowText.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunMidRowText : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* LeftText;
	UPROPERTY(meta = (BindWidget)) USpacer* Spacer;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RightText;
};
