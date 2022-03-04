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
#include "PunButtonImageWithGlow.h"
#include "PunCity/PunGameMode.h"
#include "PunCity/MainMenuPlayerController.h"

/*
 * Steam API warning disable
 */
#ifdef PLATFORM_WINDOWS
 // so vs2015 triggers depracated warnings from standard C functions within the steam api, this wrapper makes the output log just ignore these since its clearly on crack.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)

#include <steam/steam_api.h>

#pragma warning(pop)
#endif

#endif

//--

#define LOCTEXT_NAMESPACE "LobbyUI"

void ULobbyUI::Init(UMainMenuAssetLoaderComponent* maimMenuAssetLoaderIn)
{
	_mainMenuAssetLoader = maimMenuAssetLoaderIn;
	
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
	
	SinglePlayerPortraitUI->SetVisibility(isSinglePlayer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SinglePlayerPortraitUI->PunInit(this, 0);
	

	FMapSettings mapSettings;
	if (gameInst->isSinglePlayer) {
		mapSettings = UPunGameInstance::GetSavedMap(true);
	}
	// Set default AI count
	mapSettings.aiCount = GetDefaultAICount(static_cast<MapSizeEnum>(mapSettings.mapSizeEnumInt));

	
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
			// GetMapSettings from gameInst since that is what was set by PreLobbySettings
			mapSettings = gameInst->GetMapSettings();
		}
	}


	//
	clientReadyState = false;
	LobbyReadyButton->OnClicked.AddDynamic(this, &ULobbyUI::OnClickReadyButton);


	LobbyBackButton->OnClicked.AddDynamic(this, &ULobbyUI::ReturnToMainMenu);
	LobbyStartGameButton->OnClicked.AddDynamic(this, &ULobbyUI::OnClickLobbyStartGameButton);

	LobbyChatInputBox->OnTextCommitted.AddDynamic(this, &ULobbyUI::OnChatInputBoxTextCommitted);
	
	LobbyChatContentRichText->SetText(FText());
	LobbyPlayerListBox->ClearChildren();
	LobbyPlayerListBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	
	LobbySettingsUI->InitLobbySettings(mapSettings);
	
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


	/*
	 * Player Logo
	 */
	LobbyChooseLogoOverlay->SetVisibility(ESlateVisibility::Collapsed);
	BUTTON_ON_CLICK(LobbyChooseLogoDoneButton, this, &ULobbyUI::OnClickChoosePlayerLogoCloseButton);

	BUTTON_ON_CLICK(ChooseFactionSectionButton, this, &ULobbyUI::OnClickChooseFactionSectionButton);
	BUTTON_ON_CLICK(ChooseFactionButton1, this, &ULobbyUI::OnClickChooseFaction0);
	BUTTON_ON_CLICK(ChooseFactionButton2, this, &ULobbyUI::OnClickChooseFaction1);

	ChooseFactionButtonGlow1->SetVisibility(ESlateVisibility::Collapsed);
	ChooseFactionButtonGlow2->SetVisibility(ESlateVisibility::Collapsed);
	
	ChooseFactionButton1->OnHovered.Clear();
	ChooseFactionButton1->OnHovered.AddUniqueDynamic(this, &ULobbyUI::OnHoverChooseFaction1);
	ChooseFactionButton2->OnHovered.Clear();
	ChooseFactionButton2->OnHovered.AddUniqueDynamic(this, &ULobbyUI::OnHoverChooseFaction2);

	ChooseFactionButton1->OnUnhovered.Clear();
	ChooseFactionButton1->OnUnhovered.AddUniqueDynamic(this, &ULobbyUI::OnUnhoverChooseFaction1);
	ChooseFactionButton2->OnUnhovered.Clear();
	ChooseFactionButton2->OnUnhovered.AddUniqueDynamic(this, &ULobbyUI::OnUnhoverChooseFaction2);

	
	BUTTON_ON_CLICK(ChooseIconSectionButton, this, &ULobbyUI::OnClickChooseIconSectionButton);
	BUTTON_ON_CLICK(ChooseIconColorSectionButton, this, &ULobbyUI::OnClickChooseIconColorSectionButton);
	BUTTON_ON_CLICK(ChooseCharacterSectionButton, this, &ULobbyUI::OnClickChooseCharacterSectionButton);

	SetPlayerSettingsTab(0);


	//! Logos
	ChooseIconWrapBox->ClearChildren();
	const TArray<UTexture2D*>& playerLogos = _mainMenuAssetLoader->PlayerLogos;
	for (int32 i = 0; i < playerLogos.Num(); i++)
	{
		auto buttonImage = AddWidget<UPunButtonImageWithGlow>(UIEnum::ChooseLogoElement);
		buttonImage->Setup(i, CallbackEnum::ChoosePlayerLogo, this);
		buttonImage->Image2->GetDynamicMaterial()->SetVectorParameterValue("ColorForeground", FLinearColor(0.97, 0.97, 0.97));
		buttonImage->Image2->GetDynamicMaterial()->SetTextureParameterValue("Logo", playerLogos[i]);
		ChooseIconWrapBox->AddChild(buttonImage);
	}

	//! Take Colors from buttons
	{
		TArray<FLinearColor> backgroundColors;
		TArray<FLinearColor> foregroundColors;
		for (int32 i = 0; i < ChooseIconColorWrapBox1->GetChildrenCount(); i++)
		{
			TArray<UImage*> images;
			FindChildrenRecursive<UImage>(CastChecked<UPanelWidget>(ChooseIconColorWrapBox1->GetChildAt(i)), images);
			check(images.Num() == 2);

			backgroundColors.Add(images[0]->ColorAndOpacity);
			foregroundColors.Add(images[1]->ColorAndOpacity);
			
			//UImage* image = FindChildRecursive<UImage>(CastChecked<UPanelWidget>(ChooseIconColorWrapBox1->GetChildAt(i)));
			//colors.Add(image->ColorAndOpacity);
		}

		auto fillColorBox = [&](UWrapBox* wrapBox, CallbackEnum callbackEnum)
		{
			wrapBox->ClearChildren();
			for (int32 i = 0; i < backgroundColors.Num(); i++) {
				auto buttonImage = AddWidget<UPunButtonImage>(UIEnum::ChooseColorElement);
				buttonImage->Setup(i, callbackEnum, this);
				buttonImage->Image1->SetColorAndOpacity(backgroundColors[i]);
				buttonImage->Image2->SetColorAndOpacity(foregroundColors[i]);
				wrapBox->AddChild(buttonImage);
			}
		};
		
		fillColorBox(ChooseIconColorWrapBox1, CallbackEnum::ChoosePlayerColor1);
		//fillColorBox(ChooseIconColorWrapBox2, CallbackEnum::ChoosePlayerColor2);
	}

	//! Characters
	ChooseCharacterWrapBox->ClearChildren();

	// TODO: use manual arrangement
	//TArray<FString> characterNames
	//{
	//	"DefaultWhiteMale",
	//	"DefaultWhiteFemale",
	//};

	int32 index = 0;
	for (const auto& it : _mainMenuAssetLoader->PlayerCharacters)
	{
		auto buttonImage = AddWidget<UPunButtonImageWithGlow>(UIEnum::ChooseCharacterElement);
		buttonImage->Setup(index, CallbackEnum::ChoosePlayerCharacter, this);
		buttonImage->name = it.Key;
		buttonImage->Image1->GetDynamicMaterial()->SetTextureParameterValue("Character", _mainMenuAssetLoader->GetPlayerCharacter(it.Key));
		ChooseCharacterWrapBox->AddChild(buttonImage);
		index++;
	}


	
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
		SetText(MultiplayerLobbyTitle, LOCTEXT("MultiplayerLobbyTitle_Load", "<Title>Load Multiplayer Game Lobby</>"));
		GenerateWorldOverlay->SetVisibility(ESlateVisibility::Collapsed);
	} else {
		if (gameInst->isMultiplayer()) {
			SetText(MultiplayerLobbyTitle, LOCTEXT("MultiplayerLobbyTitle_Multiplayer", "<Title>Multiplayer Lobby</>"));
		} else {
			SetText(MultiplayerLobbyTitle, LOCTEXT("MultiplayerLobbyTitle_SinglePlayer", "<Title>Single Player</>"));
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
			LobbyStartGame_Multiplayer();
			return;
		}

		// Cancel start if someone unready
		if (!gameInstance()->IsAllPlayersReady()) {
			firstController->SendChat_ToServer("", LOCTEXT("Not all players are ready", "Not all players are ready...").ToString());
			firstController->SendChat_ToServer("", LOCTEXT("Start canceled", "Start canceled...").ToString());
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
		StartGameButtonText->SetText(gameInstance()->IsAllPlayersReady() ? LOCTEXT("Start Now", "Start Now") : LOCTEXT("Start Game", "Start Game"));
	}

	/*
	 * World pregen
	 */
	if (_worldInitCompletedFuture.IsReady())
	{
		bool isCompleted = static_cast<bool>(_worldInitCompletedFuture.Get());
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
				GameRand::SetRandUsageValid(true);
				GameRand::ResetStateToTickCount(0);
				uint8 succeed = terrainGenerator->NextInitStage();
				GameRand::SetRandUsageValid(false);
				return succeed;
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


	// Load Player Settings if needed
	if (!_loadedPlayerInfo && GetFirstController()->PlayerState) {
		_loadedPlayerInfo = true;
		LoadPlayerInfo();
	}

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
			gameInstance()->isReturningToLobbyList = false;
			GetFirstController()->ClientTravel("/Game/Maps/MainMenu", TRAVEL_Absolute);
			gameInstance()->EnsureSessionDestroyed(false);
		}
		else {
			gameInstance()->isReturningToLobbyList = true; // return to lobby list if it is a multiplayer game
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
				if (IsValidPun(controller->PlayerState)) { // PlayerState might need time to be filled on client
					FString playerName = controller->PlayerState->GetPlayerName();
					controller->SendChat_ToServer(playerName, text.ToString());
				}
				LobbyChatInputBox->SetText(FText());
			}
		}

		FInputModeGameAndUI_Pun inputModeData;
		inputModeData.SetWidgetToFocus(LobbyChatInputBox->TakeWidget());
		inputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		GetFirstController()->SetInputMode(inputModeData);
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
		_isMapReady = UPunGameInstance::HasSavedMapWithSettings(mapSettings);
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
			gameInstance()->CachePlayerInfos();
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
		GetFirstController()->SendChat_ToServer("", LOCTEXT("Start canceled", "Start canceled...").ToString());

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

	GetFirstController()->SendChat_ToServer("", LOCTEXT("Starting in 10", "Starting in 10...").ToString());

	//SetText(StartGameButtonText, "Cancel");
	isCountingDown = true;
	countdownTime = 10.98f;
#if WITH_EDITOR
	countdownTime = 3.98f;
#endif

	// Stop anyone from joining the server
	gameInstance()->ServerOnStartedGame();
}

void ULobbyUI::LobbyStartGame_Multiplayer()
{
	APunGameMode* gameMode = CastChecked<APunGameMode>(UGameplayStatics::GetGameMode(this));
	
	// If this is loaded, fill in the currentPlayer with lastPlayer if there is no currentPlayer
	if (gameInstance()->IsLoadingSavedGame())
	{
		GameSaveInfo saveInfo = gameInstance()->GetSavedGameToLoad();

		// TODO: "Old "
		//const TArray<FPlayerInfo>& names = gameInstance()->playerInfoList();
		//for (int32 i = 0; i < names.Num(); i++)
		//{
		//	if (names[i].name.IsEmpty() && !saveInfo.GetLastPlayerName(i).IsEmpty()) {
		//		gameInstance()->SetPlayerNameF(i, "Old " + saveInfo.GetLastPlayerName(i));
		//	}
		//}
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
	gameInstance()->CachePlayerInfos();

	GetWorld()->ServerTravel("/Game/Maps/GameMap");

	GameStartBlocker->SetVisibility(ESlateVisibility::Visible);
}


void ULobbyUI::UpdateLobbyUI()
{
	auto gameInst = gameInstance();
	const TArray<FPlayerInfo>& names = gameInst->playerInfoList();

	//// DEBUG:
	//PUN_DEBUG2("lobbyPlayerNamesDirty gameInst->playerNames:");
	//for (FString name : names) {
	//	if (name.Len() > 0) PUN_DEBUG2(": %s", *name);
	//}

	GameSaveInfo saveInfo = gameInst->GetSavedGameToLoad();


	UpdatePlayerPortraitUI(SinglePlayerPortraitUI, 0, saveInfo);

	

	//LastPlayerColumnText->SetVisibility(saveInfo.IsValid() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	// Show player list
	for (int i = 0; i < names.Num(); i++)
	{
		//PUN_DEBUG2("names %s", *names[i]);

		if (LobbyPlayerListBox->GetChildrenCount() <= i) {
			auto widget = AddWidget<UPlayerListElementUI>(UIEnum::PlayerListElement);
			widget->PunInit(this, i);
			PUN_CHECK(widget);

			LobbyPlayerListBox->AddChild(widget);
		}

		UPlayerListElementUI* element = CastChecked<UPlayerListElementUI>(LobbyPlayerListBox->GetChildAt(i));
		
		UpdatePlayerPortraitUI(element, i, saveInfo);

		element->SetVisibility(ESlateVisibility::Visible);
	}
	for (int32 i = names.Num(); i < LobbyPlayerListBox->GetChildrenCount(); i++) {
		LobbyPlayerListBox->GetChildAt(i)->SetVisibility(ESlateVisibility::Collapsed);
	}


	/*
	 * Sync Debug
	 */
	DebugText->SetVisibility(ESlateVisibility::Collapsed);
	
#if UE_BUILD_DEBUG
	APunGameMode* gameMode = Cast<APunGameMode>(UGameplayStatics::GetGameMode(this));
	if (gameMode && gameInst->saveSystem().IsLobbyLoadingSavedGame())
	{
		const TArray<APunBasePlayerController*>& controllers = gameMode->ConnectedControllers();

		DebugText->SetVisibility(ESlateVisibility::Visible);
		TArray<FText> args;

		for (int32 i = 0; i < controllers.Num(); i++) 
		{
			int32 controllerPlayerId = controllers[i]->controllerPlayerId();
			args.Add(FText::Format(
				INVTEXT("[{0}] {1} bps ... {2} / {3} \n"),
				TEXT_NUM(i),
				TEXT_NUM(controllers[i]->GetDataSyncBytesPerSecond()),
				TEXT_NUM(gameInst->clientPacketsReceived[controllerPlayerId != -1 ? controllerPlayerId : 0]),
				TEXT_NUM(gameInst->saveSystem().totalPackets())
			));
		}
		
		DebugText->SetText(JOINTEXT(args));
	}
#endif
}

static bool SteamID64Equals(uint64 steamId64A, uint64 steamId64B) {
	return  CSteamID(steamId64A).GetAccountID() == CSteamID(steamId64B).GetAccountID();
}

void ULobbyUI::UpdatePlayerPortraitUI(UPlayerListElementUI* element, int32 playerId, const GameSaveInfo& saveInfo)
{
	int32 i = playerId;

	auto gameInst = gameInstance();
	const TArray<FPlayerInfo>& playerInfos = gameInst->playerInfoList();

	
	element->PlayerName->SetVisibility(ESlateVisibility::Collapsed);
	//element->FactionName->SetVisibility(ESlateVisibility::Collapsed);

	element->BottomBlackFade->SetVisibility(ESlateVisibility::Collapsed);
	element->FactionBackground->SetVisibility(ESlateVisibility::Collapsed);
	element->PlayerLogoForeground->SetVisibility(ESlateVisibility::Collapsed);
	element->PlayerLogoBackground->SetVisibility(ESlateVisibility::Collapsed);
	element->PlayerCharacterImage->SetVisibility(ESlateVisibility::Collapsed);

	element->PlayerLogoChangeButton->SetVisibility(ESlateVisibility::Collapsed);

	element->PlayerKickButton->SetVisibility(ESlateVisibility::Collapsed);

	element->PlayerHighlight->SetVisibility(ESlateVisibility::Collapsed); // TODO: Use Later???
	element->ReadyParent->SetVisibility(ESlateVisibility::Collapsed);

	element->SaveGameTransferBar->SetVisibility(ESlateVisibility::Collapsed);
	element->SaveGameTransferText->SetVisibility(ESlateVisibility::Collapsed);

	element->EmptyText->SetVisibility(ESlateVisibility::Collapsed);
	element->PreviousPlayerText->SetVisibility(ESlateVisibility::Collapsed);
	
	
	if (gameInst->playerConnectedStates.Num() > 0 &&
		gameInst->playerConnectedStates[i])
	{
		//PUN_DEBUG2("playerConnectedStates true");
		element->playerName = playerInfos[i].name.ToString();
		element->PlayerName->SetText(FText::FromString(
			TrimStringF_Dots(playerInfos[i].name.ToString(), 12) +
			(gameInst->isMultiplayer() && gameInst->hostPlayerId == i ? LOCTEXT("(Host)", "(Host)").ToString() : "")
		));
		element->PlayerName->SetVisibility(ESlateVisibility::Visible);

		//element->FactionName->SetText(GetFactionInfo(playerInfos[i].factionEnum()).name);
		//element->FactionName->SetVisibility(ESlateVisibility::Visible);

		element->FactionBackground->GetDynamicMaterial()->SetTextureParameterValue("Image", _mainMenuAssetLoader->GetFactionImage(GetFactionInfoInt(playerInfos[i].factionIndex).internalName));
		element->PlayerLogoForeground->GetDynamicMaterial()->SetTextureParameterValue("Logo", _mainMenuAssetLoader->PlayerLogos[playerInfos[i].logoIndex]);
		element->PlayerLogoForeground->GetDynamicMaterial()->SetVectorParameterValue("ColorForeground", playerInfos[i].logoColorForeground);
		element->PlayerLogoBackground->GetDynamicMaterial()->SetVectorParameterValue("ColorBackground", playerInfos[i].logoColorBackground);
		element->PlayerCharacterImage->GetDynamicMaterial()->SetTextureParameterValue("Character", _mainMenuAssetLoader->PlayerCharacters[playerInfos[i].portraitName]);

		element->BottomBlackFade->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		element->FactionBackground->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		element->PlayerLogoForeground->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		element->PlayerLogoBackground->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		element->PlayerCharacterImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);


		// if this is the ui controlling player, allow clicking too ready from this button
		// Note:  GetFirstController()->PlayerState doesn't work on clients
		if (i == GetFirstController()->controllerPlayerId())
		{
			element->PlayerReadyButton->OnClicked.RemoveAll(this);
			element->PlayerReadyButton->OnClicked.AddDynamic(this, &ULobbyUI::OnClickReadyButton);
			element->PlayerReadyButton->SetColorAndOpacity(FLinearColor(1, 1, 1, 1));
			element->PlayerReadyButton->SetVisibility(ESlateVisibility::Visible);

			if (!gameInst->saveSystem().IsLobbyLoadingSavedGame()) {
				element->PlayerLogoChangeButton->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else {
			element->PlayerReadyButton->OnClicked.RemoveAll(this);
			element->PlayerReadyButton->SetColorAndOpacity(FLinearColor(1, 1, 1, 0.5));
			element->PlayerReadyButton->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

		if (saveInfo.IsValid())
		{
			if (!SteamID64Equals(saveInfo.playerNames[i].steamId64, playerInfos[i].steamId64))
			{
				FText name = saveInfo.playerNames[i].name;
				
				element->PreviousPlayerText->SetVisibility(ESlateVisibility::HitTestInvisible);
				element->PreviousPlayerText->SetText(FText::Format(
					LOCTEXT("PreviousPlayerWarning", "Previously\n{0}"),
					!name.IsEmpty() ? name : LOCTEXT("Empty", "Empty")
				));
			}
		}
		

		bool showKicker = gameInst->IsServer(this) && gameInst->hostPlayerId != i;
		element->PlayerKickButton->SetVisibility(showKicker ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		// Show player
		//element->PlayerHighlight->SetVisibility(ESlateVisibility::SelfHitTestInvisible);


		if (gameInst->isMultiplayer())
		{
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
				int32 totalPackets = gameInst->saveSystem().totalPackets();
				totalPackets = std::max(totalPackets, 1); // Prevent divided by zero crash

				int32 percentLoaded = gameInst->clientPacketsReceived[i] * 100 / totalPackets;
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
					SetText(element->SaveGameTransferText, TEXT_PERCENT(percentLoaded));
				}
			}
			else
			{
				showReadyCheckBox();
			}

		}

		element->EmptySelectButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		// Hide player
		element->playerName = "";

		element->EmptySelectButton->SetVisibility(ESlateVisibility::Visible);
		element->EmptyText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}


void ULobbyUI::UpdatePreviewPlayerInfoDisplay()
{
	PlayerCharacterInfoUI->UpdatePlayerInfo(previewPlayerInfo,
		_mainMenuAssetLoader->GetFactionImage(GetFactionInfoInt(previewPlayerInfo.factionIndex).internalName),
		_mainMenuAssetLoader->PlayerLogos, 
		_mainMenuAssetLoader->PlayerCharacters
	);
}

void ULobbyUI::OnClickChoosePlayerLogoCloseButton()
{
	gameInstance()->Spawn2DSound("UI", "UIWindowClose");
	LobbyChooseLogoOverlay->SetVisibility(ESlateVisibility::Collapsed);

	GetFirstController()->SendPlayerInfo_ToServer(previewPlayerInfo);
	SavePlayerInfo();
}

void ULobbyUI::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum)
{
	if (callbackEnum == CallbackEnum::SelectEmptySlot) {
		UPlayerListElementUI* element = CastChecked<UPlayerListElementUI>(punWidgetCaller);
		GetFirstController()->TryChangePlayerId_ToServer(element->slotId);
	}
	else if (callbackEnum == CallbackEnum::LobbyChoosePlayerSettings) 
	{
		LobbyChooseLogoOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		CSteamID steamId(*(uint64*)GetFirstController()->PlayerState->GetUniqueId()->GetBytes());

		previewPlayerInfo = gameInstance()->GetPlayerInfoBySteamId(steamId.ConvertToUint64());

		UpdatePreviewPlayerInfoDisplay();
	}

	else if (callbackEnum == CallbackEnum::ChoosePlayerLogo)
	{
		auto buttonImage = CastChecked<UPunButtonImage>(punWidgetCaller);
		previewPlayerInfo.logoIndex = buttonImage->index;
		UpdatePreviewPlayerInfoDisplay();
	}
	else if (callbackEnum == CallbackEnum::ChoosePlayerColor1)
	{
		auto buttonImage = CastChecked<UPunButtonImage>(punWidgetCaller);
		previewPlayerInfo.logoColorBackground = buttonImage->Image1->ColorAndOpacity;
		previewPlayerInfo.logoColorForeground = buttonImage->Image2->ColorAndOpacity;
		
		UpdatePreviewPlayerInfoDisplay();
	}
	//else if (callbackEnum == CallbackEnum::ChoosePlayerColor2)
	//{
	//	auto buttonImage = CastChecked<UPunButtonImage>(punWidgetCaller);
	//	previewPlayerInfo.logoColorForeground = buttonImage->Image1->ColorAndOpacity;
	//	UpdatePreviewPlayerInfoDisplay();
	//}
	else if (callbackEnum == CallbackEnum::ChoosePlayerCharacter)
	{
		auto buttonImage = CastChecked<UPunButtonImage>(punWidgetCaller);
		previewPlayerInfo.portraitName = buttonImage->name;
		UpdatePreviewPlayerInfoDisplay();
	}
}

static FString GetSavePlayerInfoPath() {
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "PlayerInfo.dat";
}

void ULobbyUI::SavePlayerInfo()
{
	PunFileUtils::SaveFile(GetSavePlayerInfoPath(), [&](FArchive& Ar)
	{
		int32 saveVersion = SAVE_VERSION;
		Ar << saveVersion;
		
		FPlayerInfo playerInfo;
		if (gameInstance()->isMultiplayer()) {
			CSteamID steamId(*(uint64*)GetFirstController()->PlayerState->GetUniqueId()->GetBytes());
			playerInfo = gameInstance()->GetPlayerInfoBySteamId(steamId.ConvertToUint64());
		}
		else {
			playerInfo = gameInstance()->playerInfoList()[0];
		}
		
		Ar << playerInfo;
	});
}

void ULobbyUI::LoadPlayerInfo()
{
	PunFileUtils::LoadFile(GetSavePlayerInfoPath(), [&](FArchive& Ar)
	{
		int32 saveVersion;
		Ar << saveVersion;
		if (saveVersion == SAVE_VERSION)
		{
			FPlayerInfo playerInfo;
			Ar << playerInfo;

			CSteamID steamId(*(uint64*)GetFirstController()->PlayerState->GetUniqueId()->GetBytes());
			playerInfo.steamId64 = steamId.ConvertToUint64();

			GetFirstController()->SendPlayerInfo_ToServer(playerInfo);
		}
	});
}


#undef LOCTEXT_NAMESPACE 