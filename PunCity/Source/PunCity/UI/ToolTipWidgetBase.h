// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "ToolTipWidgetBase.generated.h"

/**
 * 
 */
UCLASS()
class UToolTipWidgetBase : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) class UPunBoxWidget* TooltipPunBoxWidget;

	UPROPERTY(meta = (BindWidget)) class USizeBox* TipSizeBox;
};
