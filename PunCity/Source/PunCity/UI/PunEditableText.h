// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"

#include "PunEditableText.generated.h"

/**
 * 
 */
UCLASS()
class UPunEditableText : public UPunWidget
{
	GENERATED_BODY()
public:

	void OnInit() override
	{
		PunEditableTextBox->SelectAllTextWhenFocused = true;
		PunEditableTextBox->ClearKeyboardFocusOnCommit = true;
	}

	UPROPERTY(meta = (BindWidget)) UEditableTextBox* PunEditableTextBox;

	bool hasFocus() { return _hasFocus; }


	void SetFString(FString string) {
		PunEditableTextBox->SetText(FText::FromString(string));
	}

	FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override
	{
		PUN_LOG("NativeOnFocusReceived");
		_hasFocus = true;
		return OnFocusReceived(InGeometry, InFocusEvent).NativeReply;
	}

	void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override
	{
		PUN_LOG("NativeOnFocusLost");
		_hasFocus = false;
	}

private:
	bool _hasFocus = false;
};
