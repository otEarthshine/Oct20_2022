// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"

#include "PunRichText.generated.h"

/**
 * 
 */
UCLASS()
class UPunRichText : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) URichTextBlock* PunRichText;

	void SetRichText(const FString& string) {
		PunRichText->SetText(FText::FromString(string));
	}

	void SetText(std::string string) {
		PunRichText->SetText(ToFText(string));
	}
	void SetText(std::wstring string) {
		PunRichText->SetText(ToFText(string));
	}

	UPunRichText* SetJustification(ETextJustify::Type justification) {
		PunRichText->SetJustification(justification);
		return this;
	}
	UPunRichText* SetAutoWrapText(bool autoTextWrap) {
		PunRichText->SetAutoWrapText(autoTextWrap);
		return this;
	}

	void FlashRed()
	{
		
	}
};
