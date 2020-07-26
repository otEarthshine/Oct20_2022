// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	void OnInit() override {
		DescriptionText->SetVisibility(ESlateVisibility::Collapsed);

		ArrowDownButton->OnClicked.Clear();
		ArrowUpButton->OnClicked.Clear();
		EditableNumber->PunEditableTextBox->OnTextCommitted.Clear();
		
		ArrowDownButton->OnClicked.AddDynamic(this, &UPunEditableNumberBox::ClickArrowDownButton);
		ArrowUpButton->OnClicked.AddDynamic(this, &UPunEditableNumberBox::ClickArrowUpButton);
		EditableNumber->PunEditableTextBox->OnTextCommitted.AddDynamic(this, &UPunEditableNumberBox::NumberChanged);
		amount = 0;
		UpdateText();

		justInitialized = true;
	}

	void Set(UPunWidget* callbackTarget, CallbackEnum callbackEnum, int32 objectId = -1, std::string description = "")
	{
		_callbackTarget = callbackTarget;
		_callbackEnum = callbackEnum;

		punId = objectId;

		if (description == "") {
			DescriptionText->SetVisibility(ESlateVisibility::Collapsed);
		} else {
			DescriptionText->SetText(FText::FromString(ToFString(description)));
			DescriptionText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}

		justInitialized = false;

		PUN_CHECK(ArrowDownButton->OnClicked.GetAllObjects().Num() > 0); // Ensure initialized
	}

	void UpdateText() {
		EditableNumber->SetFString(FString::FromInt(amount));
	}

public:
	UPROPERTY(meta = (BindWidget)) URichTextBlock* DescriptionText;
	UPROPERTY(meta = (BindWidget)) UPunEditableText* EditableNumber;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowDownButton;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowUpButton;

	int32 amount;

	bool justInitialized = false;

	int32 incrementMultiplier = 10;

	int32 minAmount = MIN_int32;
	int32 maxAmount = MAX_int32;

private:
	UFUNCTION() void ClickArrowDownButton();
	UFUNCTION() void ClickArrowUpButton();
	UFUNCTION() void NumberChanged(const FText& Text, ETextCommit::Type CommitMethod);

	void ClickArrow(bool isDown);

private:
	UPROPERTY() UPunWidget* _callbackTarget;
	CallbackEnum _callbackEnum;
};
