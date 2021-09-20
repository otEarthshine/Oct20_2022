// Fill out your copyright notice in the Description page of Project Settings.

#include "PunLobbyHUD.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/AudioComponent.h"
#include "LobbyUI.h"
#include "PunCity/PunGameInstance.h"

APunLobbyHUD::APunLobbyHUD()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> lobbyUIFinder(TEXT("/Game/UI/MainMenu/LobbyUIWidget"));
	check(lobbyUIFinder.Class);
	_lobbyClass = lobbyUIFinder.Class;

	_mainMenuAssetLoader = CreateDefaultSubobject<UMainMenuAssetLoaderComponent>("MainMenuAssetLoader");
}

void APunLobbyHUD::BeginPlay()
{
	Super::BeginPlay();

	PUN_DEBUG2("PunLobbyHUD BeginPlay");

	Init();
}

void APunLobbyHUD::PostInitializeComponents()
{
	PUN_DEBUG2("PunLobbyHUD PostInitializeComponents");
	Super::PostInitializeComponents();
	Init();
}

void APunLobbyHUD::Init()
{
	PUN_DEBUG2("PunLobbyHUD Init");
	
	_lobbyUI = Cast<ULobbyUI>(CreateWidget<UUserWidget>(GetWorld(), _lobbyClass));
	check(_lobbyUI);
	_lobbyUI->AddToViewport();
	_lobbyUI->SetHUD(this, UIEnum::None);
	_lobbyUI->Init(_mainMenuAssetLoader);
}

void APunLobbyHUD::Tick(float DeltaTime)
{
	_lobbyUI->Tick();
}