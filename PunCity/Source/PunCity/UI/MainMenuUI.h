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
#include "LobbySettingsUI.h"
#include "LoadSaveUI.h"


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

	UPROPERTY(meta = (BindWidget)) UButton* LobbyListBackButton;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyListRefreshButton;
	UPROPERTY(meta = (BindWidget)) UScrollBox* LobbyListBox;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyListJoinGameButton;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyListLoadGameButton;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyListCreateGameButton;

	UPROPERTY(meta = (BindWidget)) UOverlay* PasswordPopupOverlay;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* PasswordEditableText;
	UPROPERTY(meta = (BindWidget)) UButton* PasswordConfirmButton;
	UPROPERTY(meta = (BindWidget)) UButton* PasswordCancelButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* VersionIdText;
	
	UPROPERTY(meta = (BindWidget)) UCircularThrobber* LobbyRefreshThrobber;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyConnectionWarning;
	float lastSteamConnectionRetry = 0.0f;

	UPROPERTY(meta = (BindWidget)) UCanvasPanel* MainMenuCanvas;
	UPROPERTY(meta = (BindWidget)) UCanvasPanel* SinglePlayerMainMenuCanvas;
	//UPROPERTY(meta = (BindWidget)) UCanvasPanel* MultiplayerMainMenuCanvas;
	UPROPERTY(meta = (BindWidget)) UGameSettingsUI* GameSettingsUI;
	UPROPERTY(meta = (BindWidget)) UCanvasPanel* MultiplayerLobbyListCanvas;
	UPROPERTY(meta = (BindWidget)) ULoadSaveUI* LoadSaveUI;
	UPROPERTY(meta = (BindWidget)) UOverlay* PreLobbySettingsOverlay;

	// Pre Lobby
	UPROPERTY(meta = (BindWidget)) ULobbySettingsUI* PreLobbySettingsUI;
	UPROPERTY(meta = (BindWidget)) UButton* PreLobbySettingsConfirmButton;
	UPROPERTY(meta = (BindWidget)) UButton* PreLobbySettingsBackButton;
	
	//UPROPERTY(meta = (BindWidget)) 

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
		GameSettingsUI->RefreshUI(true, true);
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

	UFUNCTION() void OpenPreLobbySettings();
	UFUNCTION() void CreateMultiplayerGame();

	// Password
	UFUNCTION() void TryOpenJoinPasswordUI() {
		Spawn2DSound("UI", "UIWindowOpen");
		FString password = GetSessionValueString(SESSION_PASSWORD, _chosenSession.Session.SessionSettings);

		if (password == "") {
			JoinMultiplayerGame(); // No password
		} else {
			PasswordPopupOverlay->SetVisibility(ESlateVisibility::Visible);
			PasswordEditableText->SetText(FText());
		}
	}
	UFUNCTION() void OnClickJoinPasswordConfirm();
	UFUNCTION() void OnClickJoinPasswordCancel() {
		PasswordPopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	UFUNCTION() void JoinMultiplayerGame();

	UFUNCTION() void OpenLoadMultiplayerGameUI() {
		Spawn2DSound("UI", "UIWindowOpen");
		MainMenuSwitcher->SetActiveWidget(LoadSaveUI);
		gameInstance()->isSinglePlayer = false;
		LoadSaveUI->OpenLoadUI();
	}
	UFUNCTION() void LoadMultiplayerGame() {
		gameInstance()->LoadMultiplayerGame();
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


	void KeyPressed_Escape() {
		LoadSaveUI->KeyPressed_Escape();
	}

private:
	bool _needLobbyListUIRefresh = false;
	TArray<FOnlineSessionSearchResult> _lastSearchResults;
	
	FOnlineSessionSearchResult _chosenSession = FOnlineSessionSearchResult();
};
