// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../DisplaySystem/PunTerrainGenerator.h"

#include "PunWidget.h"
#include "PunCity/PunGameInstance.h"
#include "Components/WidgetSwitcher.h"
#include "GameSettingsUI.h"
#include "LobbyListElementUI.h"
#include "Components/CircularThrobber.h"
#include "LoadSaveUI.h"

//#include <memory>

#include "MainMenuUI.generated.h"
/**
 * 
 */
UCLASS()
class UMainMenuUI : public UPunWidget
{
	GENERATED_BODY()
public:
	enum class MenuState {
		TerrainGeneration,
		HostJoin,
	};

	void Init();

	void Tick();

	UPROPERTY(meta = (BindWidget)) UOverlay* MultiplayerMenu;
	UPROPERTY(meta = (BindWidget)) UButton* HostButton;
	UPROPERTY(meta = (BindWidget)) UButton* JoinButton;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* AddressInputBox;

	/*
	 * New UI
	 */

	UPROPERTY(meta = (BindWidget)) UButton* MainMenuSinglePlayerButton; // Switch Index 1
	UPROPERTY(meta = (BindWidget)) UButton* MainMenuMultiplayerButton; // Switch Index 2
	UPROPERTY(meta = (BindWidget)) UButton* MainMenuGameSettingsButton; // Switch Index 3
	UPROPERTY(meta = (BindWidget)) UButton* MainMenuExitButton;

	UPROPERTY(meta = (BindWidget)) UWidgetSwitcher* MainMenuSwitcher;

	UPROPERTY(meta = (BindWidget)) UButton* SinglePlayerMenuNewGameButton;
	UPROPERTY(meta = (BindWidget)) UButton* SinglePlayerMenuLoadGameButton;
	UPROPERTY(meta = (BindWidget)) UButton* SinglePlayerMenuBackButton;

	//UPROPERTY(meta = (BindWidget)) UButton* MultiplayerMenuInternetButton; // Switch Index 4
	//UPROPERTY(meta = (BindWidget)) UButton* MultiplayerMenuLANButton; // Switch Index 4
	//UPROPERTY(meta = (BindWidget)) UButton* MultiplayerMenuBackButton;

	UPROPERTY(meta = (BindWidget)) UButton* LobbyListBackButton;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyListRefreshButton;
	UPROPERTY(meta = (BindWidget)) UScrollBox* LobbyListBox;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyListJoinGameButton;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyListLoadGameButton;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyListCreateGameButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* VersionIdText;
	
	UPROPERTY(meta = (BindWidget)) UCircularThrobber* LobbyRefreshThrobber;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyConnectionWarning;
	float lastSteamConnectionRetry = 0.0f;

	UPROPERTY(meta = (BindWidget)) UCanvasPanel* MainMenuCanvas;
	UPROPERTY(meta = (BindWidget)) UCanvasPanel* SinglePlayerMainMenuCanvas;
	UPROPERTY(meta = (BindWidget)) UCanvasPanel* MultiplayerMainMenuCanvas;
	UPROPERTY(meta = (BindWidget)) UGameSettingsUI* GameSettingsUI;
	UPROPERTY(meta = (BindWidget)) UCanvasPanel* MultiplayerLobbyListCanvas;
	UPROPERTY(meta = (BindWidget)) ULoadSaveUI* LoadSaveUI;

	UPROPERTY(meta = (BindWidget)) UOverlay* JoinGameDelayOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* JoinGameDelayText;

	// Popup
	UPROPERTY(meta = (BindWidget)) UOverlay* MainMenuPopupOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MainMenuPopupText;
	UPROPERTY(meta = (BindWidget)) UButton* MainMenuPopupCloseButton;

	UFUNCTION() void OnClickPopupCloseButton() {
		gameInstance()->Spawn2DSound("UI", "UIWindowClose");
		MainMenuPopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

public:

	UFUNCTION() void OnMainMenuSinglePlayerButtonClick() {
		MainMenuSwitcher->SetActiveWidget(SinglePlayerMainMenuCanvas);
		Spawn2DSound("UI", "UIWindowOpen");
	}

	
	UFUNCTION() void OnClickMainMenuMultiplayerButton()
	{
#if WITH_EDITOR
		gameInstance()->RefreshSessionInterface("NULL");
#else
		gameInstance()->RefreshSessionInterface("Steam");
#endif

		RefreshLobbyList();
		MainMenuSwitcher->SetActiveWidget(MultiplayerLobbyListCanvas);

		Spawn2DSound("UI", "UIWindowOpen");
	}
	UFUNCTION() void OnMainMenuGameSettingsButtonClick() {
		GameSettingsUI->RefreshUI();
		MainMenuSwitcher->SetActiveWidget(GameSettingsUI);

		Spawn2DSound("UI", "UIWindowOpen");
	}
	UFUNCTION() void OnMainMenuExitButtonClick()
	{
		Spawn2DSound("UI", "UIWindowClose");
		
		UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
	}

	//UFUNCTION() void OnMultiplayerMenuInternetButtonClick();
	UFUNCTION() void OnMultiplayerMenuLANButtonClick();

	UFUNCTION() void OnClickReturnToMainMenu() {
		Spawn2DSound("UI", "UIWindowClose");
		ReturnToMainMenu();
	}
	void ReturnToMainMenu() {
		// When returning to mainMenu destroy sessionInterface

		
		MainMenuSwitcher->SetActiveWidget(MainMenuCanvas);
	}

	UFUNCTION() void CreateSinglePlayerGame();
	UFUNCTION() void LoadSinglePlayerGame() {
		Spawn2DSound("UI", "UIWindowOpen");
		MainMenuSwitcher->SetActiveWidget(LoadSaveUI);
		gameInstance()->isSinglePlayer = true;
		LoadSaveUI->OpenLoadUI();
	}
	
	UFUNCTION() void CreateMultiplayerGame();
	UFUNCTION() void JoinMultiplayerGame();
	UFUNCTION() void LoadMultiplayerGame() {
		Spawn2DSound("UI", "UIWindowOpen");
		MainMenuSwitcher->SetActiveWidget(LoadSaveUI);
		gameInstance()->isSinglePlayer = false;
		LoadSaveUI->OpenLoadUI();
	}

	UFUNCTION() void OnRefreshButtonClick() {
		Spawn2DSound("UI", "ButtonClick");
		RefreshLobbyList();
	}
	void RefreshLobbyList();

	void CallBack2(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override {
		if (callbackEnum == CallbackEnum::CloseGameSettingsUI) {
			ReturnToMainMenu();
		}
		else if (callbackEnum == CallbackEnum::CloseLoadSaveUI) {
			ReturnToMainMenu();
		}
		else if (callbackEnum == CallbackEnum::LobbyListElementSelect) {
			_chosenSession = CastChecked<ULobbyListElementUI>(punWidgetCaller)->sessionSearchResult;
			_needLobbyListUIRefresh = true;

			PUN_DEBUG2("LobbyListElementSelect chosenSession:%s", *_chosenSession.GetSessionIdStr());

			SetButtonEnabled(LobbyListJoinGameButton, ButtonStateEnum::Enabled);
		}
	}

	UPROPERTY() TArray<ULobbyListElementUI*> lobbyListElements;

	void Spawn2DSound(std::string groupName, std::string soundName);
	
private:
	// Old
	void RefreshUI(MenuState state);
	
private:
	// Old
	std::unique_ptr<PunTerrainGenerator> terrainGenerator;
	FString _hostToJoin;

private:
	bool _needLobbyListUIRefresh = false;
	TArray<FOnlineSessionSearchResult> _lastSearchResults;
	
	FOnlineSessionSearchResult _chosenSession = FOnlineSessionSearchResult();
};
