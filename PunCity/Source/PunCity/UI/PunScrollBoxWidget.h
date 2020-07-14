// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunWidget.h"

#include "PunScrollBoxWidget.generated.h"

/**
 * 
 */
UCLASS()
class UPunScrollBoxWidget : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) class UPunBoxWidget* PunBoxWidget;

};
