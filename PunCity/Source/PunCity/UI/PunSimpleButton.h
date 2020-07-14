// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "PunSimpleButton.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunSimpleButton : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UButton* Button;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text;

	CallbackEnum callbackEnum = CallbackEnum::None;

	void Init(CallbackEnum callbackEnumIn, UPunWidget* parentIn) {
		callbackEnum = callbackEnumIn;
		parent = parentIn;
		Button->OnClicked.RemoveAll(this);
		Button->OnClicked.AddDynamic(this, &UPunSimpleButton::OnClickButton);
	}
	UPROPERTY() UPunWidget* parent;

	UFUNCTION() void OnClickButton() {
		parent->CallBack1(this, callbackEnum);
	}
};
