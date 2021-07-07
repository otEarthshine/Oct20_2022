// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "WGT_Focus_EditableTextRow_Cpp.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UWGT_Focus_EditableTextRow_Cpp : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* TextLeft;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TextRight;
	UPROPERTY(meta = (BindWidget)) UImage* Icon;

	UPROPERTY(meta = (BindWidget)) UButton* NameEditButton;

	UPROPERTY() UPunWidget* callbackParent = nullptr;
	CallbackEnum callbackEnum = CallbackEnum::None;

	
	virtual void OnInit() override {
		NameEditButton->OnClicked.AddDynamic(this, &UWGT_Focus_EditableTextRow_Cpp::OnButtonDown);
	}

	UFUNCTION() void OnButtonDown() {
		PUN_CHECK(callbackParent);
		callbackParent->CallBack1(this, callbackEnum);
	}
	
};
