// Fill out your copyright notice in the Description page of Project Settings.

#include "PunMainMenuHUD.h"
#include "MainMenuUI.h"
#include "PunCity/PunGameInstance.h"
#include "Components/AudioComponent.h"

APunMainMenuHUD::APunMainMenuHUD()
{
	{
		static ConstructorHelpers::FClassFinder<UUserWidget> uiAsset(TEXT("/Game/UI/MainMenu/MainMenuUIWidget"));
		check(uiAsset.Class);
		_mainMenuClass = uiAsset.Class;
	}

	{
		static ConstructorHelpers::FClassFinder<UUserWidget> uiAsset(TEXT("/Game/UI/MainMenu/LoadingScreenWidget"));
		check(uiAsset.Class);
		LoadingScreenClass = uiAsset.Class;
	}

	_menuSound = LoadF<USoundBase>("/Game/Sound/Cue_Menu");
}

void APunMainMenuHUD::BeginPlay()
{
	Super::BeginPlay();

	_mainMenuUI = Cast<UMainMenuUI>(CreateWidget<UUserWidget>(GetWorld(), _mainMenuClass));
	check(_mainMenuUI);
	_mainMenuUI->AddToViewport();
	_mainMenuUI->SetHUD(this, UIEnum::None);
	_mainMenuUI->Init();

	auto gameInstance = Cast<UPunGameInstance>(GetGameInstance());
	gameInstance->CreateMainMenuSound(_menuSound);
	gameInstance->PlayMenuSound();
	gameInstance->LoadingScreenClass = LoadingScreenClass;
}

void APunMainMenuHUD::Tick(float DeltaSeconds) {
	if (_mainMenuUI) {
		_mainMenuUI->Tick();
	}
}