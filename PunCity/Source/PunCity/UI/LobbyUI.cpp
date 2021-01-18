// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyUI.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "LobbyPlayerInfoUI.h"
#include "PunCity/PunPlayerController.h"
#include "PunCity/MainMenuPlayerController.h"

#include "UObject/ConstructorHelpers.h"
#include "Misc/DefaultValueHelper.h"
#include "PunCity/PunGameInstance.h"
#include "PlayerListElementUI.h"
#include "PunCity/PunGameMode.h"
#include "PunCity/MainMenuPlayerController.h"

#define LOCTEXT_NAMESPACE "LobbyUI"

void ULobbyUI::Init()
{
	AGameModeBase* gameMode = UGameplayStatics::GetGameMode(this);
	auto gameInst = gameInstance();
	
	/*
	 * Open the Lobby UI
	 * - clear any leftover playerData
	 */
	// Doesn't work...
	//if (gameMode) { // ResetPlayerCount if server
	//	gameInst->ResetPlayerCount();
	//}

	SetChildHUD(LobbySettingsUI);
	LobbySettingsUI->SetPreLobby(false);
	
	/*
	 * Singleplayer vs multiplayer
	 */
	bool isSinglePlayer = gameInstance()->isSinglePlayer;
	LobbySettingsUI->serverMapSettings.isSinglePlayer = isSinglePlayer;
	
	ESlateVisibility multiplayerUIVisible = isSinglePlayer ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible;
	PlayerListOverlay->SetVisibility(multiplayerUIVisible);
	ChatOverlay->SetVisibility(multiplayerUIVisible);
	LobbyReadyBox->SetVisibility(multiplayerUIVisible);

	FMapSettings mapSettings = FMapSettings::GetDefault(true);
	
	/*
	 * Loading Multiplayer Saves
	 */
	if (gameInst->isMultiplayer() &&
		gameInst->IsServer(this))
	{
		if (gameInst->IsLoadingSavedGame())
		{
			// Load Saved Game
			gameInst->saveSystem().LoadDataIntoCache();

			gameInst->saveSystem().ServerPrepareSync();
			gameInst->clientPacketsReceived[gameInst->hostPlayerId] = gameInst->saveSystem().totalPackets();

			// Set serverMapSettings to saveInfo
			mapSettings = gameInst->GetMapSettings();
		}
		else
		{
			// Creating a Multiplayer Game
			mapSettings = gameInst->GetMapSettings();
		}
	}


	//
	clientReadyState = false;
	LobbyReadyButton->OnClicked.AddDynamic(this, &ULobbyUI::OnClickReadyButton);

	//NameInputBox->OnTextChanged.AddDynamic(this, &ULobbyUI::PlayerNameChanged);
	//InitialAnimalsInputBox->OnTextChanged.AddDynamic(this, &ULobbyUI::InputBoxChange_InitialAnimals);

	LobbyBackButton->OnClicked.AddDynamic(this, &ULobbyUI::ReturnToMainMenu);
	LobbyStartGameButton->OnClicked.AddDynamic(this, &ULobbyUI::OnClickLobbyStartGameButton);

	LobbyChatInputBox->OnTextCommitted.AddDynamic(this, &ULobbyUI::OnChatInputBoxTextCommitted);
	
	LobbyChatContentRichText->SetText(FText());
	LobbyPlayerListBox->ClearChildren();

	
	LobbySettingsUI->Init(mapSettings);
	
	//// Only server can change settings
	//if (GetFirstController()->GetLocalRole() == ROLE_Authority)
	//{
	//	LobbyPasswordInputBox->OnTextCommitted.AddDynamic(this, &ULobbyUI::OnLobbyPasswordInputBoxTextCommitted);
	//	
	//	LobbyMapSeedInputBox->OnTextCommitted.AddDynamic(this, &ULobbyUI::OnLobbyMapSeedInputBoxTextCommitted);

	//	LobbyMapSizeDropdown->OnSelectionChanged.AddDynamic(this, &ULobbyUI::OnLobbyMapSizeDropdownChanged);
	//	// "Small" If adding another map size, also change the options on UI editor
	//	// Note: Using UI Editor for this to prevent ::Direct selection issue

	//	LobbySeaLevelDropdown->OnSelectionChanged.AddDynamic(this, &ULobbyUI::OnLobbySeaLevelDropdownChanged);
	//	LobbyMoistureDropdown->OnSelectionChanged.AddDynamic(this, &ULobbyUI::OnLobbyMoistureDropdownChanged);
	//	LobbyTemperatureDropdown->OnSelectionChanged.AddDynamic(this, &ULobbyUI::OnLobbyTemperatureDropdownChanged);
	//	LobbyMountainDensityDropdown->OnSelectionChanged.AddDynamic(this, &ULobbyUI::OnLobbyMountainDensityDropdownChanged);

	//	LobbyAICountDropdown->OnSelectionChanged.AddDynamic(this, &ULobbyUI::OnLobbyAICountDropdownChanged);
	//	RefreshAICountDropdown();
	//	LobbyAICountDropdown->SetSelectedIndex(LobbyAICountDropdown->GetOptionCount() - 1);

	//	LobbyDifficultyDropdown->OnSelectionChanged.AddDynamic(this, &ULobbyUI::OnLobbyDifficultyDropdownChanged);
	//}

	// Generate World
	GenerateWorldBar->SetVisibility(ESlateVisibility::Collapsed);
	GenerateWorldButton->SetVisibility(ESlateVisibility::Visible);
	GenerateWorldButton->OnClicked.AddDynamic(this, &ULobbyUI::OnClickGenerateWorldButton);
	WorldReadyText->SetVisibility(ESlateVisibility::Collapsed);

	CancelWorldGenerationOverlay->SetVisibility(ESlateVisibility::Collapsed);

	CheckMapReady();

	// Popup
	LobbyPopupCloseButton->OnClicked.AddDynamic(this, &ULobbyUI::OnClickPopupCloseButton);



	gameInstance()->Spawn2DSound("UI", "UIWindowOpen");

	PUN_DEBUG2("LobbyUI Open");


	gameInstance()->lobbyChatDirty = true;
	gameInstance()->lobbyChatMessages.Empty();
	gameInstance()->lobbyChatPlayerNames.Empty();

	LobbySettingsUI->SendMapSettings();

	UpdateLobbyUI();
}

void ULobbyUI::Tick()
{
	auto gameInst = gameInstance();
	gameInst->PunTick();
	
	auto firstController = LobbySettingsUI->GetFirstController();

	if (firstController->isStartingGame) {
		//PlayerCountText->SetText(FText::FromString(FString("Starting game...")));
		GameStartBlocker->SetVisibility(ESlateVisibility::Visible);
		return;
	}
	
	//PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("Players: %d"), firstController->playerNames.Num())));

	// Don't show start game button for client
	LobbyStartGameButton->SetVisibility(firstController->GetLocalRole() == ROLE_Authority ? ESlateVisibility::Visible : ESlateVisibility::Hidden);


	bool isLoading = gameInst->saveSystem().HasSyncData();
	if (isLoading) {
		SetText(MultiplayerLobbyTitle, "<Title>Load Multiplayer Game Lobby</>");
		GenerateWorldOverlay->SetVisibility(ESlateVisibility::Collapsed);
	} else {
		if (gameInst->isMultiplayer()) {
			SetText(MultiplayerLobbyTitle, "<Title>Multiplayer Lobby</>");
		} else {
			SetText(MultiplayerLobbyTitle, "<Title>Single Player</>");
		}
		GenerateWorldOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	LobbySettingsUI->SettingsBackgroundImage->SetVisibility(serverMapSettings().isMultiplayer() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);


	/*
	 * Chat
	 */
	if (gameInst->lobbyChatDirty) {
		gameInst->lobbyChatDirty = false;

		FString result;
		// sort with first message coming last...
		for (int32 i = 0; i < gameInst->lobbyChatPlayerNames.Num(); i++) {
			result.Append("\n");
			if (!gameInst->lobbyChatPlayerNames[i].IsEmpty()) {
				result.Append("<LobbyChatName>").Append(TrimStringF_Dots(gameInst->lobbyChatPlayerNames[i], 15)).Append(":</> ");
			}
			result.Append("<LobbyChat>").Append(gameInst->lobbyChatMessages[i]).Append("</>");
		}
		LobbyChatContentRichText->SetText(FText::FromString(result));
		LobbyChatScrollBox->ScrollToEnd();

		//LobbyChatScrollBox->ScrollWidgetIntoView()

		// Sound
		if (gameInst->lobbyChatPlayerNames.Num() > 0)
		{
			// Don't sound on start game chat text (already did when button was pressed)
			if (gameInst->lobbyChatPlayerNames.Last().IsEmpty() &&
				gameInst->lobbyChatMessages.Last() == "Starting in 10...")
			{}
			else
			{
				gameInstance()->Spawn2DSound("UI", "Chat");
			}
		}
	}

	/*
	 * Settings update
	 */
	LobbySettingsUI->Tick(isLoading);

	//auto setServerVsClientUI = [&](UWidget* serverWidget, UTextBlock* clientWidget, FString clientString)
	//{
	//	// Not loading and is server, show serverWidget to allow settings change
	//	if (!isLoading && firstController->GetLocalRole() == ROLE_Authority)
	//	{
	//		serverWidget->SetVisibility(ESlateVisibility::Visible);
	//		clientWidget->SetVisibility(ESlateVisibility::Collapsed);
	//	}
	//	else 
	//	{
	//		serverWidget->SetVisibility(ESlateVisibility::Collapsed);
	//		clientWidget->SetVisibility(ESlateVisibility::Visible);

	//		if (clientString != clientWidget->GetText().ToString()) {
	//			PUN_DEBUG2("LobbyClientText SetText %s", *clientString);
	//			clientWidget->SetText(FText::FromString(clientString));
	//		}
	//	}
	//};

	FMapSettings mapSettings = gameInst->GetMapSettings();
	//setServerVsClientUI(LobbyMapSeedInputBox, LobbyMapSeedText, mapSettings.mapSeed);
	//setServerVsClientUI(LobbyMapSizeDropdown, LobbyMapSizeText, MapSizeNames[mapSettings.mapSizeEnumInt]);
	//setServerVsClientUI(LobbySeaLevelDropdown, LobbySeaLevelText, MapSettingsLevelNames[static_cast<int>(mapSettings.mapSeaLevel)]);
	//setServerVsClientUI(LobbyMoistureDropdown, LobbyMoistureText, MapMoistureNames[static_cast<int>(mapSettings.mapMoisture)]);
	//setServerVsClientUI(LobbyTemperatureDropdown, LobbyTemperatureText, MapSettingsLevelNames[static_cast<int>(mapSettings.mapTemperature)]);
	//setServerVsClientUI(LobbyMountainDensityDropdown, LobbyMountainDensityText, MapSettingsLevelNames[static_cast<int>(mapSettings.mapMountainDensity)]);

	//
	//setServerVsClientUI(LobbyAICountDropdown, LobbyAICountText, FString::FromInt(mapSettings.aiCount));
	//setServerVsClientUI(LobbyDifficultyDropdown, LobbyDifficultyText, DifficultyLevelNames[static_cast<int>(mapSettings.difficultyLevel)]);

	

	// Map size or seed was changed, cancel existing world gen and ready state
	if (clientLastMapSettings != mapSettings) 
	{
		//PUN_LOG("mapSettings changed: %s -> %s", ToTChar(clientLastMapSettings.ToString()), ToTChar(mapSettings.ToString()));
		
		// Settings changed after it was initialized, unready everyone
		if (clientLastMapSettings.isInitialized()) {
			Unready();
		}

		// Check if we should show map as ready...
		CheckMapReady();

		// Cancel any world generation in progress
		if (_isGeneratingTerrain) {
			_isCancelingWorldGeneration = true;
			
			AddPopup(LOCTEXT("HostChangedMapSettings_Pop", "Host changed map settings. Canceling the world generation in progress..."));
			gameInstance()->Spawn2DSound("UI", "ButtonClickInvalid");
		}
		
		clientLastMapSettings = mapSettings;
	}
	

	/*
	 * Send map settings every 2 sec
	 */
	if (UGameplayStatics::GetTimeSeconds(this) - lastSendMapSettingsTime > 2.0f)
	{
		LobbySettingsUI->SendMapSettings();
		lastSendMapSettingsTime = UGameplayStatics::GetTimeSeconds(this);
	}

	
	/*
	 * Start countdown
	 */
	
	if (isCountingDown)
	{
		SetText(StartGameButtonText, "Cancel");
		
		if (countdownTime < 0) {
			LobbyStartGame();
			return;
		}

		// Cancel start if someone unready
		if (!gameInstance()->IsAllPlayersReady()) {
			firstController->SendChat_ToServer("", FString("Not all players are ready..."));
			firstController->SendChat_ToServer("", FString("Start canceled..."));
			isCountingDown = false;
			//SetText(StartGameButtonText, "Start Game");
			return;
		}
		
		float lastCountdownTime = countdownTime;
		countdownTime -= UGameplayStatics::GetWorldDeltaSeconds(this);
		if (static_cast<int>(countdownTime) != static_cast<int>(lastCountdownTime)) {
			firstController->SendChat_ToServer("", FString::FromInt(static_cast<int32>(countdownTime)) + FString("..."));
		}
	}
	else
	{
		// Not counting down yet, but everyone is ready.. say start now
		SetText(StartGameButtonText, gameInstance()->IsAllPlayersReady() ? "Start Now" : "Start Game");
	}

	/*
	 * World pregen
	 */
	if (_worldInitCompletedFuture.IsReady())
	{
		bool isCompleted = _worldInitCompletedFuture.Get();
		int32 percentLoaded = _terrainGenerator->percentLoaded();
		PUN_LOG("World Future %d, %d percent", isCompleted, percentLoaded);
		_worldInitCompletedFuture.Reset();

		if (_isCancelingWorldGeneration) {
			_isCancelingWorldGeneration = false;
			_isGeneratingTerrain = false;
			return;
		}

		// Cancel terrain generation if needed
		if (_isWaitingToGoBackToMainMenu) {
			_isGeneratingTerrain = false;
			return;
		}
		
		if (!isCompleted) {
			GenerateWorldBar->GetDynamicMaterial()->SetScalarParameterValue("Fraction", percentLoaded / 100.0f);

			PunTerrainGenerator* terrainGenerator = _terrainGenerator.get();
			_worldInitCompletedFuture = Async(EAsyncExecution::Thread, [terrainGenerator]() {
				return terrainGenerator->NextInitStage();
			});
		}
		else {
			_isGeneratingTerrain = false;
			CheckMapReady();
		}
	}

	if (_isGeneratingTerrain) {
		GenerateWorldBar->SetVisibility(ESlateVisibility::HitTestInvisible);
		WorldReadyText->SetVisibility(ESlateVisibility::Collapsed);
		GenerateWorldButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	else {
		if (_isMapReady)
		{
			GenerateWorldBar->GetDynamicMaterial()->SetScalarParameterValue("Fraction", 1.0f);
			GenerateWorldBar->SetVisibility(ESlateVisibility::HitTestInvisible);
			WorldReadyText->SetVisibility(ESlateVisibility::HitTestInvisible);
			GenerateWorldButton->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			GenerateWorldBar->SetVisibility(ESlateVisibility::Collapsed);
			WorldReadyText->SetVisibility(ESlateVisibility::Collapsed);
			GenerateWorldButton->SetVisibility(ESlateVisibility::Visible);
		}
	}

	if (_isWaitingToGoBackToMainMenu) {
		if (!_isGeneratingTerrain) {
			ReturnToMainMenu();
		}
	}

	// mainMenuPopup shouldn't do anything for LobbyUI
	// This is clean up popups from HandleNetworkFailure (For example, client joining error shouldn't popup after we exited created room)
	gameInstance()->mainMenuPopup = FText();

	UpdateLobbyUI();

	// End Tick()
}


//void ULobbyUI::InputBoxChange_InitialAnimals(const FText& text)
//{
//	int32 result = 0;
//	bool success = FDefaultValueHelper::ParseInt(text.ToString(), result);
//	if (success) {
//		PunSettings::Set("InitialAnimals", result);
//	}
//}

void ULobbyUI::ReturnToMainMenu()
{
	_LOG(PunNetwork, "ReturnToMainMenu (from Lobby)");
	
	// Clear any sync data
	gameInstance()->saveSystem().ClearSyncData();
	gameInstance()->ResetPlayerCount();
	
	if (_isGeneratingTerrain) {
		_isWaitingToGoBackToMainMenu = true;
		CancelWorldGenerationOverlay->SetVisibility(ESlateVisibility::Visible);
	}
	else {

		if (gameInstance()->isSinglePlayer) {
			GetFirstController()->ClientTravel("/Game/Maps/MainMenu", TRAVEL_Absolute);
			gameInstance()->EnsureSessionDestroyed(false);
		}
		else {
			gameInstance()->isReturningToLobbyList = gameInstance()->isMultiplayer(); // return to lobby list if it is a multiplayer game
			gameInstance()->EnsureSessionDestroyed(true);
		}
	}
}


void ULobbyUI::OnChatInputBoxTextCommitted(const FText& text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::Type::OnEnter) 
	{
		auto controller = GetFirstController();

		if (!text.ToString().IsEmpty())
		{
			// Special cases
			if (text.ToString() == "GetInfo" &&
				gameInstance()->IsServer(this))
			{
				IOnlineSessionPtr sessionPtr = gameInstance()->sessionInterface;
				sessionPtr->DumpSessionState();
				
				std::stringstream ss;


				PrintSessionState(ss, sessionPtr);
				
				//ss << "Session Info [Debug]:\n";
				//ss << "GetNumSessions: " << sessionPtr->GetNumSessions() << "\n";

				//FOnlineSessionSettings* sessionSettings = sessionPtr->GetSessionSettings(PUN_SESSION_NAME);
				//PrintSessionSettings(ss, *sessionSettings);

				//FString connectInfo;
				//sessionPtr->GetResolvedConnectString(PUN_SESSION_NAME, connectInfo);
				//ss << "GetResolvedConnectString: " << ToStdString(connectInfo);
				//ss << "\n\n";

				

				FWorldContext& context = gameInstance()->GetEngine()->GetWorldContextFromWorldChecked(controller->GetWorld());
				ss << "TravelURL:" << ToStdString(context.TravelURL);
				ss << "\n";
				ss << "TravelType: " << context.TravelType << "\n";
				ss << "LastURL:\n";
				PrintURL(ss, context.LastURL);
				ss << "\n";

				//sessionInterface->GetResolvedConnectString(PUN_SESSION_NAME, travelURL)
				//
				controller->SendChat_ToServer("", ToFString(ss.str()));
				LobbyChatInputBox->SetText(FText());
			}
			// Send Chat To Server
			else {
				FString playerName = controller->PlayerState->GetPlayerName();
				controller->SendChat_ToServer(playerName, text.ToString());
				LobbyChatInputBox->SetText(FText());
			}
		}

		FInputModeGameAndUI_Pun inputModeData;
		inputModeData.SetWidgetToFocus(LobbyChatInputBox->TakeWidget());
		inputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		controller->SetInputMode(inputModeData);
	}
}



void ULobbyUI::Unready() {
	// Settings change, unready everyone
	clientReadyState = false;
	LobbyReadyFill->SetVisibility(ESlateVisibility::Collapsed);
	GetFirstController()->SendReadyStatus_ToServer(clientReadyState);
}

void ULobbyUI::CheckMapReady()
{
	PUN_DEBUG2("CheckMapReady");

	// Called when: ready button, settings changed, worldGen done
	
	auto mapSettings = gameInstance()->GetMapSettings();

	if (gameInstance()->IsLoadingSavedGame()) {
		_isMapReady = true;
	} else {
		_isMapReady = PunTerrainGenerator::HasSavedMap(mapSettings);
	}
}

void ULobbyUI::OnClickReadyButton()
{
	// Not ready
	auto& saveSys = gameInstance()->saveSystem();
	if (saveSys.NeedSyncData())
	{
		AddPopup(LOCTEXT("WaitDataSync_Pop", "Please wait for data sync to complete."));
		gameInstance()->Spawn2DSound("UI", "ButtonClickInvalid");
		return;
	}

	/*
	 * Map
	 */
	CheckMapReady();
	if (_isMapReady) {
		clientReadyState = !clientReadyState;
	} else {
		AddPopup(LOCTEXT("NeedWorldGen_Pop", "Please generate the world before clicking ready."));
		gameInstance()->Spawn2DSound("UI", "ButtonClickInvalid");
		clientReadyState = false;
	}
	
	GetFirstController()->SendReadyStatus_ToServer(clientReadyState);
	LobbyReadyFill->SetVisibility(clientReadyState ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	gameInstance()->Spawn2DSound("UI", "ButtonClick2");
}


void ULobbyUI::OnClickLobbyStartGameButton()
{
	// Single Player
	if (gameInstance()->isSinglePlayer)
	{
		// Map
		CheckMapReady();
		if (_isMapReady) {
			// Travel
			GetWorld()->ServerTravel("/Game/Maps/GameMap");
			GameStartBlocker->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			AddPopup(LOCTEXT("NeedWorldGen_Pop", "Please generate the world before clicking ready."));
			gameInstance()->Spawn2DSound("UI", "ButtonClickInvalid");
		}
		return;
	}
	
	// Click while counting down, cancel the game start
	if (isCountingDown) {
		//SetText(StartGameButtonText, "Start Game");
		isCountingDown = false;
		GetFirstController()->SendChat_ToServer("", FString("Start canceled..."));

		// Allow players to join the server again
		gameInstance()->UpdateSession();
		return;
	}
	
	// If not everyone is ready, do not allow the game to start
	if (!gameInstance()->IsAllPlayersReady()) {
		AddPopup(LOCTEXT("NeedPlayersReady_Pop", "Not all players are ready. Failed to start the game."));
		gameInstance()->Spawn2DSound("UI", "ButtonClickInvalid");

		// Allow players to join the server again
		gameInstance()->UpdateSession();
		return;
	}

	gameInstance()->Spawn2DSound("UI", "ButtonClick2");

	GetFirstController()->SendChat_ToServer("", FString("Starting in 10..."));

	//SetText(StartGameButtonText, "Cancel");
	isCountingDown = true;
	countdownTime = 10.98f;
#if WITH_EDITOR
	countdownTime = 3.98f;
#endif

	// Stop anyone from joining the server
	gameInstance()->ServerOnStartedGame();
}

void ULobbyUI::LobbyStartGame()
{
	APunGameMode* gameMode = CastChecked<APunGameMode>(UGameplayStatics::GetGameMode(this));
	
	// If this is loaded, fill in the currentPlayer with lastPlayer if there is no currentPlayer
	if (gameInstance()->IsLoadingSavedGame())
	{
		GameSaveInfo saveInfo = gameInstance()->GetSavedGameToLoad();
		
		const TArray<FString>& names = gameInstance()->playerNamesF();
		for (int32 i = 0; i < names.Num(); i++)
		{
			if (names[i].IsEmpty() && !saveInfo.GetLastPlayerName(i).IsEmpty()) {
				gameInstance()->SetPlayerNameF(i, "Old " + saveInfo.GetLastPlayerName(i));
			}
		}
	}

	// Set targetPlayerCount ... Upon loading a new map, we wait for all players to connect (using targetPlayerCount) before proceeding.
	gameInstance()->targetPlayerCount = gameInstance()->playerCount();
	gameInstance()->UnreadyAll(); // Unready all, so readyStates can be used to determine if the player is loaded
	gameMode->Server_SyncPlayerStateToAllControllers();
	
	// Inform all client of game start so that we can show corresponding UI
	TArray<AActor*> found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(), found);
	for (int i = 0; i < found.Num(); i++) {
		auto controller = Cast<AMainMenuPlayerController>(found[i]);
		check(controller);
		PunSerializedData blob(true);
		serverMapSettings().Serialize(blob);
		controller->ServerStartGame(blob);
	}

	gameInstance()->DebugPunSync("Traveling to GameMap");
	gameInstance()->CachePlayerNames();

	GetWorld()->ServerTravel("/Game/Maps/GameMap");

	GameStartBlocker->SetVisibility(ESlateVisibility::Visible);
}


void ULobbyUI::UpdateLobbyUI()
{
	auto gameInst = gameInstance();
	const TArray<FString>& names = gameInst->playerNamesF();

	//// DEBUG:
	//PUN_DEBUG2("lobbyPlayerNamesDirty gameInst->playerNames:");
	//for (FString name : names) {
	//	if (name.Len() > 0) PUN_DEBUG2(": %s", *name);
	//}

	GameSaveInfo saveInfo = gameInst->GetSavedGameToLoad();

	LastPlayerColumnText->SetVisibility(saveInfo.IsValid() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	
	for (int i = 0; i < names.Num(); i++)
	{
		//PUN_DEBUG2("names %s", *names[i]);

		if (_playerListElements.Num() <= i) {
			_playerListElements.Add(AddWidget<UPlayerListElementUI>(UIEnum::PlayerListElement));
			_playerListElements[i]->PunInit(this, i);
			PUN_CHECK(_playerListElements[i]);

			LobbyPlayerListBox->AddChild(_playerListElements[i]);
		}
		

		UPlayerListElementUI* element = _playerListElements[i];


		auto setLastPlayerName = [&]()
		{
			FString lastPlayerName;
			if (saveInfo.IsValid()) {
				lastPlayerName = TrimStringF_Dots(saveInfo.GetLastPlayerName(i), 12);
			}

			if (!lastPlayerName.IsEmpty()) {
				SetTextF(element->LastPlayerName, lastPlayerName);
				element->LastPlayerName->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				return true;
			}
			
			element->LastPlayerName->SetVisibility(ESlateVisibility::Collapsed);
			return false;
		};
		

		if (gameInst->playerConnectedStates[i])
		{
			//PUN_DEBUG2("playerConnectedStates true");
			element->playerName = names[i];
			element->PlayerName->SetText(FText::FromString(TrimStringF_Dots(names[i], 12) + (gameInst->hostPlayerId == i ? "(Host)" : "")));
			element->PlayerName->SetVisibility(ESlateVisibility::Visible);

			// if this is the ui controlling player, allow clicking too ready from this button
			// Note:  GetFirstController()->PlayerState doesn't work on clients
			if (i == GetFirstController()->controllerPlayerId()) {
				element->PlayerReadyButton->OnClicked.RemoveAll(this);
				element->PlayerReadyButton->OnClicked.AddDynamic(this, &ULobbyUI::OnClickReadyButton);
				element->PlayerReadyButton->SetColorAndOpacity(FLinearColor(1, 1, 1, 1));
				element->PlayerReadyButton->SetVisibility(ESlateVisibility::Visible);
			}
			else {
				element->PlayerReadyButton->OnClicked.RemoveAll(this);
				element->PlayerReadyButton->SetColorAndOpacity(FLinearColor(1, 1, 1, 0.5));
				element->PlayerReadyButton->SetVisibility(ESlateVisibility::HitTestInvisible);
			}

			bool showKicker = gameInst->IsServer(this) && gameInst->hostPlayerId != i;
			element->PlayerKickButton->SetVisibility(showKicker ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

			// Show player
			element->PlayerHighlight->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			
			element->EmptyText->SetVisibility(ESlateVisibility::Collapsed);
			element->EmptySelectButton->SetVisibility(ESlateVisibility::Collapsed);

			auto showReadyCheckBox = [&]()
			{
				element->ReadyParent->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				element->PlayerReadyFill->SetVisibility(gameInst->IsPlayerReady(i) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

				element->SaveGameTransferBar->SetVisibility(ESlateVisibility::Collapsed);
				element->SaveGameTransferText->SetVisibility(ESlateVisibility::Collapsed);
			};

			// Ready vs Data transfer
			if (saveInfo.IsValid())
			{
				// when using percent >= 99... gameInst->clientPacketsReceived[i] == gameInst->saveSystem().totalPackets()
				// Client's side all ready checkboxes before 100%
				// Host's side all numbers
				int32 percentLoaded = gameInst->clientPacketsReceived[i] * 100 / gameInst->saveSystem().totalPackets();
				//if (gameInst->clientPacketsReceived[i] == gameInst->saveSystem().totalPackets())
				if (percentLoaded >= 99) {
					showReadyCheckBox();
				}
				else
				{
					// Data transfer
					element->SaveGameTransferBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					element->SaveGameTransferText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

					//int32 percentLoaded = gameInst->clientPacketsReceived[i] * 100 / gameInst->saveSystem().totalPackets();
					element->SaveGameTransferBar->GetDynamicMaterial()->SetScalarParameterValue("Fraction", percentLoaded / 100.0f);
					SetText(element->SaveGameTransferText, to_string(percentLoaded) + "%");

					element->ReadyParent->SetVisibility(ESlateVisibility::Collapsed);
					element->PlayerReadyFill->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			else
			{
				showReadyCheckBox();
			}

			setLastPlayerName();
			element->EmptyText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// Hide player
			element->playerName = "";
			element->PlayerName->SetVisibility(ESlateVisibility::Collapsed);

			element->PlayerKickButton->SetVisibility(ESlateVisibility::Collapsed);

			element->PlayerHighlight->SetVisibility(ESlateVisibility::Collapsed);
			element->ReadyParent->SetVisibility(ESlateVisibility::Collapsed);

			element->SaveGameTransferBar->SetVisibility(ESlateVisibility::Collapsed);
			element->SaveGameTransferText->SetVisibility(ESlateVisibility::Collapsed);
			
			element->EmptySelectButton->SetVisibility(ESlateVisibility::Visible);



			bool lastPlayerNameShown = setLastPlayerName();
			element->EmptyText->SetVisibility(lastPlayerNameShown ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
		}


		element->SetVisibility(ESlateVisibility::Visible);
	}
	for (int32 i = names.Num(); i < _playerListElements.Num(); i++) {
		_playerListElements[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULobbyUI::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum)
{
	if (callbackEnum == CallbackEnum::SelectEmptySlot) {
		UPlayerListElementUI* element = CastChecked<UPlayerListElementUI>(punWidgetCaller);
		GetFirstController()->TryChangePlayerId_ToServer(element->slotId);
	}
}


#undef LOCTEXT_NAMESPACE 