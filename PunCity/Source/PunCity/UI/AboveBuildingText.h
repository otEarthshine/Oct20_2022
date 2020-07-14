// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PunBoxWidget.h"
#include "AboveBuildingText.generated.h"

/**
 * 
 */
UCLASS()
class UAboveBuildingText : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* PunBox;
};
