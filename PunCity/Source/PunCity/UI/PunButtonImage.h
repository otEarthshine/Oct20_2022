// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "PunButtonImage.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunButtonImage : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UButton* Button1;
	UPROPERTY(meta = (BindWidget)) UImage* Image1;
	UPROPERTY(meta = (BindWidget)) UImage* Image2;

	int32 index = -1;
	FString name;
	CallbackEnum callbackEnum = CallbackEnum::None;
	UPROPERTY() UPunWidget* callbackParent = nullptr;

	void Setup(int32 indexIn, CallbackEnum callbackEnumIn, UPunWidget* callbackParentIn)
	{
		index = indexIn;
		callbackEnum = callbackEnumIn;
		callbackParent = callbackParentIn;
	}

	virtual void OnInit() override {
		UPunWidget::OnInit();

		BUTTON_ON_CLICK(Button1, this, &UPunButtonImage::OnClickedButton);
	}

	UFUNCTION() void OnClickedButton() {
		callbackParent->CallBack1(this, callbackEnum);
	}
	
};
