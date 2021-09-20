// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Blueprint/UserWidget.h"
#include "PunHUD.h"

#include "PunLobbyHUD.generated.h"

/**
 * 
 */
UCLASS()
class APunLobbyHUD : public APunHUD
{
	GENERATED_BODY()
public:
	APunLobbyHUD();
	void BeginPlay() override;

	void PostInitializeComponents() override;
	
	void Tick(float DeltaTime) override;

	class ULobbyUI* lobbyUI() { return _lobbyUI; }
private:
	void Init();
	
private:
	TSubclassOf<UUserWidget> _lobbyClass;

	UPROPERTY() ULobbyUI* _lobbyUI;

	UPROPERTY() UMainMenuAssetLoaderComponent* _mainMenuAssetLoader;
};
