// Fill out your copyright notice in the Description page of Project Settings.

#include "SettingsMenu.h"
#include "Components/Button.h"


bool USettingsMenu::Initialize()
{
	bool success = Super::Initialize();
	if (!success) return false;

	if (!ensure(TestButton)) return false;
	TestButton->OnClicked.AddDynamic(this, &USettingsMenu::Test);

	return true;
}

void USettingsMenu::Test()
{
	UE_LOG(LogTemp, Error, TEXT("Test this"));
}