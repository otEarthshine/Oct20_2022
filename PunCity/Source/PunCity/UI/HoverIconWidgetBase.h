// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunWidget.h"

#include "HoverIconWidgetBase.generated.h"

/**
 * 
 */
UCLASS()
class UHoverIconWidgetBase : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) USizeBox* IconSizeBox;
	UPROPERTY(meta = (BindWidget)) class UImage* IconImage;
};
