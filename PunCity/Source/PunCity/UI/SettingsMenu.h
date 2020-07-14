// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsMenu.generated.h"

/**
 * 
 */
UCLASS()
class USettingsMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual bool Initialize();
	
private:
	UPROPERTY(meta = (BindWidget)) class UButton* TestButton;
	
	UFUNCTION() void Test();
};
