// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuUI.h"

#include "UnrealEngine.h"
#include "PunCity/PunGameInstance.h"

#define LOCTEXT_NAMESPACE "MainMenuUI"

using namespace std;

void UMainMenuUI::Init()
{
	// TODO: fine right?
	//RefreshUI(MenuState::HostJoin);

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

	LobbyListCreateGameButton->OnClicked.AddDynamic(this, &UMainMenuUI::OpenPreLobbySettings);
	LobbyListLoadGameButton->OnClicked.AddDynamic(this, &UMainMenuUI::OpenLoadMultiplayerGameUI);
	
	//LobbyListJoinGameButton->OnClicked.AddDynamic(this, &UMainMenuUI::JoinMultiplayerGame);
	LobbyListJoinGameButton->OnClicked.AddDynamic(this, &UMainMenuUI::TryOpenJoinPasswordUI);
	PasswordConfirmButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnClickJoinPasswordConfirm);
	PasswordCancelButton->OnClicked.AddDynamic(this, &UMainMenuUI::OnClickJoinPasswordCancel);

	LobbyListBox->ClearChildren();

	LobbyRefreshThrobber->SetVisibility(ESlateVisibility::Collapsed);
	LobbyConnectionWarning->SetVisibility(ESlateVisibility::Collapsed);

	SetChildHUD(GameSettingsUI);
	GameSettingsUI->PunInit(this);

	SetChildHUD(LoadSaveUI);
	LoadSaveUI->PunInit(this);

	SetChildHUD(PreLobbySettingsUI);
	//PreLobbySettingsUI->InitLobbySettings(UPunGameInstance::GetSavedMap(false));
	PreLobbySettingsUI->SetPreLobby(true);
	PreLobbySettingsUI->SettingsBackgroundImage->SetVisibility(ESlateVisibility::Collapsed);
	BUTTON_ON_CLICK(PreLobbySettingsBackButton, this, &UMainMenuUI::OnClickReturnToMainMenu);


	

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

	VersionIdText->SetText(ToFText(GetGameVersionString()));

	// TODO: Test
	//VersionIdText->SetText(INVTEXT("Test Test: \u2713 AA\u200BA"));
	

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

			auto& searchResults = sessionSearch->SearchResults;

			PUN_DEBUG2("Populating LobbyListBox: %d", searchResults.Num());
			
			for (int32 i = 0; i < searchResults.Num(); i++)
			{
				if (i >= lobbyListElements.Num()) {
					auto lobbyListElement = AddWidget<ULobbyListElementUI>(UIEnum::LobbyListElement);
					lobbyListElement->PunInit(this);
					lobbyListElements.Add(lobbyListElement);
					LobbyListBox->AddChild(lobbyListElement);
				}
				
				if (searchResults[i].IsValid())
				{
					lobbyListElements[i]->sessionSearchResult = searchResults[i];

					bool isChosenSession = false;
					if (_chosenSession.IsValid()) {
						PUN_DEBUG2(" _chosenSession valid: %s", *_chosenSession.GetSessionIdStr());
						isChosenSession = searchResults[i].GetSessionIdStr() == _chosenSession.GetSessionIdStr();
					}

					const FOnlineSession& session = searchResults[i].Session;
					const FOnlineSessionSettings& sessionSettings = session.SessionSettings;

					PUN_DEBUG2(" Populate Session: %s", *searchResults[i].GetSessionIdStr());
					PUN_DEBUG2(" - Public:%d/%d Private:%d/%d (Open/Num)", session.NumOpenPublicConnections, sessionSettings.NumPublicConnections,
																			session.NumOpenPrivateConnections, sessionSettings.NumPrivateConnections);
					//PUN_DEBUG2(" - NumPrivateConnections: %d", sessions[i].Session.SessionSettings.NumPrivateConnections);
					//PUN_DEBUG2(" - NumOpenPublicConnections: %d", sessions[i].Session.NumOpenPublicConnections);
					//PUN_DEBUG2(" - NumOpenPrivateConnections: %d", sessions[i].Session.NumOpenPrivateConnections);
					
					lobbyListElements[i]->ElementActiveImage->SetVisibility(isChosenSession ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

					//FString value;
					//sessions[i].Session.SessionSettings.Get(SETTING_MAPNAME, value);
					//lobbyListElements[i]->SessionName->SetText(FText::FromString(value));
					//lobbyListElements[i]->SessionName->SetText(FText::FromString(sessions[i].GetSessionIdStr()));

					FString password = GetSessionValueString(SESSION_PASSWORD, sessionSettings);
					lobbyListElements[i]->PasswordLockImage->SetVisibility(password == "" ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
					

					FString hostName = session.OwningUserName;
					lobbyListElements[i]->SessionName->SetText(FText::FromString(hostName.Left(15)));

					// Note:
					// value of "sessions[i].Session.SessionSettings.NumPublicConnections + 1" represents the max number of players (public connections + self)
					// However, 
					int32 maxPlayers = sessionSettings.NumPublicConnections;
					//int32 currentPlayers = maxPlayers - searchResults[i].Session.NumOpenPublicConnections;
					int32 currentPlayers = 0;
					sessionSettings.Get(SESSION_NUM_PLAYERS, currentPlayers);

					//sessions[i].Session.OwningUserName;
					
					SetText(lobbyListElements[i]->PlayerCount, to_string(currentPlayers) + "/" + to_string(maxPlayers));
					//SetText(lobbyListElements[i]->Ping, to_string(sessions[i].PingInMs));

					//searchResults[i].

					TArray<FText> args;

					ADDTEXT_(INVTEXT("OwningUserId: {0}"), FText::FromString(session.OwningUserId->ToDebugString()));
					ADDTEXT_INV_("\n");
					ADDTEXT_(INVTEXT("OwningUserName: {0}"), FText::FromString(session.OwningUserName));
					ADDTEXT_INV_("\n\n");

					//FString value;
					//sessionSettings.Get(SETTING_MAPNAME, value);
					//ss << "SETTING_MAPNAME: " << ToStdString(value);
					PrintSessionSettings(args, sessionSettings);

					ADDTEXT_INV_("\n");
					ADDTEXT_(INVTEXT("SessionInfo:\n{0}"), FText::FromString(session.SessionInfo->ToDebugString()));

					lobbyListElements[i]->buildUniqueId = sessionSettings.BuildUniqueId;
					lobbyListElements[i]->DifferentVersionWarning->SetVisibility(
						GetSessionValue(SESSION_GAME_VERSION, sessionSettings) == GAME_VERSION ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible
					);
					
					AddToolTip(lobbyListElements[i], JOINTEXT(args));
				}
				else {
					PUN_DEBUG2(" Invalid session at index: %d", i);
					AddToolTip(lobbyListElements[i], LOCTEXT("Invalid Session", "Invalid Session"));
				}
				
				lobbyListElements[i]->SetVisibility(ESlateVisibility::Visible);
			}

			for (int32 i = searchResults.Num(); i < lobbyListElements.Num(); i++) {
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
	}


	// Create/Join Menu Block
	if (gameInstance()->bIsCreatingSession) {
		JoinGameDelayOverlay->SetVisibility(ESlateVisibility::Visible);
		SetText(JoinGameDelayText, LOCTEXT("CreatingGame", "Creating Game..."));
	}
	else if (gameInstance()->isJoiningGame) {
		JoinGameDelayOverlay->SetVisibility(ESlateVisibility::Visible);
		SetText(JoinGameDelayText, LOCTEXT("JoiningGame", "Joining Game..."));
	}
	else {
		JoinGameDelayOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	

	if (MainMenuSwitcher->GetActiveWidget() == LoadSaveUI)
	{
		LoadSaveUI->Tick();

		if (gameInstance()->isOpeningLoadMutiplayerPreLobby) {
			gameInstance()->isOpeningLoadMutiplayerPreLobby = false;

			OpenPreLobbySettings();
		}
	}

	if (MainMenuSwitcher->GetActiveWidget() == PreLobbySettingsOverlay)
	{
		bool isLoading = gameInstance()->IsLoadingSavedGame();
		PreLobbySettingsUI->Tick(isLoading);
	}


	if (MainMenuPopupOverlay->GetVisibility() == ESlateVisibility::Collapsed &&
		!gameInstance()->mainMenuPopup.IsEmpty())
	{
		MainMenuPopupOverlay->SetVisibility(ESlateVisibility::Visible);
		MainMenuPopupText->SetText(gameInstance()->mainMenuPopup);
		gameInstance()->mainMenuPopup = FText();
	}
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

void UMainMenuUI::OpenPreLobbySettings()
{
	gameInstance()->SetCreateMultiplayerGameState();
	
	MainMenuSwitcher->SetActiveWidget(PreLobbySettingsOverlay);
	PreLobbySettingsUI->InitLobbySettings(UPunGameInstance::GetSavedMap(false));

	if (gameInstance()->IsLoadingSavedGame()) {
		BUTTON_ON_CLICK(PreLobbySettingsConfirmButton, this, &UMainMenuUI::LoadMultiplayerGame);
	} else {
		BUTTON_ON_CLICK(PreLobbySettingsConfirmButton, this, &UMainMenuUI::CreateMultiplayerGame);
	}
	
	Spawn2DSound("UI", "UIWindowOpen");
}

void UMainMenuUI::CreateMultiplayerGame()
{
	// Host
	PUN_DEBUG2("Hosting Server");

	gameInstance()->SetMapSettings(PreLobbySettingsUI->serverMapSettings);

	gameInstance()->CreateMultiplayerGame();

	Spawn2DSound("UI", "UIWindowOpen");
}

void UMainMenuUI::JoinMultiplayerGame()
{
	PUN_DEBUG2("Joining Server");

	// Different Build
	int32 hostVersion = GetSessionValue(SESSION_GAME_VERSION, _chosenSession.Session.SessionSettings);
	if (hostVersion != GAME_VERSION)
	{
		//stringstream ss;
		//ss << "Cannot join a game with different version.\n";
		//ss << "Please try restarting Steam to get the latest version.\n";
		//ss << " your version: " << GetGameVersionString(GAME_VERSION) << "\n";
		//ss << " host version: " << GetGameVersionString(hostVersion);
		
		//gameInstance()->mainMenuPopup = ToFString(ss.str());
		gameInstance()->mainMenuPopup = FText::Format(
			LOCTEXT("Joining Server", "Cannot join a game with different version.\nPlease try restarting Steam to get the latest version.\n your version: {0}}\n host version: {1}\n"),
			FText::FromString(GetGameVersionString(GAME_VERSION)),
			FText::FromString(GetGameVersionString(hostVersion))
		);

		
		Spawn2DSound("UI", "PopupCannot");
		return;
	}
	
	if (_chosenSession.IsValid()) {
		gameInstance()->JoinGame(_chosenSession);
	}
	
	Spawn2DSound("UI", "UIWindowOpen");
}

void UMainMenuUI::Spawn2DSound(std::string groupName, std::string soundName)
{
	gameInstance()->Spawn2DSound(groupName, soundName);
}

void UMainMenuUI::OnClickJoinPasswordConfirm()
{
	PasswordPopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
	FString password = GetSessionValueString(SESSION_PASSWORD, _chosenSession.Session.SessionSettings);
	FString passwordInput = PasswordEditableText->GetText().ToString();

	if (password == passwordInput) {
		JoinMultiplayerGame();
	}
	else {
		gameInstance()->mainMenuPopup = LOCTEXT("InvalidPassword", "Invalid password.");
	}
}

#undef LOCTEXT_NAMESPACE