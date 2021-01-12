// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "PunRichText.h"
#include "ExclamationIcon.h"
#include "UISystemBase.h"

#include "PunButton.generated.h"

/**
 * 
 */
UCLASS()
class UPunButton : public UPunWidget
{
	GENERATED_BODY()
public:
	void OnInit() override {
		Button->OnClicked.Clear();
		Button->OnClicked.AddDynamic(this, &UPunButton::OnButtonDown);
		ExclamationIcon->SetShow(false);
	}

	void Set(FText topString, FText prefix, UTexture2D* texture, FText suffix, UPunWidget* callbackParent, CallbackEnum callbackEnum, int32 callbackVar1In = -1, int32 callbackVar2In = -1)
	{
		_callbackParent = callbackParent;
		_callbackEnum = callbackEnum;
		callbackVar1 = callbackVar1In;
		callbackVar2 = callbackVar2In;
		PUN_CHECK(Button);

		//TopText->SetVisibility(topString == "" ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		TopText->SetVisibility(topString.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		SetText(TopText, topString);

		//bool isShowingBottomRow = prefix != "" || texture != nullptr || suffix != "";
		bool isShowingBottomRow = !prefix.IsEmpty() || texture != nullptr || !suffix.IsEmpty();
		BottomRow->SetVisibility(isShowingBottomRow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

		PrefixText->SetText(prefix);
		SuffixText->SetText(suffix);
		if (IsValid(texture)) {
			IconImage->SetBrushFromTexture(texture);
			IconImage->SetVisibility(ESlateVisibility::Visible);
		} else {
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	UPROPERTY(meta = (BindWidget)) UButton* Button;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* TopText;

	UPROPERTY(meta = (BindWidget)) USizeBox* BottomRow;
	UPROPERTY(meta = (BindWidget)) UImage* IconImage;
	UPROPERTY(meta = (BindWidget)) UPunRichText* PrefixText;
	UPROPERTY(meta = (BindWidget)) UPunRichText* SuffixText;

	UPROPERTY(meta = (BindWidget)) UExclamationIcon* ExclamationIcon;

private:
	UPROPERTY() UPunWidget* _callbackParent = nullptr;

	CallbackEnum _callbackEnum = CallbackEnum::None;

	UFUNCTION() void OnButtonDown() {
		PUN_CHECK(_callbackParent);
		_callbackParent->CallBack1(this, _callbackEnum);
	}
};