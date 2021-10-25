// Pun Dumnernchanvanit's


#include "PunCity/PunGameMode.h"
#include "GameFramework/PlayerState.h"
#include "PunCity/PunUtils.h"
#include "PunCity/PunPlayerController.h"
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
//#include "steam/isteamuser.h"
//#include "steam/isteamutils.h"


void APunGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (_ConnectedControllers.Num() > 0)
	{
		// Already in game, give login error
		auto punPlayer = Cast<APunPlayerController>(_ConnectedControllers[0]);
		if (punPlayer) {
			ErrorMessage = "Game Session already started.";
		}
		else
		{
			auto firstController = CastChecked<APunBasePlayerController>(_ConnectedControllers[0]);
			auto gameInst = firstController->gameInstance();
			FOnlineSessionSettings* sessionsSettings = gameInst->GetSessionSettings();

			PUN_DEBUG2("PreLogin: players:%d", sessionsSettings->NumPublicConnections);

			// Game already full
			if (_ConnectedControllers.Num() >= sessionsSettings->NumPublicConnections)
			{
				ErrorMessage = "Game Session full.";
			}
		}
	}

	AGameModeBase::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

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
	check(NewPlayer->PlayerState);
	
	FString playerName = NewPlayer->PlayerState->GetPlayerName();
	//TSharedPtr<const FUniqueNetId> uniqueNetId = NewPlayer->PlayerState->GetUniqueId().GetUniqueNetId();
	CSteamID steamId(*(uint64*)NewPlayer->PlayerState->GetUniqueId()->GetBytes());
	
	PUN_DEBUG2("PostLogin: %s steamId:%lu steamId64:%llu", *playerName, steamId.GetAccountID(), steamId.ConvertToUint64());

	auto newController = CastChecked<APunBasePlayerController>(NewPlayer);
	auto gameInst = newController->gameInstance();
	auto punPlayer = Cast<APunPlayerController>(NewPlayer);
	bool isInGame = punPlayer != nullptr;

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
	}

	// Cleanup for singlePlayer
	if (gameInst->isSinglePlayer) {
		gameInst->ResetPlayerCount();
	}
	
	int32 newPlayerId = gameInst->ConnectPlayer(playerName, steamId.ConvertToUint64());
	newController->SetControllerPlayerId(newPlayerId);


	// Sync
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

		_LOG(LogNetworkInput, "Sync - Server...SendSaveInfo_ToClient %s size:%d", *saveInfo.name, saveInfo.totalCompressedDataSize());

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
		
		controller->SyncPlayersState_ToClient(gameInst->playerInfoList(),
												gameInst->playerReadyStates(),
												gameInst->playerConnectedStates,
												gameInst->clientPacketsReceived,
												gameInst->hostPlayerId, gameInst->serverTick());
	}
}



void APunGameMode::Logout(AController* Exiting)
{
	check(Exiting->PlayerState);
	FString playerName = Exiting->PlayerState->GetPlayerName();

	CSteamID steamId(*(uint64*)Exiting->PlayerState->GetUniqueId()->GetBytes());
	
	PUN_DEBUG2("Logout: %s steamId:%lu", *playerName, steamId.GetAccountID());

	auto exitingController = CastChecked<APunBasePlayerController>(Exiting);

	// MainMenuPlayerController
	auto mainMenuController = Cast<AMainMenuPlayerController>(exitingController);
	if (mainMenuController) {
		mainMenuController->SendChat_ToServer("", playerName + FString(" disconnected"));
	}
	
	_ConnectedControllers.Remove(exitingController);

	auto gameInst = exitingController->gameInstance();
	gameInst->DisconnectPlayer(playerName, steamId.ConvertToUint64());
	Server_SyncPlayerStateToAllControllers();

	
	// Server PunPlayerController: Send chat "x disconnected"
	auto punPlayer = Cast<APunPlayerController>(gameInst->GetFirstLocalPlayerController());
	_LOG(PunSync, "Logout: punPlayer:%p", punPlayer);
	if (punPlayer) {
		auto command = std::make_shared<FSendChat>();
		command->isSystemMessage = true;
		command->message = playerName + FString(" disconnected");
		punPlayer->networkInterface()->SendNetworkCommand(command);

		// Remove clog for the player that left
		APunPlayerController::kPlayerIdToClogStatus.erase(punPlayer->playerId());
		APunPlayerController::kPlayerIdToMissingGameTick.erase(punPlayer->playerId());
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