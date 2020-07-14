// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "PunTextWidget.generated.h"

/**
 * 
 */
UCLASS()
class UPunTextWidget : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* PunText;

	void SetText(std::string str) {
		PunText->SetText(FText::FromString(ToFString(str)));
	}
	void SetText(std::wstring str) {
		PunText->SetText(FText::FromString(ToFString(str)));
	}
};
