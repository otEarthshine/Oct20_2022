// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "PunEditableText.h"
#include "PunEditableNumberBox.generated.h"

/**
 * 
 */
UCLASS()
class UPunEditableNumberBox : public UPunWidget
{
	GENERATED_BODY()
public:
	void OnInit() override
	{
		DescriptionText->SetVisibility(ESlateVisibility::Collapsed);

		EnableCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UPunEditableNumberBox::OnEnableCheckBox);

		ArrowDownButton->OnClicked.Clear();
		ArrowUpButton->OnClicked.Clear();
		ArrowDownButton->OnClicked.AddUniqueDynamic(this, &UPunEditableNumberBox::ClickArrowDownButton);
		ArrowUpButton->OnClicked.AddUniqueDynamic(this, &UPunEditableNumberBox::ClickArrowUpButton);
		
		EditableNumber->PunEditableTextBox->OnTextCommitted.Clear();
		EditableNumber->PunEditableTextBox->OnTextCommitted.AddDynamic(this, &UPunEditableNumberBox::NumberChanged);
		amount = 0;
		UpdateText();

		callbackWidgetCaller = this;

		justInitialized = true;
	}

	void Set(UPunWidget* callbackTarget, CallbackEnum callbackEnum, int32 objectId = -1, FText description = FText(),
			FText checkBoxEnabledDescription = FText(), bool isChecked = false, ResourceEnum resourceEnumIn = ResourceEnum::None)
	{
		_callbackTarget = callbackTarget;
		_callbackEnum = callbackEnum;

		punId = objectId;
		uiIndex = -1;

		if (description.IsEmpty()) {
			DescriptionText->SetVisibility(ESlateVisibility::Collapsed);
		} else {
			DescriptionText->SetText(description);
			DescriptionText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}

		ESlateVisibility editableNumberVisibility;
		if (!checkBoxEnabledDescription.IsEmpty()) {
			EnableCheckBox->SetVisibility(ESlateVisibility::Visible);
			EnableCheckBox->SetIsChecked(isChecked);
			DescriptionText->SetText(isChecked ? checkBoxEnabledDescription : description);

			editableNumberVisibility = isChecked ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
		} else {
			EnableCheckBox->SetVisibility(ESlateVisibility::Collapsed);

			editableNumberVisibility = ESlateVisibility::Visible;
		}

		EditableNumber->SetVisibility(editableNumberVisibility);
		ArrowDownButton->SetVisibility(editableNumberVisibility);
		ArrowUpButton->SetVisibility(editableNumberVisibility);

		resourceEnum = resourceEnumIn;
		if (resourceEnum != ResourceEnum::None && isChecked) {
			IconImage->SetVisibility(ESlateVisibility::Visible);
			SetResourceImage(IconImage, resourceEnum, assetLoader());
		} else {
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}

		justInitialized = false;

		PUN_CHECK(ArrowDownButton->OnClicked.GetAllObjects().Num() > 0); // Ensure initialized
	}

	void UpdateText() {
		EditableNumber->SetFString(FString::FromInt(amount));
	}

public:
	UPROPERTY(meta = (BindWidget)) UCheckBox* EnableCheckBox;
	UPROPERTY(meta = (BindWidget)) UImage* IconImage;
	
	UPROPERTY(meta = (BindWidget)) URichTextBlock* DescriptionText;
	UPROPERTY(meta = (BindWidget)) UPunEditableText* EditableNumber;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowDownButton;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowUpButton;

	int32 amount;
	ResourceEnum resourceEnum = ResourceEnum::None;

	bool justInitialized = false;

	int32 incrementMultiplier = 10;
	int32 shiftIncrementMultiplier = 100;

	int32 minAmount = MIN_int32;
	int32 maxAmount = MAX_int32;

	bool isUsingSpecialControl = false; // Special Ctrl key will increment/decrement to maximum possible
	int32 ctrlClickIncrementAmount = 10;
	int32 ctrlClickDecrementAmount = 10;

	bool isEditableNumberActive = true;

	int32 uiIndex = -1;
	std::function<void(int32, int32, int32, IGameNetworkInterface*)> onEditNumber;

	UPROPERTY() UPunWidget* callbackWidgetCaller;
	
private:
	UFUNCTION() void ClickArrowDownButton();
	UFUNCTION() void ClickArrowUpButton();
	UFUNCTION() void NumberChanged(const FText& Text, ETextCommit::Type CommitMethod);

	void ClickArrow(bool isDown);

	UFUNCTION() void OnEnableCheckBox(bool active)
	{
		if (active) {
			amount = 1000;
			UpdateText();
		}
		isEditableNumberActive = active;
		
		if (_callbackTarget) {
			_callbackTarget->CallBack1(this, _callbackEnum);
		}
	}

private:
	UPROPERTY() UPunWidget* _callbackTarget = nullptr;
	CallbackEnum _callbackEnum;
};
