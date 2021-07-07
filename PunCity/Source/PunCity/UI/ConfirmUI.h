// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "ConfirmUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UConfirmUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ConfirmText;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* ConfirmYesButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* ConfirmNoButton;
};
