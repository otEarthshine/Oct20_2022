// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"

#include "HumanSlotIcon.generated.h"

/**
 * 
 */
UCLASS()
class UHumanSlotIcon : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) class USizeBox* SlotFiller;
	UPROPERTY(meta = (BindWidget)) class USizeBox* SlotShadow;
	UPROPERTY(meta = (BindWidget)) UImage* SlotFillerImage;
	UPROPERTY(meta = (BindWidget)) class USizeBox* SlotCross;
};
