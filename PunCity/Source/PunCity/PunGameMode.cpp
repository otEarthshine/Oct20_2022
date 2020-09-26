// Pun Dumnernchanvanit's


#include "PunCity/PunGameMode.h"
#include "GameFramework/PlayerState.h"
#include "PunCity/PunUtils.h"
#include "PunCity/PunPlayerController.h"
#include "PunCity/MainMenuPlayerController.h"

APlayerController* APunGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	APlayerController* newController = AGameModeBase::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);

	
	if (newController->PlayerState) {
		PUN_DEBUG2("Login: %s", *newController->PlayerState->GetPlayerName());
	} else {
		PUN_DEBUG2("Login: No State");
	}

	return newController;
}

void APunGameMode::PostLogin(APlayerController* NewPlayer)
{
	FString playerName = NewPlayer->PlayerState ? NewPlayer->PlayerState->GetPlayerName() : "No State";
	PUN_DEBUG2("PostLogin: %s", *playerName);

	auto newController = CastChecked<APunBasePlayerController>(NewPlayer);
	auto gameInst = newController->gameInstance();
	auto punPlayer = Cast<APunPlayerController>(NewPlayer);

	gameInst->DebugPunSync("PostLogin Sync1");
	
	_ConnectedControllers.Add(newController);

	// Host logged in first. Later on, host may change to other position changing the hostPlayerId
	if (_ConnectedControllers.Num() == 1) 
	{
		gameInst->hostPlayerId = 0;

		// Cleanup for the first player
		if (!punPlayer) { // Don't ResetPlayerCount when starting play
			gameInst->ResetPlayerCount();

			// Clear game save when exiting to the lobby search. This prevent the next lobby from showing "Load"
			gameInst->saveSystem().ClearSyncData();
		}

		// If this is the GameMap, load the cached playerNames
		if (punPlayer) {
			gameInst->UseCachePlayerNames();
			gameInst->DebugPunSync("PostLogin Used Cache");
		}
	}

	// Cleanup for singlePlayer
	if (gameInst->isSinglePlayer) {
		gameInst->ResetPlayerCount();
	}
	
	
	int32 newPlayerId = gameInst->ConnectPlayer(playerName);
	newController->SetControllerPlayerId(newPlayerId);
	Server_SyncPlayerStateToAllControllers();

	gameInst->DebugPunSync("PostLogin Sync2");

	// Multiplayer Load Game, start data transfer
	if (gameInst->isMultiplayer() &&
		gameInst->IsLoadingSavedGame())
	{
		FBufferArchive SaveArchive;
		SaveArchive.SetIsSaving(true);
		SaveArchive.SetIsLoading(false);

		GameSaveInfo saveInfo = gameInst->GetSavedGameToLoad();
		saveInfo.Serialize(SaveArchive);

		_LOG(LogNetworkInput, "Sync - Server...SendSaveInfo_ToClient %s size:%d", *saveInfo.name, saveInfo.compressedDataSize);

		newController->SendSaveInfo_ToClient(SaveArchive);
	}

	auto mainMenuController = Cast<AMainMenuPlayerController>(NewPlayer);
	if (mainMenuController) {
		mainMenuController->SendChat_ToServer("", playerName + FString(" connected"));
	}
	
	if (punPlayer) {
		punPlayer->OnPostLogin();
	}

	AGameModeBase::PostLogin(NewPlayer);

	gameInst->UpdateSession();
}


void APunGameMode::Server_SyncPlayerStateToAllControllers()
{
	//PUN_DEBUG2("Server_SyncPlayerStateToAllControllers");
	
	const TArray<APunBasePlayerController*>& controllers = ConnectedControllers();
	for (auto controller : controllers) 
	{
		auto gameInst = controller->gameInstance();

		//PUN_DEBUG2("controller loop count:%d", gameInst->playerNamesF().Num());
		
		controller->SyncPlayersState_ToClient(gameInst->playerNamesF(),
												gameInst->playerReadyStates(),
												gameInst->playerConnectedStates,
												gameInst->clientPacketsReceived,
												gameInst->hostPlayerId, gameInst->serverTick());
	}
}



void APunGameMode::Logout(AController* Exiting)
{
	FString playerName = Exiting->PlayerState ? Exiting->PlayerState->GetPlayerName() : "No State";
	PUN_DEBUG2("Logout: %s", *playerName);

	auto exitingController = CastChecked<APunBasePlayerController>(Exiting);

	// MainMenuPlayerController
	auto mainMenuController = Cast<AMainMenuPlayerController>(exitingController);
	if (mainMenuController) {
		mainMenuController->SendChat_ToServer("", playerName + FString(" disconnected"));
	}
	
	_ConnectedControllers.Remove(exitingController);

	auto gameInst = exitingController->gameInstance();
	gameInst->DisconnectPlayer(playerName);
	Server_SyncPlayerStateToAllControllers();

	
	// Server PunPlayerController: Send chat "x disconnected"
	auto punPlayer = Cast<APunPlayerController>(gameInst->GetFirstLocalPlayerController());
	_LOG(PunSync, "Logout: punPlayer:%p", punPlayer);
	if (punPlayer) {
		auto command = std::make_shared<FSendChat>();
		command->isSystemMessage = true;
		command->message = playerName + FString(" disconnected");
		punPlayer->networkInterface()->SendNetworkCommand(command);
	}

	
	// PunBasePlayerController
	auto punPlayerBase = Cast<APunBasePlayerController>(Exiting);
	if (punPlayerBase) {
		PUN_DEBUG2("OnLogout: Starting");
		punPlayerBase->OnLogout();
	}
	else {
		PUN_DEBUG2("OnLogout: Not APunPlayerController");
	}

	AGameModeBase::Logout(Exiting);

	gameInst->UpdateSession();
}