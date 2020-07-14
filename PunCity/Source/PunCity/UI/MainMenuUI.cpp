// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuUI.h"

#include "UnrealEngine.h"
#include "PunCity/PunGameInstance.h"

using namespace std;

void UMainMenuUI::Init()
{
	RefreshUI(MenuState::HostJoin);

	MainMenuSinglePlayerButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnMainMenuSinglePlayerButtonClick);
	MainMenuMultiplayerButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnClickMainMenuMultiplayerButton);
	MainMenuGameSettingsButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnMainMenuGameSettingsButtonClick);
	MainMenuExitButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnMainMenuExitButtonClick);

	SinglePlayerMenuNewGameButton->OnClicked.AddDynamic(this, &UMainMenuUI::CreateSinglePlayerGame);
	SinglePlayerMenuLoadGameButton->OnClicked.AddDynamic(this, &UMainMenuUI::LoadSinglePlayerGame);
	SinglePlayerMenuBackButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnClickReturnToMainMenu);

	//MultiplayerMenuInternetButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnMultiplayerMenuInternetButtonClick);
	//MultiplayerMenuLANButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnMultiplayerMenuLANButtonClick);
	//MultiplayerMenuBackButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnClickReturnToMainMenu);
	
	LobbyListBackButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnClickReturnToMainMenu);
	LobbyListRefreshButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnRefreshButtonClick);

	LobbyListCreateGameButton->OnClicked.AddDynamic(this, &UMainMenuUI::CreateMultiplayerGame);
	LobbyListLoadGameButton->OnClicked.AddDynamic(this, &UMainMenuUI::LoadMultiplayerGame);
	LobbyListJoinGameButton->OnClicked.AddDynamic(this, &UMainMenuUI::JoinMultiplayerGame);

	LobbyListBox->ClearChildren();

	LobbyRefreshThrobber->SetVisibility(ESlateVisibility::Collapsed);
	LobbyConnectionWarning->SetVisibility(ESlateVisibility::Collapsed);

	SetChildHUD(GameSettingsUI);
	GameSettingsUI->PunInit(this);

	SetChildHUD(LoadSaveUI);
	LoadSaveUI->PunInit(this);

	JoinGameDelayOverlay->SetVisibility(ESlateVisibility::Collapsed);

	gameInstance()->bIsCreatingSession = false;
	gameInstance()->isJoiningGame = false;

	// Popup
	MainMenuPopupCloseButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnClickPopupCloseButton);
	
	// Return to lobby menu
	if(gameInstance()->isReturningToLobbyList)
	{
		RefreshLobbyList();
		MainMenuSwitcher->SetActiveWidget(MultiplayerLobbyListCanvas);

		Spawn2DSound("UI", "UIWindowOpen");
	}

	// No longer caring about a game save now that the mainMenu is brought up
	gameInstance()->SetSavedGameToLoad(GameSaveInfo());
}

void UMainMenuUI::Tick()
{
	gameInstance()->PunTick();

	// Not Connected to Steam, retry every 2 sec
	if (LobbyConnectionWarning->GetVisibility() == ESlateVisibility::Visible &&
		UGameplayStatics::GetTimeSeconds(this) - lastSteamConnectionRetry > 2.0f)
	{
		lastSteamConnectionRetry = UGameplayStatics::GetTimeSeconds(this);
		
#if WITH_EDITOR
		gameInstance()->RefreshSessionInterface("NULL");
#else
		gameInstance()->RefreshSessionInterface("Steam");
#endif

		LobbyConnectionWarning->SetVisibility(gameInstance()->IsSessionInterfaceValid() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	
	// Lobby List
	if (MainMenuSwitcher->GetActiveWidget() == MultiplayerLobbyListCanvas)
	{
		auto sessionSearch = gameInstance()->sessionSearch;
		
		// Check if search result changed
		if (sessionSearch.IsValid()) 
		{
			auto searchResults = sessionSearch->SearchResults;
			if (_lastSearchResults.Num() != searchResults.Num()) {
				_needLobbyListUIRefresh = true;
				_lastSearchResults = searchResults;
			}
			else
			{
				for (int32 i = 0; i < _lastSearchResults.Num(); i++) {
					if (_lastSearchResults[i].GetSessionIdStr() != searchResults[i].GetSessionIdStr()) {
						_needLobbyListUIRefresh = true;
						_lastSearchResults = searchResults;
						break;
					}
				}
			}
		}
		
		if (_needLobbyListUIRefresh) 
		{
			_needLobbyListUIRefresh = false;

			auto& sessions = sessionSearch->SearchResults;

			PUN_DEBUG2("Populating LobbyListBox: %d", sessions.Num());
			
			for (int32 i = 0; i < sessions.Num(); i++)
			{
				if (i >= lobbyListElements.Num()) {
					auto lobbyListElement = AddWidget<ULobbyListElementUI>(UIEnum::LobbyListElement);
					lobbyListElement->PunInit(this);
					lobbyListElements.Add(lobbyListElement);
					LobbyListBox->AddChild(lobbyListElement);
				}
				
				if (sessions[i].IsValid())
				{
					lobbyListElements[i]->sessionSearchResult = sessions[i];

					bool isChosenSession = false;
					if (_chosenSession.IsValid()) {
						PUN_DEBUG2(" _chosenSession valid: %s", *_chosenSession.GetSessionIdStr());
						isChosenSession = sessions[i].GetSessionIdStr() == _chosenSession.GetSessionIdStr();
					}

					PUN_DEBUG2(" Populate Session: %s", *sessions[i].GetSessionIdStr());
					PUN_DEBUG2(" - NumPublicConnections: %d", sessions[i].Session.SessionSettings.NumPublicConnections);
					PUN_DEBUG2(" - NumPrivateConnections: %d", sessions[i].Session.SessionSettings.NumPrivateConnections);
					PUN_DEBUG2(" - NumOpenPublicConnections: %d", sessions[i].Session.NumOpenPublicConnections);
					PUN_DEBUG2(" - NumOpenPrivateConnections: %d", sessions[i].Session.NumOpenPrivateConnections);
					
					lobbyListElements[i]->ElementActiveImage->SetVisibility(isChosenSession ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

					//FString value;
					//sessions[i].Session.SessionSettings.Get(SETTING_MAPNAME, value);
					//lobbyListElements[i]->SessionName->SetText(FText::FromString(value));
					//lobbyListElements[i]->SessionName->SetText(FText::FromString(sessions[i].GetSessionIdStr()));

					FString hostName = sessions[i].Session.OwningUserName;
					lobbyListElements[i]->SessionName->SetText(FText::FromString(hostName.Left(15)));

					// Note:
					// value of "sessions[i].Session.SessionSettings.NumPublicConnections + 1" represents the max number of players (public connections + self)
					// However, 
					int32 maxPlayers = sessions[i].Session.SessionSettings.NumPublicConnections;
					int32 currentPlayers = maxPlayers - sessions[i].Session.NumOpenPublicConnections;

					//sessions[i].Session.OwningUserName;
					
					SetText(lobbyListElements[i]->PlayerCount, to_string(currentPlayers) + "/" + to_string(maxPlayers));
					//SetText(lobbyListElements[i]->Ping, to_string(sessions[i].PingInMs));

					int32 sessionTick = 0;
					sessions[i].Session.SessionSettings.Get(SETTING_TEST, sessionTick);
					PUN_DEBUG2(" - sessionTick %d", sessionTick)
				}
				else {
					PUN_DEBUG2(" Invalid session at index: %d", i);
				}
				
				lobbyListElements[i]->SetVisibility(ESlateVisibility::Visible);
			}

			for (int32 i = sessions.Num(); i < lobbyListElements.Num(); i++) {
				lobbyListElements[i]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}


		// Show throbber when search is in progress
		if (sessionSearch &&
			sessionSearch.IsValid() &&
			sessionSearch->SearchState == EOnlineAsyncTaskState::Type::InProgress &&
			sessionSearch->SearchResults.Num() == 0)
		{
			LobbyRefreshThrobber->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			LobbyRefreshThrobber->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (gameInstance()->bIsCreatingSession) {
			JoinGameDelayOverlay->SetVisibility(ESlateVisibility::Visible);
			SetText(JoinGameDelayText, "Creating Game...");
		}
		else if (gameInstance()->isJoiningGame) {
			JoinGameDelayOverlay->SetVisibility(ESlateVisibility::Visible);
			SetText(JoinGameDelayText, "Joining Game...");
		}
		else {
			JoinGameDelayOverlay->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (MainMenuSwitcher->GetActiveWidget() == LoadSaveUI)
	{
		LoadSaveUI->Tick();
	}


	if (MainMenuPopupOverlay->GetVisibility() == ESlateVisibility::Collapsed &&
		!gameInstance()->mainMenuPopup.IsEmpty())
	{
		MainMenuPopupOverlay->SetVisibility(ESlateVisibility::Visible);
		MainMenuPopupText->SetText(FText::FromString(gameInstance()->mainMenuPopup));
		gameInstance()->mainMenuPopup = FString();
	}
}

void UMainMenuUI::RefreshUI(MenuState state)
{
	//if (state == MenuState::TerrainGeneration)
	//{
	//	MultiplayerGameSetupMenu->SetVisibility(ESlateVisibility::Visible);
	//	//UseExistingTerrainButton->SetVisibility(terrainGenerator->HasSavedMap() ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

	//	MultiplayerMenu->SetVisibility(ESlateVisibility::Hidden);
	//}
	//else
	//{
	//	MultiplayerGameSetupMenu->SetVisibility(ESlateVisibility::Hidden);

	//	MultiplayerMenu->SetVisibility(ESlateVisibility::Visible);
	//}
}

void UMainMenuUI::RefreshLobbyList()
{
	PUN_DEBUG2("RefreshLobbyList");

	if (!gameInstance()->IsSessionInterfaceValid()) 
	{
		// Not connected to Steam
		LobbyConnectionWarning->SetVisibility(ESlateVisibility::Visible);
		LobbyRefreshThrobber->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	LobbyConnectionWarning->SetVisibility(ESlateVisibility::Collapsed);
	
	//TSharedPtr<const FUniqueNetId> steamId = gameInstance()->GetSteamID();
	//if (steamId.IsValid()) {
	//	gameInstance()->FindOnlineSessions(steamId, false, true);
	//}

	gameInstance()->FindGame();

	_needLobbyListUIRefresh = true;
	_chosenSession = FOnlineSessionSearchResult();

	LobbyRefreshThrobber->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	SetButtonEnabled(LobbyListJoinGameButton, ButtonStateEnum::Disabled);
}

//void UMainMenuUI::OnMultiplayerMenuInternetButtonClick()
//{
//	gameInstance()->RefreshSessionInterface("Steam");
//
//	RefreshLobbyList();
//	MainMenuSwitcher->SetActiveWidget(MultiplayerLobbyListCanvas);
//
//	Spawn2DSound("UI", "UIWindowOpen");
//}
void UMainMenuUI::OnMultiplayerMenuLANButtonClick()
{
	gameInstance()->RefreshSessionInterface("NULL");

	RefreshLobbyList();
	MainMenuSwitcher->SetActiveWidget(MultiplayerLobbyListCanvas);

	Spawn2DSound("UI", "UIWindowOpen");
}

void UMainMenuUI::CreateSinglePlayerGame()
{
	PUN_DEBUG2("Starting Single Player Server");

	gameInstance()->RefreshSessionInterface("NULL");
	gameInstance()->CreateSinglePlayerGame();

	Spawn2DSound("UI", "UIWindowOpen");
}

void UMainMenuUI::CreateMultiplayerGame()
{
	// Host
	PUN_DEBUG2("Hosting Server");

	gameInstance()->CreateMultiplayerGame();

	Spawn2DSound("UI", "UIWindowOpen");
}

void UMainMenuUI::JoinMultiplayerGame()
{
	PUN_DEBUG2("Joining Server");
	
	if (_chosenSession.IsValid()) {
		gameInstance()->JoinGame(_chosenSession);
	}
	
	Spawn2DSound("UI", "UIWindowOpen");
}

void UMainMenuUI::Spawn2DSound(std::string groupName, std::string soundName)
{
	gameInstance()->Spawn2DSound(groupName, soundName);
}