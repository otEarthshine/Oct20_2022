// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"

#include "PunSelectButton.generated.h"

/**
 * Button that when selected ... can hold an active state...
 */
UCLASS()
class PROTOTYPECITY_API UPunSelectButton : public UPunWidget
{
	GENERATED_BODY()
public:
	void PunInit(UPunWidget* parent) {
		_callbackParent = parent;
		Button->OnClicked.AddDynamic(this, &UPunSelectButton::OnButtonClicked);
	}

	void Set(FText buttonText, UPunWidget* callbackParent, CallbackEnum callbackEnum, int32 callbackVar1In = -1, int32 callbackVar2In = -1) {
		_callbackParent = callbackParent;
		_callbackEnum = callbackEnum;
		callbackVar1 = callbackVar1In;
		callbackVar2 = callbackVar2In;
		PUN_CHECK(Button);

		SetText(ButtonText, buttonText);
	}

	void SetHighlight(bool active) {
		ButtonActiveImage->SetVisibility(active ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	
	UPROPERTY(meta = (BindWidget)) UButton* Button;
	UPROPERTY(meta = (BindWidget)) UImage* ButtonActiveImage;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ButtonText;

private:
	UFUNCTION() void OnButtonClicked();

private:
	UPROPERTY() UPunWidget* _callbackParent;
	CallbackEnum _callbackEnum;
};
